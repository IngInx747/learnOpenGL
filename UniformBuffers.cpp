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

// Global Variables
const char* APP_TITLE = "Advanced OpenGL - Demo";
const int gWindowWidth = 800;
const int gWindowHeight = 600;
GLFWwindow* gWindow = NULL;

// Camera system
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
bool firstMouse = true;
float lastX = gWindowWidth / 2;
float lastY = gWindowHeight / 2;

// FPS
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Function prototypes
void processInput(GLFWwindow* window);
void mouseCallback(GLFWwindow* window, double xpos, double ypos);
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void glfw_onFramebufferSize(GLFWwindow* window, int width, int height);
void showFPS(GLFWwindow* window);
bool initOpenGL();

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
	Cube cube;

	// Shader loader
	Shader shaders[4];
	shaders[0].loadShaders("shaders/uniformbuffers.vert", "shaders/ubR.frag");
	shaders[1].loadShaders("shaders/uniformbuffers.vert", "shaders/ubG.frag");
	shaders[2].loadShaders("shaders/uniformbuffers.vert", "shaders/ubB.frag");
	shaders[3].loadShaders("shaders/uniformbuffers.vert", "shaders/ubY.frag");



	// Configure a uniform buffer object
	// ---------------------------------
	// 1. Get relevant block indices
	GLuint uBlockIds[4];
	uBlockIds[0] = glGetUniformBlockIndex(shaders[0].ID(), "uMatrices");
	uBlockIds[1] = glGetUniformBlockIndex(shaders[1].ID(), "uMatrices");
	uBlockIds[2] = glGetUniformBlockIndex(shaders[2].ID(), "uMatrices");
	uBlockIds[3] = glGetUniformBlockIndex(shaders[3].ID(), "uMatrices");
	// 2. Link each shader's uniform block to this uniform binding point
	glUniformBlockBinding(shaders[0].ID(), uBlockIds[0], 0);
	glUniformBlockBinding(shaders[1].ID(), uBlockIds[1], 0);
	glUniformBlockBinding(shaders[2].ID(), uBlockIds[2], 0);
	glUniformBlockBinding(shaders[3].ID(), uBlockIds[3], 0);
	// 3. Create the buffer
	GLuint uboMatrices;
	glGenBuffers(1, &uboMatrices);
	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	// 4. Define range of the buffer linking to a uniform binding point
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboMatrices, 0, 2 * sizeof(glm::mat4));
	// 5. Store related data
	float width_height_ratio = (float)gWindowWidth / (float)gWindowHeight;
	glm::mat4 projection = glm::perspective(glm::radians(camera.fov), width_height_ratio, 0.1f, 100.0f);
	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(projection));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	// Notice, there are 4 shaders, but one only needs to set camera 1 time, instead of 4.



	// Rendering loop
	while (!glfwWindowShouldClose(gWindow)) {

		// Per-frame time
		float currentFrame = (float) glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Display FPS on title
		showFPS(gWindow);

		// Key input
		processInput(gWindow);

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



		// Set view together with projection matrix in the uniform block
		glm::mat4 view = camera.getViewMatrix();
		glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(view));
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		// Notice, there are 4 shaders, but one only needs to set camera 1 time, instead of 4.

		// Draw scene
		glm::mat4 modelMatrix;

		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.75f, 0.75f, 0.0f));
		shaders[0].use();
		shaders[0].setUniform("uModel", modelMatrix);
		cube.Draw(shaders[0]);

		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0.75f, 0.75f, 0.0f));
		shaders[1].use();
		shaders[1].setUniform("uModel", modelMatrix);
		cube.Draw(shaders[1]);

		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.75f, -0.75f, 0.0f));
		shaders[2].use();
		shaders[2].setUniform("uModel", modelMatrix);
		cube.Draw(shaders[2]);

		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0.75f, -0.75f, 0.0f));
		shaders[3].use();
		shaders[3].setUniform("uModel", modelMatrix);
		cube.Draw(shaders[3]);



		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		glfwPollEvents();
		glfwSwapBuffers(gWindow);
	}
	
	glfwTerminate();

	return 0;
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

	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		camera.processAccerlate(true);
	else
		camera.processAccerlate(false);

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
