#pragma once

#include <glm/glm.hpp>

class Object
{
public:
	Object()
		: m_vec3Position(glm::vec3())
		, m_mat3Rotation(glm::mat3())
		, m_vec3Scale(glm::vec3(1.f))
	{}

	Object(glm::vec3 position, glm::mat3 rotation, glm::vec3 scale = glm::vec3(1.f))
		: m_vec3Position(position)
		, m_mat3Rotation(rotation)
		, m_vec3Scale(scale)
	{}

	~Object() {}

	void setPosition(glm::vec3 pos) { m_vec3Position = pos; }
	glm::vec3 getPosition() { return m_vec3Position; }

	void setOrientation(glm::mat3 rot) { m_mat3Rotation = rot; }
	glm::mat3 getOrientation() { return m_mat3Rotation; }

	void setScale(glm::vec3 s) { m_vec3Scale = s; }
	void setScale(float s) { m_vec3Scale = glm::vec3(s); }
	glm::vec3 getScale() { return m_vec3Scale; }

protected:
	glm::vec3 m_vec3Position;
	glm::mat3 m_mat3Rotation;
	glm::vec3 m_vec3Scale;
};

