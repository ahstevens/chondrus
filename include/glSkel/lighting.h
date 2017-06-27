#ifndef LIGHTING_H
#define LIGHTING_H

#include <GL/glew.h>

#include <glSkel/BroadcastSystem.h>
#include <glSkel/shader.h>

struct BasicLight {	
	GLboolean on;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
};

struct DLight : BasicLight {
	glm::vec3 direction;
};

struct PLight : BasicLight {
	glm::vec3 position;
	GLfloat constant;
	GLfloat linear;
	GLfloat quadratic;
};

struct SLight : PLight {
	glm::vec3 direction;
	GLfloat cutOff;
	GLfloat outerCutOff;
};


#define MAX_N_PLIGHTS 4


class LightingSystem : public BroadcastSystem::Listener
{
public:
	DLight dLight;
	std::vector<PLight> pLights;
	SLight sLight;

    LightingSystem() : meshInitiated(false)
    {

    }

    // Uses the current shader
    void SetupLighting(Shader s) 
	{
		// Directional light
		if (dLight.on)
		{
			glUniform3f(glGetUniformLocation(s.Program, "dirLight.direction"), dLight.direction.x, dLight.direction.y, dLight.direction.z);
			glUniform3f(glGetUniformLocation(s.Program, "dirLight.ambient"), dLight.ambient.x, dLight.ambient.y, dLight.ambient.z);
			glUniform3f(glGetUniformLocation(s.Program, "dirLight.diffuse"), dLight.diffuse.x, dLight.diffuse.y, dLight.diffuse.z);
			glUniform3f(glGetUniformLocation(s.Program, "dirLight.specular"), dLight.specular.x, dLight.specular.y, dLight.specular.z);
		}
		else
		{
			glUniform3f(glGetUniformLocation(s.Program, "dirLight.ambient"), 0.f, 0.f, 0.f);
			glUniform3f(glGetUniformLocation(s.Program, "dirLight.diffuse"), 0.f, 0.f, 0.f);
			glUniform3f(glGetUniformLocation(s.Program, "dirLight.specular"), 0.f, 0.f, 0.f);
		}

		// Point light
		for (int i = 0; i < pLights.size(); ++i)
		{
			std::string name = "pointLights[" + std::to_string(i);
			name += "]";

			if (pLights[i].on)
			{
				glUniform3f(glGetUniformLocation(s.Program, (name + ".position").c_str()), pLights[i].position.x, pLights[i].position.y, pLights[i].position.z);
				glUniform3f(glGetUniformLocation(s.Program, (name + ".ambient").c_str()), pLights[i].ambient.r, pLights[i].ambient.g, pLights[i].ambient.b);
				glUniform3f(glGetUniformLocation(s.Program, (name + ".diffuse").c_str()), pLights[i].diffuse.r, pLights[i].diffuse.g, pLights[i].diffuse.b);
				glUniform3f(glGetUniformLocation(s.Program, (name + ".specular").c_str()), pLights[i].specular.r, pLights[i].specular.g, pLights[i].specular.b);
				glUniform1f(glGetUniformLocation(s.Program, (name + ".constant").c_str()), pLights[i].constant);
				glUniform1f(glGetUniformLocation(s.Program, (name + ".linear").c_str()), pLights[i].linear);
				glUniform1f(glGetUniformLocation(s.Program, (name + ".quadratic").c_str()), pLights[i].quadratic);
			}
			else
			{
				glUniform3f(glGetUniformLocation(s.Program, (name + ".ambient").c_str()), 0.f, 0.f, 0.f);
				glUniform3f(glGetUniformLocation(s.Program, (name + ".diffuse").c_str()), 0.f, 0.f, 0.f);
				glUniform3f(glGetUniformLocation(s.Program, (name + ".specular").c_str()), 0.f, 0.f, 0.f);
			}
		}

		// SpotLight
		if (sLight.on)
		{
			glUniform3f(glGetUniformLocation(s.Program, "spotLight.position"), sLight.position.x, sLight.position.y, sLight.position.z);
			glUniform3f(glGetUniformLocation(s.Program, "spotLight.direction"), sLight.direction.x, sLight.direction.y, sLight.direction.z);
			glUniform3f(glGetUniformLocation(s.Program, "spotLight.ambient"), sLight.ambient.r, sLight.ambient.g, sLight.ambient.b);
			glUniform3f(glGetUniformLocation(s.Program, "spotLight.diffuse"), sLight.diffuse.r, sLight.diffuse.g, sLight.diffuse.b);
			glUniform3f(glGetUniformLocation(s.Program, "spotLight.specular"), sLight.specular.r, sLight.specular.g, sLight.specular.b);
			glUniform1f(glGetUniformLocation(s.Program, "spotLight.constant"), sLight.constant);
			glUniform1f(glGetUniformLocation(s.Program, "spotLight.linear"), sLight.linear);
			glUniform1f(glGetUniformLocation(s.Program, "spotLight.quadratic"), sLight.quadratic);
			glUniform1f(glGetUniformLocation(s.Program, "spotLight.cutOff"), sLight.cutOff);
			glUniform1f(glGetUniformLocation(s.Program, "spotLight.outerCutOff"), sLight.outerCutOff);
		}
		else
		{
			glUniform3f(glGetUniformLocation(s.Program, "spotLight.ambient"), 0.f, 0.f, 0.f);
			glUniform3f(glGetUniformLocation(s.Program, "spotLight.diffuse"), 0.f, 0.f, 0.f);
			glUniform3f(glGetUniformLocation(s.Program, "spotLight.specular"), 0.f, 0.f, 0.f);
		}
	}

	bool addDLight(glm::vec3 direction = glm::vec3(-1.0f),
		glm::vec3 ambient = glm::vec3(0.1f), glm::vec3 diffuse = glm::vec3(1.f), glm::vec3 specular = glm::vec3(1.f))
	{
		this->dLight.direction = direction;
		this->dLight.ambient = ambient;
		this->dLight.diffuse = diffuse;
		this->dLight.specular = specular;

		this->dLight.on = true;

		return true;
	}

	bool addPLight(glm::vec3 position = glm::vec3(1.0f),
		glm::vec3 ambient = glm::vec3(0.05f), glm::vec3 diffuse = glm::vec3(0.8f), glm::vec3 specular = glm::vec3(1.0f),
		GLfloat constant = 1.0f, GLfloat linear = 0.09f, GLfloat quadratic = 0.032f)
	{
		if (pLights.size() == MAX_N_PLIGHTS) return false;

		PLight pl;
		pl.position = position;
		pl.ambient = ambient;
		pl.diffuse = diffuse;
		pl.specular = specular;
		pl.constant = constant;
		pl.linear = linear;
		pl.quadratic = quadratic;

		pl.on = true;

		pLights.push_back(pl);

		return true;
	}

	bool addSLight(glm::vec3 position = glm::vec3(1.0f), glm::vec3 direction = glm::vec3(0.0f),
		glm::vec3 ambient = glm::vec3(0.0f), glm::vec3 diffuse = glm::vec3(1.0f), glm::vec3 specular = glm::vec3(1.0f),
		GLfloat constant = 1.0f, GLfloat linear = 0.09f, GLfloat quadratic = 0.032f,
		GLfloat cutOffDeg = 12.5f, GLfloat outerCutOffDeg = 15.0f)
	{
		this->sLight.position = position;
		this->sLight.direction = direction;
		this->sLight.ambient = ambient;
		this->sLight.diffuse = diffuse;
		this->sLight.specular = specular;
		this->sLight.constant = constant;
		this->sLight.linear = linear;
		this->sLight.quadratic = quadratic;
		this->sLight.cutOff = glm::cos(glm::radians(cutOffDeg));
		this->sLight.outerCutOff = glm::cos(glm::radians(outerCutOffDeg));

		this->sLight.on = true;

		return true;
	}

	void Draw(Shader s)
	{
		if(!meshInitiated)
			this->setupLightMesh();

		glm::mat4 model;

		glBindVertexArray(this->VAO);
		for (GLuint i = 0; i < pLights.size(); ++i)
		{
			if(pLights[i].on)
				glUniform3f(glGetUniformLocation(s.Program, "col"), pLights[i].specular.r, pLights[i].specular.g, pLights[i].specular.b);
			else
				glUniform3f(glGetUniformLocation(s.Program, "col"), 0.f, 0.f, 0.f);

			model = glm::mat4();
			model = glm::translate(model, pLights[i].position);
			model = glm::scale(model, glm::vec3(0.2f)); // Make it a smaller cube
			glUniformMatrix4fv(glGetUniformLocation(s.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
			glDrawElements(GL_TRIANGLES, this->nIndices, GL_UNSIGNED_INT, 0);
		}
		glBindVertexArray(0);

	}

	void receiveEvent(Object * obj, const int event, void * data)
	{
		if (event == BroadcastSystem::EVENT::KEY_PRESS)
		{
			int key;
			memcpy(&key, data, sizeof(key));

			if (key == GLFW_KEY_1)
				dLight.on = !dLight.on;
			if (key == GLFW_KEY_2)
				pLights[0].on = !pLights[0].on;
			if (key == GLFW_KEY_3)
				pLights[1].on = !pLights[1].on;
			if (key == GLFW_KEY_4)
				pLights[2].on = !pLights[2].on;
			if (key == GLFW_KEY_5)
				pLights[3].on = !pLights[3].on;
			if (key == GLFW_KEY_6)
				sLight.on = !sLight.on;
		}
	}

private:
	GLuint VBO, VAO, EBO;
	GLsizei nIndices;
	GLboolean meshInitiated;

    void setupLightMesh()
	{
		// Construct light geometry
		std::vector<glm::vec3> vertices;
		std::vector<GLuint> indices;

		glm::vec3 frontBotLeft = glm::vec3(-0.5f, -0.5f, 0.5f); // 0
		glm::vec3 frontBotRight = glm::vec3(0.5f, -0.5f, 0.5f); // 1
		glm::vec3 frontTopRight = glm::vec3(0.5f, 0.5f, 0.5f);  // 2
		glm::vec3 frontTopLeft = glm::vec3(-0.5f, 0.5f, 0.5f);  // 3
		glm::vec3 backBotRight = glm::vec3(0.5f, -0.5f, -0.5f); // 4
		glm::vec3 backBotLeft = glm::vec3(-0.5f, -0.5f, -0.5f); // 5
		glm::vec3 backTopLeft = glm::vec3(-0.5f, 0.5f, -0.5f);  // 6
		glm::vec3 backTopRight = glm::vec3(0.5f, 0.5f, -0.5f);  // 7

		vertices = { frontBotLeft, frontBotRight, frontTopRight, frontTopLeft,
					 backBotRight, backBotLeft, backTopLeft, backTopRight };
		
		indices = { 0, 1, 2, 2, 3, 0,   // Face 1
					4, 5, 6, 6, 7, 4,   // Face 2
					1, 4, 7, 7, 2, 1,   // Face 3
					5, 0, 3, 3, 6, 5,   // Face 4
					3, 2, 7, 7, 6, 3,   // Face 5
					1, 0, 5, 5, 4, 1 }; // Face 6

		// Send data and its description to GPU
		glGenVertexArrays(1, &this->VAO);
		glGenBuffers(1, &this->VBO);
		glGenBuffers(1, &this->EBO);

		glBindVertexArray(this->VAO);

		glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (GLvoid*)0);
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);

		nIndices = static_cast<GLsizei>( indices.size() );

		meshInitiated = true;
	}
};

#endif