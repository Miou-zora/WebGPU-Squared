#include "GL/gl3w.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"

const int WIDTH = 800;
const int HEIGHT = 800;

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

auto main(int ac, char **av) -> int
{
	/* Initialize the library */
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		return -1;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	/* Create a windowed mode window and its OpenGL context */
	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "OpenGL FrameWork", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	if (gl3wInit())
	{
		fprintf(stderr, "failed to initialize OpenGL\n");
		return -1;
	}
	if (!gl3wIsSupported(4, 2))
	{
		fprintf(stderr, "OpenGL 4.2 not supported\n");
		return -1;
	}
	glfwSwapInterval(1); // enable vsync

	glfwSetKeyCallback(window, key_callback);
	glfwSetWindowTitle(window, "MyOpenGLWindow");
	while (!glfwWindowShouldClose(window))
	{ //////////////////////////////
		/* Swap front and back buffers */
		glfwSwapBuffers(window);
		/* Poll for and process events */
		glfwPollEvents();
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}