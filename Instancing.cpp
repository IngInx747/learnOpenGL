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

// Global Variables
const char* APP_TITLE = "Advanced OpenGL - Instancing";
const int gWindowWidth = 800;
const int gWindowHeight = 600;
GLFWwindow* gWindow = NULL;

// Camera system
Camera camera(glm::vec3(0.0f, 30.0f, 255.0f));
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
	//Model objectSponzaModel("Resources/sponza/sponza.obj");
	//Model objectCountryhouseModel("Resources/CountryHouse/house.obj");
	//Model objectWarehouseModel("Resources/warehouse/warehouse.obj");
	//Model objectFarmhouseModel("Resources/farmhouse/farmhouse.obj");
	//Model objectIndustrialFansModel("Resources/IndustrialFans/IndustrialFans.obj");
	//Model objectNanosuit("Resources/nanosuit/nanosuit.obj");
	Model objectPlanet("Resources/planet/planet.obj");
	Model objectRock("Resources/rock/rock.obj");

	// Shader loader
	Shader objectShader, instanceShader;
	objectShader.loadShaders("shaders/demo.vert", "shaders/demo.frag");
	instanceShader.loadShaders("shaders/instancing.vert", "shaders/instancing.frag");



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
	objectShader.setUniform("uDirectionalLight.direction", directionalLightDirection);
	objectShader.setUniform("uDirectionalLight.ambient", 0.5f, 0.5f, 0.5f);
	objectShader.setUniform("uDirectionalLight.diffuse", 1.0f, 1.0f, 1.0f);
	objectShader.setUniform("uDirectionalLight.specular", 1.0f, 1.0f, 1.0f);
	objectShader.setUniform("uSpotLight.innerCutOff", glm::cos(glm::radians(12.5f)));
	objectShader.setUniform("uSpotLight.outerCutOff", glm::cos(glm::radians(17.5f)));
	objectShader.setUniform("uSpotLight.ambient", 0.0f, 0.0f, 0.0f);
	objectShader.setUniform("uSpotLight.diffuse", 1.0f, 1.0f, 1.0f);
	objectShader.setUniform("uSpotLight.specular", 1.0f, 1.0f, 1.0f);
	objectShader.setUniform("uSpotLight.constant", 1.0f);
	objectShader.setUniform("uSpotLight.linear", 0.09f);
	objectShader.setUniform("uSpotLight.quadratic", 0.032f);

	instanceShader.use();
	instanceShader.setUniform("uDirectionalLight.direction", directionalLightDirection);
	instanceShader.setUniform("uDirectionalLight.ambient", 0.5f, 0.5f, 0.5f);
	instanceShader.setUniform("uDirectionalLight.diffuse", 1.0f, 1.0f, 1.0f);
	instanceShader.setUniform("uDirectionalLight.specular", 1.0f, 1.0f, 1.0f);
	instanceShader.setUniform("uSpotLight.innerCutOff", glm::cos(glm::radians(12.5f)));
	instanceShader.setUniform("uSpotLight.outerCutOff", glm::cos(glm::radians(17.5f)));
	instanceShader.setUniform("uSpotLight.ambient", 0.0f, 0.0f, 0.0f);
	instanceShader.setUniform("uSpotLight.diffuse", 1.0f, 1.0f, 1.0f);
	instanceShader.setUniform("uSpotLight.specular", 1.0f, 1.0f, 1.0f);
	instanceShader.setUniform("uSpotLight.constant", 1.0f);
	instanceShader.setUniform("uSpotLight.linear", 0.09f);
	instanceShader.setUniform("uSpotLight.quadratic", 0.032f);



	// Generate a large list of semi-random model transformation matrices
	unsigned int cnt_obj = 100000;
	glm::mat4 modelMatrices[cnt_obj];
	srand(glfwGetTime());
	float radius = 150.0f;
	float offset = 25.0f;

	for (unsigned int i=0; i<cnt_obj; i++) {

		glm::mat4 matrix;

		float angle = (float) i / (float) cnt_obj * 360.0f;
		float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float rx = sin(angle) * radius + displacement;
		displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float ry = displacement * 0.4f;
		displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float rz = cos(angle) * radius + displacement;
		matrix = glm::translate(matrix, glm::vec3(rx, ry, rz));

		float scale = (rand() % 20) / 100.0f + 0.05;
		matrix = glm::scale(matrix, glm::vec3(scale));

		float rotateAngle = (rand() % 360);
		matrix = glm::rotate(matrix, rotateAngle, glm::vec3(0.4f, 0.6f, 0.8f));

		modelMatrices[i] = matrix;
	}

	unsigned int ibo;
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ARRAY_BUFFER, ibo);
	glBufferData(GL_ARRAY_BUFFER, cnt_obj * sizeof(glm::mat4), &modelMatrices[0], GL_STATIC_DRAW);

	for (Mesh & mesh : objectRock.meshes) {

		unsigned rockVAO = mesh.VAO();
		glBindVertexArray(rockVAO);
		size_t vec4Size = (int) sizeof(glm::vec4);

		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(vec4Size));
		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));

		glVertexAttribDivisor(3, 1);
		glVertexAttribDivisor(4, 1);
		glVertexAttribDivisor(5, 1);
		glVertexAttribDivisor(6, 1);

		glBindVertexArray(0);
	}



	// Camera global
	float width_height_ratio = (float)gWindowWidth / (float)gWindowHeight;



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



		// Camera transformations
		glm::mat4 view = camera.getViewMatrix();
		glm::mat4 projection = glm::perspective(glm::radians(camera.fov), width_height_ratio, 0.1f, 1000.0f);
		//glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 100.0f);

		objectShader.use();
		objectShader.setUniform("uView", view);
		objectShader.setUniform("uProjection", projection);
		objectShader.setUniform("uCameraPos", camera.position);

		objectShader.setUniform("uSpotLight.position", camera.position);
		objectShader.setUniform("uSpotLight.direction", camera.front);

		instanceShader.use();
		instanceShader.setUniform("uView", view);
		instanceShader.setUniform("uProjection", projection);
		instanceShader.setUniform("uCameraPos", camera.position);
		instanceShader.setUniform("uSpotLight.position", camera.position);
		instanceShader.setUniform("uSpotLight.direction", camera.front);

		// Draw Models
		glm::mat4 modelMatrix;

		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, -3.0f, 0.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(4.0f, 4.0f, 4.0f));
		objectShader.use();
		objectShader.setUniform("uModel", modelMatrix);
		objectPlanet.Draw(objectShader);

		// To see difference between using Instancing (1) and not (2)

		// (1)
		instanceShader.use();
		instanceShader.setUniform("uMaterial.texture_diffuse1", 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, objectRock.textures_loaded[0].id);
		for (Mesh & mesh : objectRock.meshes) {
			glBindVertexArray(mesh.VAO());
			glDrawElementsInstanced(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0, cnt_obj);
			glBindVertexArray(0);
		}
		
		// (2)
		//for (unsigned int i=0; i<cnt_obj; i++) {
		//	objectShader.use();
		//	objectShader.setUniform("uModel", modelMatrices[i]);
		//	objectRock.Draw(objectShader);
		//}



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

	static bool firstMouse = true;

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
