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
#include <Texture.h>

/** Shader Wrapper */
#include <ShaderProgram.h>

/** Camera Wrapper */
#include <EularCamera.h>

/** Model Wrapper */
#include <Model.h>
#include <Primitives.h>



int texture_skybox_index = 15;



std::vector<float> skyboxVertices = {
	// front
	-1.0, -1.0,  1.0,
	 1.0, -1.0,  1.0,
	 1.0,  1.0,  1.0,
	-1.0,  1.0,  1.0,
	// back
	-1.0, -1.0, -1.0,
	 1.0, -1.0, -1.0,
	 1.0,  1.0, -1.0,
	-1.0,  1.0, -1.0,
};

std::vector<unsigned int> skyboxElements = {
	// right
	1, 5, 6,
	6, 2, 1,
	// left
	4, 0, 3,
	3, 7, 4,
	// top
	3, 2, 6,
	6, 7, 3,
	// bottom
	4, 5, 1,
	1, 0, 4,
	// front
	0, 1, 2,
	2, 3, 0,
	// back
	7, 6, 5,
	5, 4, 7,
};

class Skybox {
public:
	Skybox();

	void Draw(Shader & shader, glm::mat4 & view, glm::mat4 & projection);
	void LoadTexture(std::vector<std::string> & faces);

private:
	unsigned int vbo, vao, ebo, tid;

	void setup();
};

Skybox :: Skybox() { setup(); }

void Skybox :: setup() {

	glGenBuffers(1, &vbo); // Generate an empty vertex buffer on the GPU
	glGenBuffers(1, &ebo);
	glGenVertexArrays(1, &vao); // Tell OpenGL to create new Vertex Array Object
	
	glBindVertexArray(vao); // Make the vertices buffer the current one
	
	glBindBuffer(GL_ARRAY_BUFFER, vbo); // "bind" or set as the current buffer we are working with
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * skyboxVertices.size(),
		skyboxVertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * skyboxElements.size(),
		skyboxElements.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0); // vertex positions
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), NULL);

	glBindVertexArray(0); // Release control of vao
}

void Skybox :: Draw(Shader & shader, glm::mat4 & view, glm::mat4 & projection) {

	shader.use();
	shader.setUniform("uView", view);
	shader.setUniform("uProjection", projection);
	shader.setUniform("uSkybox", 0); // no other texture units

	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL); // change depth func so depth test passes when val == depth buffer

	glBindVertexArray(vao);
	glActiveTexture(GL_TEXTURE0 + texture_skybox_index);
	glBindTexture(GL_TEXTURE_CUBE_MAP, tid);
	glDrawElements(GL_TRIANGLES, skyboxElements.size(), GL_UNSIGNED_INT, 0);
	
	glBindVertexArray(0); // Unbind
	glActiveTexture(GL_TEXTURE0);
	glDepthFunc(GL_LESS); // restore default depth func
	glDepthMask(GL_TRUE);
}

void Skybox :: LoadTexture(std::vector<std::string> & faces) {
	tid = LoadCubemap(faces);
}

// Global Variables
const char* APP_TITLE = "Advanced OpenGL - CubeMaps";
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



	// Camera global
	float width_height_ratio = (float)gWindowWidth / (float)gWindowHeight;



	/** Load models */
	Cube objectCube1, objectCube3;
	TrCube objectCube2;
	Plane objectPlane;
	Skybox skybox;
	//Model objectSponzaModel("Resources/sponza/sponza.obj");
	Model objectCountryhouseModel("Resources/CountryHouse/house.obj");
	// Note: some textures of Nanosuit Model are transparent.
	// To see complete model, disable blending, change fragment shader or use a new shader
	Model objectNanosuit("Resources/nanosuit_reflection/nanosuit.obj");



	// Load textures manually
	objectCube1.AddTexture("Resources/default/container.jpg", TEX_DIFFUSE);
	objectCube1.AddTexture("Resources/default/container.jpg", TEX_SPECULAR);
	objectCube2.AddTexture("Resources/default/redwindow.png", TEX_DIFFUSE);
	objectCube2.AddTexture("Resources/default/redwindow.png", TEX_SPECULAR);

	std::vector<std::string> faces = {
		"Resources/skyboxes/lake/right.jpg",
		"Resources/skyboxes/lake/left.jpg",
		"Resources/skyboxes/lake/top.jpg",
		"Resources/skyboxes/lake/bottom.jpg",
		"Resources/skyboxes/lake/front.jpg",
		"Resources/skyboxes/lake/back.jpg",
	};
	skybox.LoadTexture(faces);



	// Shader loader
	Shader objectShader, skyboxShader, envMapShader, nanoShader;
	objectShader.loadShaders("shaders/cubemaps.vert", "shaders/cubemaps.frag");
	skyboxShader.loadShaders("shaders/skybox.vert",   "shaders/skybox.frag");
	envMapShader.loadShaders("shaders/envmap.vert", "shaders/envmap.frag");
	nanoShader.loadShaders("shaders/cubemaps.vert", "shaders/cubemaps_nanosuit.frag");



	// Light global
	glm::vec3 pointLightPos[] = {
		glm::vec3( 3.0f,  0.0f,  0.0f),
		glm::vec3(-3.0f,  0.0f,  0.0f),
		glm::vec3( 0.0f,  0.0f, -3.0f),
		glm::vec3( 0.0f,  0.0f,  3.0f)
	};
	glm::vec3 directionalLightDirection(1.0f, -1.0f, 1.0f);

	// Object shader config
	objectShader.use();
	// Directional light
	objectShader.setUniform("uDirectionalLight.direction", directionalLightDirection);
	objectShader.setUniform("uDirectionalLight.ambient", 0.1f, 0.1f, 0.1f);
	objectShader.setUniform("uDirectionalLight.diffuse", 1.0f, 1.0f, 1.0f);
	objectShader.setUniform("uDirectionalLight.specular", 1.0f, 1.0f, 1.0f);
	// Spot light
	objectShader.setUniform("uSpotLight.innerCutOff", glm::cos(glm::radians(12.5f)));
	objectShader.setUniform("uSpotLight.outerCutOff", glm::cos(glm::radians(17.5f)));
	objectShader.setUniform("uSpotLight.ambient", 0.0f, 0.0f, 0.0f);
	objectShader.setUniform("uSpotLight.diffuse", 1.0f, 1.0f, 1.0f);
	objectShader.setUniform("uSpotLight.specular", 1.0f, 1.0f, 1.0f);
	objectShader.setUniform("uSpotLight.constant", 1.0f);
	objectShader.setUniform("uSpotLight.linear", 0.09f);
	objectShader.setUniform("uSpotLight.quadratic", 0.032f);

	// Directional light
	nanoShader.use();
	nanoShader.setUniform("uDirectionalLight.direction", directionalLightDirection);
	nanoShader.setUniform("uDirectionalLight.ambient", 0.1f, 0.1f, 0.1f);
	nanoShader.setUniform("uDirectionalLight.diffuse", 1.0f, 1.0f, 1.0f);
	nanoShader.setUniform("uDirectionalLight.specular", 1.0f, 1.0f, 1.0f);
	// Spot light
	nanoShader.setUniform("uSpotLight.innerCutOff", glm::cos(glm::radians(12.5f)));
	nanoShader.setUniform("uSpotLight.outerCutOff", glm::cos(glm::radians(17.5f)));
	nanoShader.setUniform("uSpotLight.ambient", 0.0f, 0.0f, 0.0f);
	nanoShader.setUniform("uSpotLight.diffuse", 1.0f, 1.0f, 1.0f);
	nanoShader.setUniform("uSpotLight.specular", 1.0f, 1.0f, 1.0f);
	nanoShader.setUniform("uSpotLight.constant", 1.0f);
	nanoShader.setUniform("uSpotLight.linear", 0.09f);
	nanoShader.setUniform("uSpotLight.quadratic", 0.032f);
	


	// Skybox texture render unit index (it may change)
	nanoShader.use();
	nanoShader.setUniform("uSkybox", texture_skybox_index);
	envMapShader.use();
	envMapShader.setUniform("uSkybox", texture_skybox_index);



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
		glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



		// Camera transformations
		glm::mat4 view = camera.getViewMatrix();
		glm::mat4 projection = glm::perspective(glm::radians(camera.fov), width_height_ratio, 0.1f, 100.0f);
		//projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 100.0f);



		/** Set shader(s) and draw model(s) */

		// Object shader
		// Spot light
		objectShader.use();
		objectShader.setUniform("uSpotLight.position", camera.position);
		objectShader.setUniform("uSpotLight.direction", camera.front);
		objectShader.setUniform("uCameraPos", camera.position);
		objectShader.setUniform("uView", view);
		objectShader.setUniform("uProjection", projection);

		nanoShader.use();
		nanoShader.setUniform("uSpotLight.position", camera.position);
		nanoShader.setUniform("uSpotLight.direction", camera.front);
		nanoShader.setUniform("uCameraPos", camera.position);
		nanoShader.setUniform("uView", view);
		nanoShader.setUniform("uProjection", projection);

		// Environment mapping
		envMapShader.use();
		envMapShader.setUniform("uCameraPos", camera.position);
		envMapShader.setUniform("uView", view);
		envMapShader.setUniform("uProjection", projection);



		/** Obustacle objects */
		glm::mat4 modelMatrix;

		// Draw CountryHouse
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(5.0f, -5.0f, -10.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.001f, 0.001f, 0.001f));
		envMapShader.use();
		envMapShader.setUniform("uModel", modelMatrix);
		objectCountryhouseModel.Draw(envMapShader);

		// Cube
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
		modelMatrix = glm::rotate(modelMatrix, (float)glfwGetTime() * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		objectShader.use();
		objectShader.setUniform("uModel", modelMatrix);
		objectCube1.Draw(objectShader);

		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(2.0f, 0.0f, 0.0f));
		modelMatrix = glm::rotate(modelMatrix, (float)glfwGetTime() * glm::radians(10.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		envMapShader.use();
		envMapShader.setUniform("uModel", modelMatrix);
		objectCube3.Draw(envMapShader);

		// Nanosuit
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(-4.0f, -1.0f, 0.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.2f, 0.2f, 0.2f));
		nanoShader.use();
		nanoShader.setUniform("uModel", modelMatrix);
		objectNanosuit.Draw(nanoShader); // here, use a different shader or texture units overlap



		/** Skybox */
		view = glm::mat4(glm::mat3(camera.getViewMatrix())); // remove translation composition
		skybox.Draw(skyboxShader, view, projection);



		/** Transparent objects */
		// Cube
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(-2.0f, 0.0f, 0.0f));
		modelMatrix = glm::rotate(modelMatrix, (float) glfwGetTime() * glm::radians(20.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		objectShader.use();
		objectShader.setUniform("uModel", modelMatrix);
		objectCube2.UpdateRenderOrder(camera.position, modelMatrix);
		objectCube2.Draw(objectShader);



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
