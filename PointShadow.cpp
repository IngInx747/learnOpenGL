#include <iostream>
#include <sstream>
#include <string>
#include <memory>

/** Basic GLFW header */
//#include <GL/glew.h>	// Important - this header must come before glfw3 header
#include <glad/glad.h>
#include <GLFW/glfw3.h>

/** GLFW Math */
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

/** GLFW Texture header */
#include <stb_image/stb_image.h> // Support several formats of image file

/** Shader Wrapper */
#include <ShaderProgram.h>

/** Camera Wrapper */
#include <EularCamera.h>

/** Model Wrapper */
#include <Model.h>
#include <Primitives.h>

/************************************************
* Deoth Map Framebuffer
*************************************************/

class DepthMap {
public:

	int width, height;
	float near, far;

	DepthMap(int width, int height, float near, float far)
		: width(width), height(height), near(near), far(far)
	{
		setup();
	}

	~DepthMap() {
		glDeleteFramebuffers(1, &fbo);
	}

	void Bind() {
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	}

	void Unbind() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	std::vector<glm::mat4> GetTransforms(glm::vec3 & position) const;

	unsigned int FBO() { return fbo; }
	unsigned int TID() { return tid; }
	
private:

	unsigned int fbo;
	unsigned int tid;

	void setup();
};

void DepthMap :: setup() {
	// create depth texture
	glGenTextures(1, &tid);
	glBindTexture(GL_TEXTURE_CUBE_MAP, tid);
	for (unsigned int i = 0; i < 6; i++)
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, width, height,
			0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	// generate fbo, attach depth texture as fbo's depth buffer
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, tid, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

std::vector<glm::mat4> DepthMap :: GetTransforms(glm::vec3 & lightPos) const {
	glm::mat4 shadowProjection = glm::perspective(glm::radians(90.0f), 1.0f, near, far);
	std::vector<glm::mat4> shadowTransforms;
	shadowTransforms.push_back(shadowProjection * glm::lookAt(lightPos,
		lightPos + glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3( 0.0f, -1.0f,  0.0f)));
	shadowTransforms.push_back(shadowProjection * glm::lookAt(lightPos,
		lightPos + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3( 0.0f, -1.0f,  0.0f)));
	shadowTransforms.push_back(shadowProjection * glm::lookAt(lightPos,
		lightPos + glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3( 0.0f,  0.0f,  1.0f)));
	shadowTransforms.push_back(shadowProjection * glm::lookAt(lightPos,
		lightPos + glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3( 0.0f,  0.0f, -1.0f)));
	shadowTransforms.push_back(shadowProjection * glm::lookAt(lightPos,
		lightPos + glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3( 0.0f, -1.0f,  0.0f)));
	shadowTransforms.push_back(shadowProjection * glm::lookAt(lightPos,
		lightPos + glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3( 0.0f, -1.0f,  0.0f)));
	return shadowTransforms;
}

// Global Variables
const char* APP_TITLE = "Advanced OpenGL - Shadow Mapping";
const int gWindowWidth = 1280;
const int gWindowHeight = 720;
GLFWwindow* gWindow = NULL;

// Camera system
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

// Lighting mode
bool use_torch = true;
bool use_blinn = false;
float use_gamma = 2.2f;

// General function
bool initOpenGL();
void processInput(GLFWwindow* window);
void mouseCallback(GLFWwindow* window, double xpos, double ypos);
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void glfw_onFramebufferSize(GLFWwindow* window, int width, int height);
void showFPS(GLFWwindow* window);

// Scene related
std::shared_ptr<Base3D> pObjPlane, pObjCube;
std::shared_ptr<Model> pObjPlanet;
void renderScene(Shader & shader);

/************************************************
* Main
*************************************************/

int main() {

	if (!initOpenGL()){
		// An error occured
		std::cerr << "GLFW initialization failed" << std::endl;
		return -1;
	}

	// build and compile shaders
	// -------------------------
	Shader objectShader, simpleDepthShader;
	objectShader.loadShaders(
		"shaders/point_shadow.vert",
		"shaders/point_shadow.frag");
	simpleDepthShader.loadShaders(
		"shaders/point_shadow_map.vert",
		"shaders/point_shadow_map.frag",
		"shaders/point_shadow_map.geom");

	// load models and primitives
	// --------------------------
	pObjPlanet = std::make_shared<Model>("Resources/planet/planet.obj");
	pObjPlane  = std::make_shared<Plane>();
	pObjCube   = std::make_shared<Cube>();

	// load textures
	// -------------
	pObjCube.get()->AddTexture("Resources/default/wood.png", TEX_DIFFUSE);
	pObjCube.get()->AddTexture("Resources/default/wood.png", TEX_SPECULAR);

	// configure depth map FBO
	// -----------------------
	DepthMap depthMap(1024, 1024, 1.0f, 25.0f);
	unsigned int depthMapTexUnit = 15;

	// lighting info
	// -------------
	glm::vec3 lightPos(0.0f, 0.0f, 0.0f);

	// shader configuration
	// --------------------
	objectShader.use();
	objectShader.setUniform("uPointLight.position",  lightPos);
	objectShader.setUniform("uPointLight.ambient",   0.0f, 0.0f, 0.0f);
	objectShader.setUniform("uPointLight.diffuse",   1.0f, 1.0f, 1.0f);
	objectShader.setUniform("uPointLight.specular",  1.0f, 1.0f, 1.0f);
	objectShader.setUniform("uPointLight.constant",  1.0f);
	objectShader.setUniform("uPointLight.linear",    0.09f);
	objectShader.setUniform("uPointLight.quadratic", 0.032f);
	// Spot light
	objectShader.setUniform("uSpotLight.innerCutOff", glm::cos(glm::radians(12.5f)));
	objectShader.setUniform("uSpotLight.outerCutOff", glm::cos(glm::radians(17.5f)));
	objectShader.setUniform("uSpotLight.ambient",  0.0f, 0.0f, 0.0f);
	objectShader.setUniform("uSpotLight.diffuse",  1.0f, 1.0f, 1.0f);
	objectShader.setUniform("uSpotLight.specular", 1.0f, 1.0f, 1.0f);
	objectShader.setUniform("uSpotLight.constant", 1.0f);
	objectShader.setUniform("uSpotLight.linear", 0.09f);
	objectShader.setUniform("uSpotLight.quadratic", 0.032f);
	// Textures
	// ...
	// Shadow map
	objectShader.setUniform("uShadowMap", (int) depthMapTexUnit);

	float aspect = (float) gWindowWidth / (float) gWindowHeight;

	// render loop
	// -----------
	while (!glfwWindowShouldClose(gWindow)) {

		// Display FPS on title
		showFPS(gWindow);

		// input
		// -----
		processInput(gWindow);

		// move light position over time
		// -----
		lightPos.z = sin(glfwGetTime() * 0.5f) * 3.0f;

		// render
		// ------
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// 0. create depth cubemap transformation matrices
		// --------------------------------------------------------------
		std::vector<glm::mat4> shadowTransforms = depthMap.GetTransforms(lightPos);

		// 1. render scene to depth cubemap
		// --------------------------------------------------------------
		glViewport(0, 0, depthMap.width, depthMap.height);
		depthMap.Bind();
		glClear(GL_DEPTH_BUFFER_BIT);
		simpleDepthShader.use();
		simpleDepthShader.setUniform("uFarPlane", depthMap.far);
		simpleDepthShader.setUniform("uLightPos", lightPos);
		for (int i = 0; i < 6; i++)
			simpleDepthShader.setUniform("uShadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
		renderScene(simpleDepthShader);
		depthMap.Unbind();

		// 2. render scene as normal unsing the generated depth/shadow map
		// ---------------------------------------------
		// reset viewport
		#ifdef __APPLE__
		glViewport(0, 0, 2 * gWindowWidth, 2 * gWindowHeight);
		#else
		glViewport(0, 0, gWindowWidth, gWindowHeight);
		#endif
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 view = camera.getViewMatrix();
		glm::mat4 projection = glm::perspective(glm::radians(camera.fov), aspect, 0.1f, 100.0f);
		objectShader.use();
		objectShader.setUniform("uView", view);
		objectShader.setUniform("uProjection", projection);
		objectShader.setUniform("uCameraPos", camera.position);
		objectShader.setUniform("uBlinn", use_blinn);
		objectShader.setUniform("uGamma", use_gamma);
		objectShader.setUniform("uTorch", use_torch);
		objectShader.setUniform("uFarPlane", depthMap.far);
		// set point light
		objectShader.setUniform("uPointLight.position", lightPos);
		// set spot light
		objectShader.setUniform("uSpotLight.position", camera.position);
		objectShader.setUniform("uSpotLight.direction", camera.front);
		// bind shadow map texture
		glActiveTexture(GL_TEXTURE0 + depthMapTexUnit);
		glBindTexture(GL_TEXTURE_CUBE_MAP, depthMap.TID());
		// render scene as normal case
		renderScene(objectShader);

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(gWindow);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}

// renders the 3D scene
// --------------------
void renderScene(Shader &shader)
{
	// Room
	glm::mat4 model;
	model = glm::scale(model, glm::vec3(10.0f));
	shader.use();
	shader.setUniform("uModel", model);
	glDisable(GL_CULL_FACE);
	shader.setUniform("uReverseNormal", 1);
	pObjCube.get()->Draw(shader);
	shader.setUniform("uReverseNormal", 0);
	glEnable(GL_CULL_FACE);

	// cubes
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(4.0f, -3.5f, 0.0f));
	shader.use();
	shader.setUniform("uModel", model);
	pObjCube.get()->Draw(shader);

	model = glm::mat4();
	model = glm::translate(model, glm::vec3(2.0f, 3.0f, 1.0f));
	model = glm::scale(model, glm::vec3(1.5f));
	shader.use();
	shader.setUniform("uModel", model);
	pObjCube.get()->Draw(shader);

	model = glm::mat4();
	model = glm::translate(model, glm::vec3(-3.0f, -1.0f, 0.0f));
	shader.use();
	shader.setUniform("uModel", model);
	pObjCube.get()->Draw(shader);

	model = glm::mat4();
	model = glm::translate(model, glm::vec3(-1.5f, 1.0f, 1.5f));
	shader.use();
	shader.setUniform("uModel", model);
	pObjCube.get()->Draw(shader);

	model = glm::mat4();
	model = glm::translate(model, glm::vec3(-1.5f, 2.0f, -3.0f));
	model = glm::scale(model, glm::vec3(1.5f));
	model = glm::rotate(model, (float) glfwGetTime() * glm::radians(10.0f),
		glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
	shader.use();
	shader.setUniform("uModel", model);
	pObjCube.get()->Draw(shader);

	// Model
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(2.0f, 1.0f, -1.0));
	model = glm::scale(model, glm::vec3(0.2f));
	shader.use();
	shader.setUniform("uModel", model);
	pObjPlanet.get()->Draw(shader);
}

//-----------------------------------------------------------------------------
// Initialize GLFW and OpenGL
//-----------------------------------------------------------------------------
bool initOpenGL() {

	// Intialize GLFW 
	// GLFW is configured.  Must be called before calling any GLFW functions
	if (!glfwInit()) {
		// An error occured
		std::cerr << "GLFW initialization failed" << std::endl;
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// forward compatible with newer versions of OpenGL as they become available
	// but not backward compatible (it will not run on devices that do not support OpenGL 3.3
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// Create an OpenGL 3.3 core, forward compatible context window
	gWindow = glfwCreateWindow(gWindowWidth, gWindowHeight, APP_TITLE, NULL, NULL);
	if (gWindow == NULL) {
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}

	// Make the window's context the current one
	glfwMakeContextCurrent(gWindow);

	// Set the required callback functions
	//glfwSetKeyCallback(gWindow, glfw_onKey);
	glfwSetCursorPosCallback(gWindow, mouseCallback);
	glfwSetScrollCallback(gWindow, scrollCallback);
	glfwSetFramebufferSizeCallback(gWindow, glfw_onFramebufferSize);

	// Initialize GLAD: load all OpenGL function pointers
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD" << std::endl;
		return false;
	}

	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);

	// Define the viewport dimensions
	//glViewport(0, 0, gWindowWidth, gWindowHeight);

	// Depth testing
	glEnable(GL_DEPTH_TEST);

	// Winding order
	glEnable(GL_CULL_FACE);

	// Blending
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Hide the cursor and capture it
	glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	return true;
}

//-----------------------------------------------------------------------------
// Is called whenever a key is pressed/released via GLFW
//-----------------------------------------------------------------------------
void processInput(GLFWwindow* window) {

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	// per-frame time logic
	// --------------------
	static float deltaTime = 0.0f;
	static float lastFrame = 0.0f;
	float currentFrame = glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.processKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.processKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.processKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.processKeyboard(RIGHT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		camera.processKeyboard(UP, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		camera.processKeyboard(DOWN, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		camera.processAccerlate(true);
	else
		camera.processAccerlate(false);

	static bool gWireframe = false;
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
		gWireframe = !gWireframe;
		if (gWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
		use_torch = !use_torch;
	if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
		use_blinn = !use_blinn;
	if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS)
		use_gamma = use_gamma >= 4.0f ? 4.0f : use_gamma + 0.01f;
	if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS)
		use_gamma = use_gamma <= 0.1f ? 0.1f : use_gamma - 0.01f;
}

//-----------------------------------------------------------------------------
// Is called whenever mouse movement is detected via GLFW
//-----------------------------------------------------------------------------
void mouseCallback(GLFWwindow* window, double xpos, double ypos) {

	static bool firstMouse = true;
	static float lastX = gWindowWidth / 2;
	static float lastY = gWindowHeight / 2;

	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coord range from buttom to top
	lastX = xpos;
	lastY = ypos;

	camera.processMouse(xoffset, yoffset);
}

//-----------------------------------------------------------------------------
// Is called whenever scroller is detected via GLFW
//-----------------------------------------------------------------------------
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	camera.processScroll(yoffset);
}

//-----------------------------------------------------------------------------
// Is called when the window is resized
//-----------------------------------------------------------------------------
void glfw_onFramebufferSize(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

//-----------------------------------------------------------------------------
// Code computes the average frames per second, and also the average time it takes
// to render one frame.  These stats are appended to the window caption bar.
//-----------------------------------------------------------------------------
void showFPS(GLFWwindow* window)
{
	static double previousSeconds = 0.0;
	static int frameCount = 0;
	double elapsedSeconds;
	double currentSeconds = glfwGetTime(); // returns number of seconds since GLFW started, as double float

	elapsedSeconds = currentSeconds - previousSeconds;

	// Limit text updates to 4 times per second
	if (elapsedSeconds > 0.25)
	{
		previousSeconds = currentSeconds;
		double fps = (double)frameCount / elapsedSeconds;
		double msPerFrame = 1000.0 / fps;

		// The C++ way of setting the window title
		std::ostringstream outs;
		outs.precision(3);	// decimal places
		outs << std::fixed
			<< APP_TITLE << "    "
			<< "FPS: " << fps << "    "
			<< "Frame Time: " << msPerFrame << " (ms)";
		glfwSetWindowTitle(window, outs.str().c_str());

		// Reset for next average.
		frameCount = 0;
	}

	frameCount++;
}
