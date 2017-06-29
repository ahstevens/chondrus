#pragma once

// Std. Includes
#include <vector>

// GL Includes
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glSkel/Object.h>
#include <glSkel/BroadcastSystem.h>
#include "GLFWInputBroadcaster.h"

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};
	
// Default camera values
const glm::vec3 m_vec3DefaultPosition =  glm::vec3(0.f, 0.f, 0.f);
const glm::vec3 m_vec3DefaultUp       =  glm::vec3(0.f, 1.f, 0.f);
const float m_fDefaultYaw             = -90.f;
const float m_fDefaultPitch           =   0.f;
const float m_fDefaultSpeed           =  30.f;
const float m_fDefaultSensitivity     =   0.25f;
const float m_fDefaultZoom            =  45.f;
const float m_fDefaultZoomMin         =  45.f;
const float m_fDefaultZoomMax         =   1.f; 
const float m_fNearClip				  =   0.1f;
const float m_fFarClip				  =  50.f;

// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera : public Object, public BroadcastSystem::Listener
{
public:
    // Constructor with vectors
    Camera(
		glm::vec3 position = m_vec3DefaultPosition,
		glm::vec3 up = m_vec3DefaultUp,
		float yaw = m_fDefaultYaw,
		float pitch = m_fDefaultPitch
	) 
		: m_fMovementSpeed(m_fDefaultSpeed)
		, m_fSensitivity(m_fDefaultSensitivity)
		, m_fZoom(m_fDefaultZoom)
		, m_fZoomMin(m_fDefaultZoomMin)
		, m_fZoomMax(m_fDefaultZoomMax)
    {
        m_vec3Position = position;
		m_vec3WorldUp = up;
        m_fYaw = yaw;
        m_fPitch = pitch;
        updateCameraVectors();
		memset(m_brMovementState, 0, sizeof(m_brMovementState));
    }

    // Returns the view matrix calculated using Eular Angles and the LookAt Matrix
    glm::mat4 getViewMatrix()
    {
        return glm::lookAt(m_vec3Position, m_vec3Position + m_mat3Rotation[2], m_mat3Rotation[1]);
    }

	// Returns the view matrix calculated using Eular Angles and the LookAt Matrix
	glm::mat4 getProjectionMatrix(float aspect_ratio)
	{
		return glm::perspective(glm::radians(getZoom()), aspect_ratio, m_fNearClip, m_fFarClip);
	}

	float getNearPlane()
	{
		return m_fNearClip;
	}

	float getFarPlane()
	{
		return m_fFarClip;
	}

	float getZoom() 
	{ 
		return m_fZoom; 
	}

	void receiveEvent(Object * obj, const int event, void * data)
	{
		if (event == BroadcastSystem::EVENT::KEY_PRESS)
		{
			int key;
			memcpy(&key, data, sizeof(key));

			// Camera controls
			if (key == GLFW_KEY_W)
				m_brMovementState[FORWARD] = true;
			if (key == GLFW_KEY_S)
				m_brMovementState[BACKWARD] = true;
			if (key == GLFW_KEY_A)
				m_brMovementState[LEFT] = true;
			if (key == GLFW_KEY_D)
				m_brMovementState[RIGHT] = true;

			if (key == GLFW_KEY_LEFT_SHIFT)
				m_fMovementSpeed *= 5.f;
		}

		if (event == BroadcastSystem::EVENT::KEY_UNPRESS)
		{
			int key;
			memcpy(&key, data, sizeof(key));

			// Camera controls
			if (key == GLFW_KEY_W)
				m_brMovementState[FORWARD] = false;
			if (key == GLFW_KEY_S)
				m_brMovementState[BACKWARD] = false;
			if (key == GLFW_KEY_A)
				m_brMovementState[LEFT] = false;
			if (key == GLFW_KEY_D)
				m_brMovementState[RIGHT] = false;

			if (key == GLFW_KEY_LEFT_SHIFT)
				m_fMovementSpeed *= 0.2f;
		}

		if (event == BroadcastSystem::EVENT::MOUSE_MOVE)
		{
			float offset[2];
			memcpy(offset, data, sizeof(offset)); // recover array
			look(offset[0], offset[1]);
		}

		if (event == BroadcastSystem::EVENT::MOUSE_SCROLL)
		{
			float yoffset;
			memcpy(&yoffset, data, sizeof(yoffset));
			zoom( yoffset );
		}
	}

	void update(float deltaTime)
	{		
		// Move the camera based on its current movement state
		move(deltaTime);

        // Update Front, Right and Up Vectors using the updated Eular angles
        this->updateCameraVectors();
	}

private:
	// Camera Attributes
	glm::vec3 m_vec3WorldUp;

	// Eular Angles
	float m_fYaw;
	float m_fPitch;

	// Camera options
	float m_fMovementSpeed;
	float m_fSensitivity;
	float m_fZoom, m_fZoomMin, m_fZoomMax;

	bool m_brMovementState[4]; // FORWARD, BACKWARD, LEFT, RIGHT
	

	// Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
	void move(float deltaTime)
	{
		float velocity = m_fMovementSpeed * deltaTime;
		if (m_brMovementState[FORWARD])
			m_vec3Position += m_mat3Rotation[2] * velocity;
		if (m_brMovementState[BACKWARD])
			m_vec3Position -= m_mat3Rotation[2] * velocity;
		if (m_brMovementState[LEFT])
			m_vec3Position -= m_mat3Rotation[0] * velocity;
		if (m_brMovementState[RIGHT])
			m_vec3Position += m_mat3Rotation[0] * velocity;
	}

	// Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
	void look(float dx, float dy, GLboolean constrainPitch = true)
	{
		dx *= m_fSensitivity;
		dy *= m_fSensitivity;

		m_fYaw += dx;
		m_fPitch += dy;

		// Make sure that when pitch is out of bounds, screen doesn't get flipped
		if (constrainPitch)
		{
			if (m_fPitch > 89.f)
				m_fPitch = 89.f;
			if (m_fPitch < -89.f)
				m_fPitch = -89.f;
		}
	}

	// Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
	void zoom(float dz)
	{
		if (m_fZoom >= m_fZoomMax && m_fZoom <= m_fZoomMin)
			m_fZoom -= dz;
		if (m_fZoom <= m_fZoomMax)
			m_fZoom = m_fZoomMax;
		if (m_fZoom >= m_fZoomMin)
			m_fZoom = m_fZoomMin;
	}

    // Calculates the front vector from the Camera's (updated) Eular Angles
    void updateCameraVectors()
    {
        // Calculate the new Front vector
        glm::vec3 front;
        front.x = cos(glm::radians(m_fYaw)) * cos(glm::radians(m_fPitch));
        front.y = sin(glm::radians(m_fPitch));
        front.z = sin(glm::radians(m_fYaw)) * cos(glm::radians(m_fPitch));
		m_mat3Rotation[2] = glm::normalize(front);

        // Also re-calculate the Right and Up vector
		m_mat3Rotation[0] = glm::normalize(glm::cross(m_mat3Rotation[2], m_vec3WorldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
		m_mat3Rotation[1] = glm::normalize(glm::cross(m_mat3Rotation[0], m_mat3Rotation[2]));
    }
};
