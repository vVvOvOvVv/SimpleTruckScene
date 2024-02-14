// C++ related headers
#define _USE_MATH_DEFINES	// use pre-defined math constants
#include <cmath>
#include <iostream>
#include <math.h>
#include <string>
#include <vector>
#include <map>
#include <cstddef>
#include <cstdlib>
using namespace std;

// OpenGL related headers
#include <GLEW/glew.h>
#include <GLFW/glfw3.h>
#include <AntTweakBar.h>
#include "ShaderProgram.h"
#include <glm/fwd.hpp>
#include <glm/gtx/transform.hpp> 
using namespace glm;

// vertex attribute format - interleaved vertex
struct VertexColor {
	GLfloat pos[3],		// position - x,y,z
			color[3];	// color - r,g,b
};

// global variables
// settings
unsigned int gWindowWidth = 800, gWindowHeight = 800;

// scene content
ShaderProgram gShader;	// shader program object
GLuint gVBO = 0,		// vertex buffer object identifier
	   gVAO = 0;		// vertex array object identifier

// consts for wheels and tires
const float gWheelColor = 0.4f,		
			gWheelCenterColor = 0.8f,
		    gTireColor = 0.15f, 
			gTireRadius = 0.125f,
			gWheelRadius = 0.08f;  
// number of slices for wheels
const int gSlices = 50;
// initial centers for wheels - circles generated at 0, 0, 0 in init()
vec3 gFrontWheelCenter(-0.225f, -0.375f, 0.0f);
vec3 gBackWheelCenter(0.225f, -0.375f, 0.0f);

// frame stats
float gFrameRate = 60.0f, 
	  gFrameTime = 1 / gFrameRate;

// controls - wireframe and background color
bool gWireframe = false;	// switch between wireframe and fill
vec3 gBGColor(0.2f);
	
// transformation sensitives
const float gTranslateSensitivity = 0.1f,
			gRotateSensitivity = 0.1f,
			gWheelRotateSensitivity = 1.0f,
			gScaleSensitivity = 0.1f;

// transformation control via UI
float gGroundSlope = 0,	// slope of the ground
	  gTruckPos = 0,	// displacement of truck from center (x-axis)
	  gPrevSlope,		// to check if gGroundSlope changed via UI, not keyboard
	  gRotateWheelAngle = 0.0f; // to track how the wheel should be rotated

// obj model matric
map<string, mat4> gModelMatrix;		// store transformation matrices for the different obj


// generate vertices for circles - tires and wheels
	// no scale factor provided for circle to maintain circular wheels no matter window size
void generate_circle(const float radius,
					 const unsigned int slices,
					 float centerX, 
				     float centerY,
					 vector<GLfloat> &vertices,
					 bool tireWheel) {
	float slice_angle = M_PI * 2.0f / slices,		// angle of each slice
		  angle = 0,				// angle to generate x, y coord
		  x, y, z = 0.0f;			// x, y, z coord

	// generate coord
	for (int i = 0; i <= slices; i++) {
		// generate perimiter of circle
		x = radius * cos(angle) + centerX;
		y = radius * sin(angle) + centerY;

		// push back coord
		vertices.push_back(x);
		vertices.push_back(y);
		vertices.push_back(z);

		// update to next angle
		angle += slice_angle;

		// push back color
		if (tireWheel) { // true => tire, false => wheel
			vertices.push_back(gTireColor);
			vertices.push_back(gTireColor);
			vertices.push_back(gTireColor);
		} else {
			if (i <= (gSlices / 1.5) && i >= (gSlices / 2.5)) {
				// to help see rotation easier
				vertices.push_back(0.2f);
				vertices.push_back(0.2f);
				vertices.push_back(0.2f);
			} else {
				vertices.push_back(gWheelColor);
				vertices.push_back(gWheelColor);
				vertices.push_back(gWheelColor);
			}
		}
	}
}

// initialize scene and render settings
static void init(GLFWwindow* window) {
	// set the color the color buffer should be initially cleared to
	glClearColor(gBGColor.r, gBGColor.g, gBGColor.b, 1.0f);

	// set window
	string title = "Simple Truck Scene";
	glfwSetWindowTitle(window, title.c_str());

	// compile and link a vertex and fragment shader pair
	gShader.compileAndLink("colorTransform.vert", "color.frag");

	// initialize model matrices to identity matrices
	gModelMatrix["Ground"] = mat4(1.0f);
	gModelMatrix["Truck"] = mat4(1.0f);
	gModelMatrix["FrontWheel"] = mat4(1.0f);
	gModelMatrix["BackWheel"] = mat4(1.0f);

	// initial scene
	vector<GLfloat> vertices = {
		// ground ===================================
		-3.0f, -0.5f, 0.0f,		// top left - vertex
		0.0f, 0.4f, 0.0f,		// top left - color

		3.0f, -0.5f, 0.0f,		// top right - vertex
		0.0f, 0.4f, 0.0f,		// top right - color

		-3.0f, -3.0f, 0.0f,		// bot left - vertex
		0.0f, 0.6f, 0.0f,		// bot left - color

		3.0f, -3.0f, 0.0f,		// bot right - vertex
		0.0f, 0.6f, 0.0f,		// bot right - color
		// truck ====================================
		// driver compartment -----------------------
		-0.3f, -0.1f, 0.0f,		// top left - vertex
		0.0f, 1.0f, 0.0f,		// top left - color

		-0.1f, -0.1f, 0.0f,		// top right - vertex
		0.0f, 1.0f, 0.0f,		// top right - color

		-0.35f, -0.225f, 0.0f,	// mid left - vertex
		0.0f, 1.0f, 0.0f,		// mid left - color

		-0.1f, -0.225f, 0.0f,	// mid right - vertex
		1.0f, 0.0f, 0.0f,		// mid right - color

		-0.35f, -0.35f, 0.0f,	// bot left - vertex
		1.0f, 0.0f, 0.0f,		// bot left - color

		-0.1f, -0.35f, 0.0f,	// bot right - vertex
		1.0f, 0.0f, 0.0f,		// bot right - color
		// window -----------------------------------
		-0.3f, -0.12f, 0.0f,	// top left - vertex
		0.4f, 0.4f, 0.4f,		// top left - color

		-0.2f, -0.12f, 0.0f,	// top right - vertex
		0.1f, 0.1f, 0.1f,		// top left - color

		-0.34f, -0.225f, 0.0f,	// bot left - vertex
		0.1f, 0.1f, 0.1f,		// bot left - color

		-0.2f, -0.225f, 0.0f,	// bot right - vertex
		0.4f, 0.4f, 0.4f,		// bot right - color
		// back ------------------------------------
		-0.05f, -0.12f, 0.0f,	// top left - vertex
		1.0f, 0.0f, 0.0f,		// top left - color

		0.35f, -0.12f, 0.0f,	// top right - vertex
		1.0f, 0.0f, 0.0f,		// top right - color

		-0.1f, -0.235f, 0.0f,	// mid left - vertex
		1.0f, 0.0f, 0.0f,		// mid left - color

		0.4f, -0.235f, 0.0f,	// mid right - vertex
		0.0f, 0.0f, 1.0f,		// mid right - color

		-0.05f, -0.35f, 0.0f,	// bot left - vertex
		0.0f, 0.0f, 1.0f,		// bot left - color

		0.35f, -0.35f, 0.0f,	// bot right - vertex
		0.0f, 0.0f, 1.0f,		// bot right - color
		// base ------------------------------------
		-0.35f, -0.35f, 0.0f,	// top left - vertex
		0.6f, 0.6f, 0.6f,		// top left - color

		0.35f, -0.35f, 0.0f,	// top right - vertex
		0.6f, 0.6f, 0.6f,		// top left - color

		-0.35f, -0.4f, 0.0f,	// bot left - vertex
		0.2f, 0.2f, 0.2f,		// bot left - color

		0.35f, -0.4f, 0.0f,		// bot right - vertex
		0.2f, 0.2f, 0.2f,		// bot left - color
	};
	// wheels ========================================
	// wheels will be set to center first
	// front tire ------------------------------------
	vertices.push_back(0.0f);			// x - front tire
	vertices.push_back(0.0f);			// y - front tire
	vertices.push_back(0.0f);			// z - front tire
	vertices.push_back(gTireColor);		// r - front tire
	vertices.push_back(gTireColor);		// g - front tire
	vertices.push_back(gTireColor);		// b - front tire
	generate_circle(gTireRadius, gSlices, 0.0f, 0.0f, vertices, true);
	// front wheel -----------------------------------
	vertices.push_back(0.0f);				// x - front tire
	vertices.push_back(0.0f);				// y - front tire
	vertices.push_back(0.0f);				// z - front tire
	vertices.push_back(gWheelCenterColor);	// r - front tire
	vertices.push_back(gWheelCenterColor);	// g - front tire
	vertices.push_back(gWheelCenterColor);	// b - front tire
	generate_circle(gWheelRadius, gSlices, 0.0f, 0.0f, vertices, false);
	// back tire -------------------------------------
	vertices.push_back(0.0f);				// x - front tire
	vertices.push_back(0.0f);				// y - front tire
	vertices.push_back(0.0f);				// z - front tire
	vertices.push_back(gTireColor);			// r - front tire
	vertices.push_back(gTireColor);			// g - front tire
	vertices.push_back(gTireColor);			// b - front tire
	generate_circle(gTireRadius, gSlices, 0.0f, 0.0f, vertices, true);
	// back wheel ------------------------------------
	vertices.push_back(0.0f);				// x - front tire
	vertices.push_back(0.0f);				// y - front tire
	vertices.push_back(0.0f);				// z - front tire
	vertices.push_back(gWheelCenterColor);	// r - front tire
	vertices.push_back(gWheelCenterColor);	// g - front tire
	vertices.push_back(gWheelCenterColor);	// b - front tire
	generate_circle(gWheelRadius, gSlices, 0.0f, 0.0f, vertices, false);

	// create VBO and buffer the data
	glGenBuffers(1, &gVBO);					// generate unused VBO identifier
	glBindBuffer(GL_ARRAY_BUFFER, gVBO);	// bind the VBO
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)* vertices.size(), &vertices[0], GL_DYNAMIC_DRAW);

	// create VAO, specify VBO data and format of the data
	glGenVertexArrays(1, &gVAO);			// generate unused VAO identifier
	glBindVertexArray(gVAO);				// create VAO
	glBindBuffer(GL_ARRAY_BUFFER, gVBO);	// bind the VBO
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexColor),
		reinterpret_cast<void*>(offsetof(VertexColor, pos)));	// specify format of position data
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexColor),
		reinterpret_cast<void*>(offsetof(VertexColor, color)));		// specify format of colour data

	glEnableVertexAttribArray(0);	// enable vertex attributes
	glEnableVertexAttribArray(1);
}

// update scene
static void update_scene(GLFWwindow* window) {
	// update background color
	glClearColor(gBGColor.r, gBGColor.g, gBGColor.b, 1.0f);

	// declare variables to transform objects
	vec3 moveTruckVec(0.0f);
	float rotateAngle = 0.0f;

	// update variables based on keyboard input ============
	// left, right arrows - move truck, rotate wheels
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
		moveTruckVec.x -= gTranslateSensitivity * gFrameTime;
		gTruckPos += moveTruckVec.x;
		gRotateWheelAngle += gWheelRotateSensitivity * gFrameTime;
	}
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
		moveTruckVec.x += gTranslateSensitivity * gFrameTime;
		gTruckPos += moveTruckVec.x;
		gRotateWheelAngle -= gWheelRotateSensitivity * gFrameTime;
	}
	// up, down arrows - tilt ground slope
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
		if (gGroundSlope < 15.0f) {
			rotateAngle -= gRotateSensitivity * gFrameTime;
			gGroundSlope -= degrees(rotateAngle);
			gPrevSlope = gGroundSlope;
		}
	}
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
		if (gGroundSlope > -15.0f) {
			rotateAngle += gRotateSensitivity * gFrameTime;
			gGroundSlope -= degrees(gRotateSensitivity * gFrameTime);
			gPrevSlope = gGroundSlope;
		}
	}
	// check if slope changed from UI interaction
	if (gGroundSlope != gPrevSlope) {
		rotateAngle += radians(gPrevSlope - gGroundSlope);
		// note: if we just take gGroundSlope, the obj will keep spinning
	}

	// update model matrices
	gModelMatrix["Ground"] *= translate(vec3(1.0f, -0.5f, 0.0f))
		* rotate(rotateAngle, vec3(0.0f, 0.0f, 1.0f))
		* translate(vec3(-1.0f, 0.5f, 0.0f));
	gModelMatrix["Truck"] *= translate(moveTruckVec)
		* translate(vec3(1.0f - gTruckPos, -0.5f, 0.0f))
		* rotate(rotateAngle, vec3(0.0f, 0.0f, 1.0f))
		* translate(vec3(-1.0f + gTruckPos, 0.5f, 0.0f));
	gModelMatrix["FrontWheel"] = gModelMatrix["Truck"]
		* translate(gFrontWheelCenter)
		* rotate(gRotateWheelAngle, vec3(0.0f, 0.0f, 1.0f));
	gModelMatrix["BackWheel"] = gModelMatrix["Truck"]
		* translate(gBackWheelCenter)
		* rotate(gRotateWheelAngle, vec3(0.0f, 0.0f, 1.0f));

	// update gPrevSlope
	gPrevSlope = gGroundSlope;
}

// create and populate tweak bar elements
static TwBar* create_UI(const string name = "Interface") {
	TwBar* twBar = TwNewBar(name.c_str());

	TwWindowSize(gWindowWidth, gWindowHeight);
	TwDefine(" TW_HELP visible=false "); // disable help menu
	TwDefine(" GLOBAL fontsize=3 ");	 //set large font size
	TwDefine(" Main label='User Interface' refresh=0.02 text=light size='250 450' position='10 10' ");

	// frame stat entries
	TwAddVarRO(twBar, "Frame Rate", TW_TYPE_FLOAT,
			   &gFrameRate, " group='Frame Stats' precision=2 ");
	TwAddVarRO(twBar, "Frame Time", TW_TYPE_FLOAT,
			   &gFrameTime, " group='Frame Stats' ");

	// display controls
	TwAddVarRW(twBar, "Wireframe", TW_TYPE_BOOLCPP, &gWireframe, " group='Display' ");

	// background color control
	TwAddVarRW(twBar, "Background", TW_TYPE_COLOR3F, &gBGColor,
			   " label='Background' opened=true ");

	// truck movement
	// allow user to use UI to change slope
	TwAddVarRW(twBar, "Ground Slope", TW_TYPE_FLOAT, &gGroundSlope,
			   " group='Controls' min=-15.0 max=15.0 step=0.1");
	// simply show x-coord of truck's center
	TwAddVarRO(twBar, "Position", TW_TYPE_FLOAT, &gTruckPos,
			   " group='Controls' min=-1.00 max=1.00 step=0.01");

	return twBar;
}

// function to render the scene
static void render_scene() {
	// clear color buffer
	glClear(GL_COLOR_BUFFER_BIT);

	gShader.use();						// use the shaders associated with the shader program

	glBindVertexArray(gVAO);			// make VAO active

	gShader.setUniform("uModelMatrix", gModelMatrix["Ground"]);	// set model matrix
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);	// draw ground

	gShader.setUniform("uModelMatrix", gModelMatrix["Truck"]);	// set model matrix
	glDrawArrays(GL_TRIANGLE_STRIP, 4, 6);	// draw driver compartment
	glDrawArrays(GL_TRIANGLE_STRIP, 10, 4); // draw window
	glDrawArrays(GL_TRIANGLE_STRIP, 14, 6);	// draw truck back
	glDrawArrays(GL_TRIANGLE_STRIP, 20, 4); // draw base

	gShader.setUniform("uModelMatrix", gModelMatrix["FrontWheel"]);	// set model matrix
	glDrawArrays(GL_TRIANGLE_FAN, 24, gSlices + 2);					// draw front tire
	glDrawArrays(GL_TRIANGLE_FAN, 24 + gSlices + 2, gSlices + 2);	// draw front wheel

	gShader.setUniform("uModelMatrix", gModelMatrix["BackWheel"]);	// set model matrix
	glDrawArrays(GL_TRIANGLE_FAN, 24 + (2 * (gSlices + 2)), gSlices + 2); // draw back tire
	glDrawArrays(GL_TRIANGLE_FAN, 24 + (3 * (gSlices + 2)), gSlices + 2); // draw back wheel

	// flush the graphics pipeline
	glFlush();
}

// mouse movement callback function
static void cursor_position_callback(GLFWwindow* window, 
									 double xpos, double ypos) {
	// pass cursor position to tweak bar
	TwEventMousePosGLFW(static_cast<int>(xpos), static_cast<int>(ypos));
}

// mouse button callback function
static void mouse_button_callback(GLFWwindow* window, 
								  int button, int action, int mods) {
	// pass mouse button status to tweak bar
	TwEventMouseButtonGLFW(button, action);
}

// key press or release callback function
static void key_callback(GLFWwindow* window, int key,
						 int scancode, int action, int mods) {
	// close the window when the ESCAPE key is pressed
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		// set flag to close the window
		glfwSetWindowShouldClose(window, GL_TRUE);
		return;
	}
}

// error callback function
static void error_callback(int error, const char* description)
{
	cerr << description << endl;	// output error description
}

int main(void) {
	GLFWwindow* window = nullptr;	// GLFW window handle

	glfwSetErrorCallback(error_callback);	// set GLFW error callback function

	// initialise GLFW
	if (!glfwInit())
	{
		// if failed to initialise GLFW
		exit(EXIT_FAILURE);
	}

	// minimum OpenGL version 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// create a window and its OpenGL context
	window = glfwCreateWindow(gWindowWidth, gWindowHeight, "A1", nullptr, nullptr);

	// check if window created successfully
	if (window == nullptr)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);	// set window context as the current context
	glfwSwapInterval(1);			// swap buffer interval

	// initialise GLEW
	if (glewInit() != GLEW_OK)
	{
		// if failed to initialise GLEW
		cerr << "GLEW initialisation failed" << endl;
		exit(EXIT_FAILURE);
	}

	// set GLFW callback functions
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	// avoid missing keyboard input
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// initialise scene and render settings
	init(window);

	// initialise AntTweakBar
	TwInit(TW_OPENGL_CORE, nullptr);
	TwBar* tweakBar = create_UI("Interface");	// create and populate tweak bar elements

	// timing data
	double lastUpdateTime = glfwGetTime();	// last update time
	double elapsedTime = lastUpdateTime;	// time since last update
	int frameCount = 0;						// number of frames since last update

	// the rendering loop
	while (!glfwWindowShouldClose(window))
	{
		update_scene(window);		// update scene (translations, rotation, etc.)

		if (gWireframe)		// update render mode
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		render_scene();		// render the scene

		// prevent UI from rendering as wireframes
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		TwDraw();			// draw tweak bar

		glfwSwapBuffers(window);	// swap buffers
		glfwPollEvents();			// poll for events

		frameCount++;
		elapsedTime = glfwGetTime() - lastUpdateTime;	// time since last update

		// if elapsed time since last update > 1 second
		if (elapsedTime > 1.0)
		{
			gFrameTime = elapsedTime / frameCount;	// average time per frame
			gFrameRate = 1 / gFrameTime;			// frames per second
			lastUpdateTime = glfwGetTime();			// set last update time to current time
			frameCount = 0;							// reset frame counter
		}
	}

	// clean up
	glDeleteBuffers(1, &gVBO);
	glDeleteVertexArrays(1, &gVAO);

	// terminate tweak bar
	TwDeleteBar(tweakBar);
	TwTerminate();
	// close the window and terminate GLFW
	glfwDestroyWindow(window);
	glfwTerminate();

	exit(EXIT_SUCCESS);
}
