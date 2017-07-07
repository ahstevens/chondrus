// System Headers
#define GLEW_STATIC      // use static GLEW libs
#include <GL/glew.h>     // include before GLFW (gl.h)
#include <GLFW/glfw3.h>

// Standard Headers
#include <cstdio>
#include <cstdlib>
#include <random>
#include <ctime> // for time()

// Our classes
#include <Engine.h>

std::default_random_engine generator;

// Engine
Engine *engine;

int main(int argc, char * argv[]) 
{
	srand(time(NULL)); // Seed rand with time

    // Load GLFW and Create a Window
    glfwInit();

	engine = new Engine();

	if(!engine->init())
	{
		fprintf(stderr, "Failed to Create OpenGL Context");
		return EXIT_FAILURE;
	}

	engine->mainLoop();

	//glfwTerminate();

    return EXIT_SUCCESS;
}