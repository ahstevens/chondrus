#pragma once
#include <vector>
#include <algorithm>
#include <iostream>

#include "GLFWInputBroadcaster.h"


GLFWInputBroadcaster::GLFWInputBroadcaster()
{
}

GLFWInputBroadcaster& GLFWInputBroadcaster::getInstance()
{
	static GLFWInputBroadcaster instance;
	return instance;
}

void GLFWInputBroadcaster::init(GLFWwindow * window)
{
	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, mouse_position_callback);
	glfwSetScrollCallback(window, scroll_callback);

	memset(keys, 0, sizeof keys);
	firstMouse = true;
	lastX = 0;
	lastY = 0;
}

bool GLFWInputBroadcaster::keyPressed(const int glfwKeyCode)
{
	return keys[glfwKeyCode];
}

void GLFWInputBroadcaster::poll()
{
	glfwPollEvents();
}

// Is called whenever a key is pressed/released via GLFW
void GLFWInputBroadcaster::key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{	
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
		return;
	}

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
		{
			getInstance().keys[key] = true;
			getInstance().notify(NULL, BroadcastSystem::EVENT::KEY_PRESS, &key);
		}
		else if (action == GLFW_RELEASE)
		{
			getInstance().keys[key] = false;
			getInstance().notify(NULL, BroadcastSystem::EVENT::KEY_UNPRESS, &key);
		}
	}
}

void GLFWInputBroadcaster::mouse_button_callback(GLFWwindow * window, int button, int action, int mods)
{
	if (action == GLFW_PRESS)
		getInstance().notify(NULL, BroadcastSystem::EVENT::MOUSE_CLICK, &button);
	else if (action == GLFW_RELEASE)
		getInstance().notify(NULL, BroadcastSystem::EVENT::MOUSE_UNCLICK, &button);
}

void GLFWInputBroadcaster::mouse_position_callback(GLFWwindow * window, double xpos, double ypos)
{
	if (getInstance().firstMouse)
	{
		getInstance().lastX = static_cast<GLfloat>(xpos);
		getInstance().lastY = static_cast<GLfloat>(ypos);
		getInstance().firstMouse = false;
	}

	GLfloat xoffset = static_cast<GLfloat>(xpos) - getInstance().lastX;
	GLfloat yoffset = getInstance().lastY - static_cast<GLfloat>(ypos);  // Reversed since y-coordinates go from bottom to left

	getInstance().lastX = static_cast<GLfloat>(xpos);
	getInstance().lastY = static_cast<GLfloat>(ypos);

	float offset[2] = { static_cast<float>(xoffset), static_cast<float>(yoffset) };

	getInstance().notify(NULL, BroadcastSystem::EVENT::MOUSE_MOVE, &offset);
}

void GLFWInputBroadcaster::scroll_callback(GLFWwindow * window, double xoffset, double yoffset)
{
	float offset = static_cast<float>(yoffset);
	getInstance().notify(NULL, BroadcastSystem::EVENT::MOUSE_SCROLL, &offset);
}
