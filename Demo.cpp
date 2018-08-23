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



class FrameBuffer {
public:

	int width, height;

	FrameBuffer(int width, int height)
		: width(width), height(height)
	{
		setup();
	}

	~FrameBuffer() {
		glDeleteFramebuffers(1, &fbo);
		glDeleteRenderbuffers(1, &rbo);
	}

	void Bind() { // bind to framebuffer and draw scene as normally
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	}

	void Unbind() {
		// now bind back to default framebuffer and draw a quad plane with attached fb color texture
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	unsigned int FBO() { return fbo; }
	unsigned int TID() { return tid; }
	unsigned int RBO() { return rbo; }

private:
	unsigned int fbo;
	unsigned int tid;
	unsigned int rbo;

	void setup();
};

void FrameBuffer :: setup() {
	//
	#ifdef __APPLE__
	int ratio = 2;
	#else
	int ratio = 1;
	#endif
	// Framebuffer config
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	// Crete a color attachment texture
	glGenTextures(1, &tid);
	glBindTexture(GL_TEXTURE_2D, tid);
	// set tex dimensions to screen size (for OSX, double screen size)
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ratio * width, ratio * height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tid, 0);
	// Create a render buffer obj for depth and stencil attachment
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	// This obj is to be used as an image, not a general data buffer like a texture
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, ratio * width, ratio * height);
	// attach renderbuffer obj to depth and stencil attachment of framebuffer
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
	//
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cerr << "ERROR: Framebuffer is not complete!\n";
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}



// Global Variables
const char* APP_TITLE = "Advanced OpenGL - Demo";
const int gWindowWidth = 1280;
const int gWindowHeight = 720;
GLFWwindow* gWindow = NULL;

// Camera system
Camera camera(glm::vec3(0.0f, 0.0f, 30.0f));

// Function prototypes
void processInput(GLFWwindow* window);
void mouseCallback(GLFWwindow* window, double xpos, double ypos);
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void glfw_onFramebufferSize(GLFWwindow* window, int width, int height);
void showFPS(GLFWwindow* window);
bool initOpenGL();
void renderScene(Shader & shader);

// Models
std::shared_ptr<Model>
objectCountryhouseModel,
objectWarehouseModel,
objectFarmhouseModel,
objectIndustrialFansModel,
objectNanosuit,
objectSphere;

//-----------------------------------------------------------------------------
// Main Application Entry Point
//-----------------------------------------------------------------------------
int main() {

	if (!initOpenGL()){
		// An error occured
		std::cerr << "GLFW initialization failed" << std::endl;
		return -1;
	}

	// Model loader
	//Model objectSponzaModel("Resources/sponza/sponza.obj");
	//Model objectCountryhouseModel("Resources/CountryHouse/house.obj");
	//Model objectWarehouseModel("Resources/warehouse/warehouse.obj");
	//Model objectFarmhouseModel("Resources/farmhouse/farmhouse.obj");
	//Model objectIndustrialFansModel("Resources/IndustrialFans/IndustrialFans.obj");
	//Model objectNanosuit("Resources/nanosuit/nanosuit.obj");
	//Model objectSphere("Resources/sphere/sphere.obj");

	objectCountryhouseModel = std::make_shared<Model>("Resources/CountryHouse/house.obj");
	objectWarehouseModel = std::make_shared<Model>("Resources/warehouse/warehouse.obj");
	objectFarmhouseModel = std::make_shared<Model>("Resources/farmhouse/farmhouse.obj");
	objectIndustrialFansModel = std::make_shared<Model>("Resources/IndustrialFans/IndustrialFans.obj");
	objectNanosuit = std::make_shared<Model>("Resources/nanosuit/nanosuit.obj");
	objectSphere = std::make_shared<Model>("Resources/sphere/sphere.obj");

	// Shader loader
	Shader objectShader("shaders/demo.vert", "shaders/demo.frag");
	Shader screenShader("shaders/screenshader.vert", "shaders/screenshader.frag");
	Shader sphereShader("shaders/sphere.vert", "shaders/sphere.frag");

	// Framebuffer
	FrameBuffer framebuffer(gWindowWidth, gWindowHeight);
	Quad objectQuad;



	// Light global
	glm::vec3 pointLightPos[] = {
		glm::vec3( 3.0f,  0.0f,  0.0f),
		glm::vec3(-3.0f,  0.0f,  0.0f),
		glm::vec3( 0.0f,  0.0f, -3.0f),
		glm::vec3( 0.0f,  0.0f,  3.0f)
	};
	glm::vec3 directionalLightDirection(1.0f, -1.0f, 0.0f);

	// Object shader config
	objectShader.use();
	// Light config
	// Directional light
	objectShader.setUniform("uDirectionalLight.direction", directionalLightDirection);
	objectShader.setUniform("uDirectionalLight.ambient", 0.5f, 0.5f, 0.5f);
	objectShader.setUniform("uDirectionalLight.diffuse", 1.0f, 1.0f, 1.0f);
	objectShader.setUniform("uDirectionalLight.specular", 1.0f, 1.0f, 1.0f);
	// Point light
	//for (int i=0; i<4; i++) {
	//	objectShader.setUniform(("uPointLights[" + std::to_string(i) +"].position").c_str(),  pointLightPos[i]);
	//	objectShader.setUniform(("uPointLights[" + std::to_string(i) +"].ambient").c_str(),   0.2f, 0.2f, 0.2f);
	//	objectShader.setUniform(("uPointLights[" + std::to_string(i) +"].diffuse").c_str(),   1.0f, 1.0f, 1.0f);
	//	objectShader.setUniform(("uPointLights[" + std::to_string(i) +"].specular").c_str(),  1.0f, 1.0f, 1.0f);
	//	objectShader.setUniform(("uPointLights[" + std::to_string(i) +"].constant").c_str(),  1.0f);
	//	objectShader.setUniform(("uPointLights[" + std::to_string(i) +"].linear").c_str(),    0.09f);
	//	objectShader.setUniform(("uPointLights[" + std::to_string(i) +"].quadratic").c_str(), 0.032f);
	//}
	// Spot light
	objectShader.setUniform("uSpotLight.innerCutOff", glm::cos(glm::radians(12.5f)));
	objectShader.setUniform("uSpotLight.outerCutOff", glm::cos(glm::radians(17.5f)));
	objectShader.setUniform("uSpotLight.ambient", 0.0f, 0.0f, 0.0f);
	objectShader.setUniform("uSpotLight.diffuse", 1.0f, 1.0f, 1.0f);
	objectShader.setUniform("uSpotLight.specular", 1.0f, 1.0f, 1.0f);
	objectShader.setUniform("uSpotLight.constant", 1.0f);
	objectShader.setUniform("uSpotLight.linear", 0.09f);
	objectShader.setUniform("uSpotLight.quadratic", 0.032f);



	// Camera global
	float width_height_ratio = (float)gWindowWidth / (float)gWindowHeight;



	// Rendering loop
	while (!glfwWindowShouldClose(gWindow)) {

		// Display FPS on title
		showFPS(gWindow);

		// Key input
		processInput(gWindow);



		// Camera transformations
		glm::mat4 view = camera.getViewMatrix();glm::mat4 projection;
		projection = glm::perspective(glm::radians(camera.fov), width_height_ratio, 0.1f, 100.0f);
		//projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 100.0f);

		// Set shader(s) and draw model(s)
		objectShader.use();
		objectShader.setUniform("uView", view);
		objectShader.setUniform("uProjection", projection);
		objectShader.setUniform("uCameraPos", camera.position);

		objectShader.setUniform("uSpotLight.position", camera.position);
		objectShader.setUniform("uSpotLight.direction", camera.front);

		// Draw scene
		framebuffer.Bind();
		glEnable(GL_DEPTH_TEST);
		//glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		renderScene(objectShader);

		framebuffer.Unbind();


		// Draw scene on framebuffer
		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//screenShader.use();
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, framebuffer.TID());
		//screenShader.setUniform("uMaterial.texture1", 0); // load texture manually
		//modelMatrix = glm::mat4(1.0f);
		//modelMatrix = glm::translate(modelMatrix, glm::vec3(0.5f, 0.5f, 0.0f));
		//modelMatrix = glm::scale(modelMatrix, glm::vec3(0.5f, 0.5f, 1.0f));
		//screenShader.use();
		//screenShader.setUniform("uProcessMode", 1);
		//screenShader.setUniform("uModel", modelMatrix);
		//objectQuad.Draw(screenShader);

		sphereShader.use();
		sphereShader.setUniform("uView", view);
		sphereShader.setUniform("uProjection", projection);
		glActiveTexture(GL_TEXTURE0 + 3);
		glBindTexture(GL_TEXTURE_2D, framebuffer.TID());
		sphereShader.setUniform("sphereMap", 3);

		float degree = (float) glfwGetTime() * glm::radians(10.0f);

		glm::mat4 modelMatrix;
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, 25.0f));
		modelMatrix = glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		//modelMatrix = glm::scale(modelMatrix, glm::vec3(0.2f, 0.2f, 0.2f));
		sphereShader.setUniform("uModel", modelMatrix);
		objectSphere.get()->Draw(sphereShader);

		renderScene(objectShader);



		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		glfwPollEvents();
		glfwSwapBuffers(gWindow);
	}
	
	glfwTerminate();

	return 0;
}

void renderScene(Shader & shader) {

	glm::mat4 modelMatrix;

	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-30.0f, -5.0f, 0.0f));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	shader.use();
	shader.setUniform("uModel", modelMatrix);
	objectFarmhouseModel.get()->Draw(shader);
	
	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(30.0f, 0.0f, 0.0f));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(2.0f, 2.0f, 2.0f));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	shader.use();
	shader.setUniform("uModel", modelMatrix);
	objectWarehouseModel.get()->Draw(shader);

	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(10.0f, -5.0f, 0.0f));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(0.002f, 0.002f, 0.002f));
	//modelMatrix = glm::rotate(modelMatrix, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	shader.use();
	shader.setUniform("uModel", modelMatrix);
	objectCountryhouseModel.get()->Draw(shader);

	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-4.0f, -1.0f, 25.0f));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(0.2f, 0.2f, 0.2f));
	shader.use();
	shader.setUniform("uModel", modelMatrix);
	objectNanosuit.get()->Draw(shader);

	for (int i=0; i<4; i++) {
		modelMatrix = glm::mat4(1.0f);
		glm::vec3 FansPosition(-34.0f + i * 2.5f, -3.5f, 17.0f);
		modelMatrix = glm::translate(modelMatrix, FansPosition);
		shader.use();
		shader.setUniform("uModel", modelMatrix);
		objectIndustrialFansModel.get()->Draw(shader);
	}
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

	// Configure global OpenGL state	
	glEnable(GL_DEPTH_TEST);

	// Blending functionality
	glEnable(GL_BLEND);
	// Tell OpenGL how to calc colors of blended fragments (pixels)
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

	// FPS
	static float deltaTime = 0.0f;
	static float lastFrame = 0.0f;
	// Per-frame time
	float currentFrame = (float) glfwGetTime();
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
