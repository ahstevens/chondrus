#pragma once

#include <vector>
#include <algorithm>

#include <glSkel/mesh.h>

class GeometryStrip
{
public:
	GeometryStrip(std::vector<std::vector<glm::vec3>> v)
		: vertices(v)
	{}

	~GeometryStrip() {}

	std::vector<glm::vec3> getVertices()
	{
		std::vector<glm::vec3> v;

		size_t nrows = this->vertices.size();

		for (GLuint row = 0; row < nrows; ++row)
		{
			size_t ncols = this->vertices[row].size();

			for (GLuint col = 0; col < ncols; ++col)
			{
				v.push_back(this->vertices[row][col]);
			}
		}

		return v;
	}

	std::vector<GLuint> getIndices()
	{
		std::vector<GLuint> inds;

		size_t nrows = this->vertices.size();

		for (GLuint row = 0; row < nrows - 1; ++row)
		{
			size_t ncols = this->vertices[row].size();

			for (GLuint col = 0; col < ncols - 1; ++col)
			{
				GLuint b = row * ncols + col;

				inds.push_back(b);
				inds.push_back(b + ncols + 1);
				inds.push_back(b + ncols);

				inds.push_back(b);
				inds.push_back(b + 1);
				inds.push_back(b + ncols + 1);
			}
		}

		return inds;
	}

	void glueLeft(GeometryStrip &s)
	{
		assert(this->vertices.size() == s.vertices.size()); // make sure the strips are the same length

		size_t nrows = s.vertices.size();

		for (GLuint row = 0; row < nrows; ++row)
		{
			size_t ncols = s.vertices[row].size();

			GLfloat xDisplacement = this->vertices[row].front().x - s.vertices[row].back().x;
			GLfloat yDisplacement = this->vertices[row].front().y - s.vertices[row].back().y;

			for (GLuint col = 0; col < ncols; ++col)
			{
				s.vertices[row][col].x += xDisplacement;
				s.vertices[row][col].y += yDisplacement;
			}

			// smooth out glue line
			this->vertices[row].front().z = (this->vertices[row].front().z + s.vertices[row].back().z) / 2.f;

			// Stitch the two mesh rows together
			this->vertices[row].insert(std::begin(this->vertices[row]), std::begin(s.vertices[row]), std::end(s.vertices[row]) - 1);			
		}
	}

	void glueRight(GeometryStrip &s)
	{
		assert(this->vertices.size() == s.vertices.size());

		size_t nrows = s.vertices.size();

		for (GLuint row = 0; row < nrows; ++row)
		{
			size_t ncols = s.vertices[row].size();

			GLfloat xDisplacement = this->vertices[row].back().x - s.vertices[row].front().x;
			GLfloat yDisplacement = this->vertices[row].back().y - s.vertices[row].front().y;

			for (GLuint col = 0; col < ncols; ++col)
			{
				s.vertices[row][col].x += xDisplacement;
				s.vertices[row][col].y += yDisplacement;
			}

			// smooth out glue line
			this->vertices[row].back().z = (this->vertices[row].back().z + s.vertices[row].front().z) / 2.f;

			// Stitch the two mesh rows together
			this->vertices[row].insert(std::end(this->vertices[row]), std::begin(s.vertices[row]) + 1, std::end(s.vertices[row]));
		}
	}

	unsigned int getWidthVertexCount() { return vertices[0].size();	}

	unsigned int getHeightVertexCount() { return vertices.size(); }

private:
	std::vector<std::vector<glm::vec3>> vertices; // row major grid of vertices
};