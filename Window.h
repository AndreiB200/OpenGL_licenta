#include <GLFW/glfw3.h>
#include <iostream>
#include "Camera.h"


float lastX;
float lastY;
bool firstMouse = true;

float xoffset = 0.0f;
float yoffset = 0.0f;

Camera* mycamera;

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	xoffset = xpos - lastX;
	yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	mycamera->ProcessMouseMovement(xoffset, yoffset);
}

void framebuffer_size(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

class Window
{
public: 
	GLFWwindow* window = nullptr;

	void bindCamera(Camera* _camera)
	{
		mycamera = _camera;
	}

	Window(int WIDTH, int HEIGHT, const char *title)
	{
		this->WIDTH = WIDTH;
		this->HEIGHT = HEIGHT;

		window = glfwCreateWindow(WIDTH, HEIGHT, title, NULL, NULL);
		windowCheck(window);
		glfwSetWindowPos(window, 100, 100);
		glfwMakeContextCurrent(window);
		glfwSetFramebufferSizeCallback(window, framebuffer_size);

		glfwSetCursorPosCallback(window, mouse_callback);
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		lastX = WIDTH / 2.0f;
		lastY = HEIGHT / 2.0f;
	}

	void cursorActivate()
	{
		if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			glfwSetCursorPosCallback(window, NULL);
			firstMouse = true;
		}
		else
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			glfwSetCursorPosCallback(window, mouse_callback);
		}
	}

private:
	int WIDTH;
	int HEIGHT;

	int windowCheck(GLFWwindow* window)
	{
		if (window == NULL)
		{
			std::cout << "Failed to create GLFW window" << std::endl;
			glfwTerminate();
			return -1;
		}
		return 0;
	}
};