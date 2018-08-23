#include <iostream>
#include <sstream>
#include <string>

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

class DepthMap
{
public:

	int width;
	int height;

	DepthMap(int width = 1024, int height = 1024)
		: width(width), height(height)
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
	glBindTexture(GL_TEXTURE_2D, tid);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height,
		0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	// generate fbo, attach depth texture as fbo's depth buffer
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tid, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

// Function prototypes
bool initOpenGL();
void processInput(GLFWwindow* window);
void mouseCallback(GLFWwindow* window, double xpos, double ypos);
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void glfw_onFramebufferSize(GLFWwindow* window, int width, int height);
void showFPS(GLFWwindow* window);
void renderScene(Shader & shader, Plane & plane, Cube & cube, Model &);

/************************************************
*
* Don't define Primitives before OpenGL start!
*
*************************************************/

int main() {

	if (!initOpenGL()){
		// An error occured
		std::cerr << "GLFW initialization failed" << std::endl;
		return -1;
	}

	// build and compile shaders
	// -------------------------
	Shader objectShader, simpleDepthShader, debugDepthQuad;
	objectShader.loadShaders("shaders/parallel_shadow.vert", "shaders/parallel_shadow.frag");
	simpleDepthShader.loadShaders("shaders/parallel_shadow_map.vert", "shaders/parallel_shadow_map.frag");
	debugDepthQuad.loadShaders("shaders/parallel_shadow_debug.vert", "shaders/parallel_shadow_debug.frag");

	Model objPlanet("Resources/planet/planet.obj");
	Plane objPlane;
	Cube objCube;
	Quad objQuad;

	// configure depth map FBO
	// -----------------------
	DepthMap depthMap;

	// lighting info
	// -------------
	glm::vec3 lightPos(-2.0f, 4.0f, -1.0f);

	// load textures
	// -------------
	unsigned int woodTexture = LoadTexture("Resources/default/wood.png");

	// shader configuration
	// --------------------
	debugDepthQuad.use();
	debugDepthQuad.setUniform("uDepthMap", 0);

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

	objectShader.setUniform("uMaterial.texture_diffuse1", 0);
	objectShader.setUniform("uMaterial.texture_specular1", 0);
	objectShader.setUniform("uShadowMap", 15);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(gWindow)) {

		// Display FPS on title
		showFPS(gWindow);

		// input
		// -----
		processInput(gWindow);

		// render
		// ------
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// 1. render depth of scene to texture (from light's perspective)
		// --------------------------------------------------------------
		glm::mat4 lightProjection, lightView;
		float near_plane = 1.0f, far_plane = 7.5f;
		float aspect = (float) gWindowWidth / (float) gWindowHeight;
		//lightProjection = glm::perspective(glm::radians(camera.fov), aspect, 0.1f, 100.0f);
		lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
		lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
		//lightView = camera.getViewMatrix();
		// render scene from light's point of view
		simpleDepthShader.use();
		simpleDepthShader.setUniform("uView", lightView);
		simpleDepthShader.setUniform("uProjection", lightProjection);

		glViewport(0, 0, depthMap.width, depthMap.height);
		depthMap.Bind();
		glClear(GL_DEPTH_BUFFER_BIT);
		glCullFace(GL_FRONT);
		renderScene(simpleDepthShader, objPlane, objCube, objPlanet);
		glCullFace(GL_BACK);
		depthMap.Unbind();

		// reset viewport
		#ifdef __APPLE__
		glViewport(0, 0, 2 * gWindowWidth, 2 * gWindowHeight);
		#else
		glViewport(0, 0, gWindowWidth, gWindowHeight);
		#endif
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// 2. render scene as normal unsing the generated depth/shadow map
		// ---------------------------------------------
		objectShader.use();
		glm::mat4 view = camera.getViewMatrix();
		glm::mat4 projection = glm::perspective(glm::radians(camera.fov), aspect, 0.1f, 100.0f);
		objectShader.setUniform("uView", view);
		objectShader.setUniform("uProjection", projection);
		objectShader.setUniform("uCameraPos", camera.position);
		objectShader.setUniform("uBlinn", use_blinn);
		objectShader.setUniform("uGamma", use_gamma);
		objectShader.setUniform("uTorch", use_torch);
		// set light uniforms
		objectShader.setUniform("uSpotLight.position", camera.position);
		objectShader.setUniform("uSpotLight.direction", camera.front);
		objectShader.setUniform("uLightSpaceMatrix", lightProjection * lightView);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, woodTexture);
		glActiveTexture(GL_TEXTURE15);
		glBindTexture(GL_TEXTURE_2D, depthMap.TID());
		renderScene(objectShader, objPlane, objCube, objPlanet);

		// render Depth map to quad for visual debugging
		// ---------------------------------------------
		//debugDepthQuad.use();
		//debugDepthQuad.setUniform("uNearPlane", near_plane);
		//debugDepthQuad.setUniform("uFarPlane", far_plane);
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, depthMap);
		//objQuad.Draw(debugDepthQuad);

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
void renderScene(Shader &shader, Plane & plane, Cube & cube, Model & obj)
{
	// floor
	glm::mat4 model;
	model = glm::translate(model, glm::vec3(0.0f, -0.5f, 0.0f));
	model = glm::scale(model, glm::vec3(50.0f));
	shader.use();
	shader.setUniform("uModel", model);
	glBindVertexArray(plane.VAO());
	glDrawElements(GL_TRIANGLES, plane.indices.size(), GL_UNSIGNED_INT, 0);
	// cubes
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(0.0f, 1.5f, 0.0));
	shader.use();
	shader.setUniform("uModel", model);
	cube.Draw(shader);

	model = glm::mat4();
	model = glm::translate(model, glm::vec3(2.0f, 0.0f, 1.0));
	shader.use();
	shader.setUniform("uModel", model);
	cube.Draw(shader);

	model = glm::mat4();
	model = glm::translate(model, glm::vec3(-1.0f, 0.0f, 2.0));
	model = glm::rotate(model, (float) glfwGetTime() * glm::radians(10.0f),
		glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
	model = glm::scale(model, glm::vec3(0.5f));
	shader.use();
	shader.setUniform("uModel", model);
	cube.Draw(shader);

	model = glm::mat4();
	model = glm::translate(model, glm::vec3(-2.0f, 1.0f, -1.0));
	model = glm::scale(model, glm::vec3(0.2f));
	shader.use();
	shader.setUniform("uModel", model);
	obj.Draw(shader);
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
		use_gamma = use_gamma <= 1.0f ? 1.0f : use_gamma - 0.01f;
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
