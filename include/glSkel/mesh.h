#pragma once
// Std. Includes
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>
#include <algorithm>

// GL Includes
#define GLEW_STATIC      // use static GLEW libs
#include <GL/glew.h> // Contains all the necessery OpenGL includes
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const float consolidationSearchRadius = 0.01f; // cm

struct Texture {
    GLuint id;
    std::string type;
};

class Mesh {
public:
	std::map<int, int> m_vOpposingVertPairs;
	std::vector<int> m_vBoundaryEdges;

    /*  Functions  */
    // Constructor to make a DCEL mesh from a triangle soup
	Mesh(std::vector<glm::vec3> vvec3Vertices, std::vector<GLuint> vuiIndices, std::vector<Texture> vTextures);    

	GLuint getVAO();

	unsigned int getVertexCount();

	unsigned int getFaceCount();

	unsigned int getHalfEdgeCount();

	unsigned int getBoundaryEdgeCount();

	float getSurfaceArea();

	float getPerimeter();

	void getIndexedVertices(std::vector<int> &i, std::vector<glm::vec3> &v);

	bool isBoundaryVertex(int index);

	void solidify(float distBetweenLayers);

	void updateMeshSerial(std::vector<float> &data);

	int getClosestVertexIndex(float x, float y, float z);
	
	std::vector<int> getClosestVertexIndicesKernel(float x, float y, float z, float radius);
	std::vector<int> getClosestVertexIndicesKernel(float x, float y, float z, float rx, float ry, float rz);

	glm::vec3 getCentroidPosition(const std::vector<int> &indices);
	glm::vec3 getPositionAtIndex(const int &index);

/************** HALF-EDGE (HE) DATA STRUCTS *******************/
private:
	struct HE_Vertex;
	struct HE_Edge;
	struct HE_Face;

	struct HE_Vertex {
		int id;
		glm::vec3 pos;
		HE_Edge *halfedge;

		HE_Vertex()
			: id(-1)
			, pos(glm::vec3(0.f))
			, halfedge(NULL)
		{}
	};

	struct HE_Edge {
		HE_Edge *next;
		HE_Edge *opposite;
		HE_Face *face;
		HE_Vertex *head;

		HE_Edge()
			: next(NULL)
			, opposite(NULL)
			, face(NULL)
			, head(NULL)
		{}

		HE_Edge* getPrev() const
		{
			HE_Edge *checkedEdge = this->opposite;
			while (checkedEdge->next != this)
				checkedEdge = checkedEdge->next->opposite;

			return checkedEdge;
		}

		bool isBoundaryEdge() { return face == NULL; }

	};

	struct HE_Face {
		HE_Edge *edge;
		glm::vec3 normal;

		HE_Face()
			: edge(NULL)
			, normal(glm::vec3(0.f))
		{}

		bool operator<(const HE_Face &rhs) const
		{
			return glm::length(this->normal) < glm::length(rhs.normal);
		}
	};

	/***** EDGE MAP DEF *****/
	typedef std::pair<int, int> EdgeMapIndexT;
	typedef std::map< EdgeMapIndexT, HE_Edge* > EdgeMapT;

	std::vector<HE_Edge*> m_vpEdges;
	std::vector<HE_Face*> m_vpFaces;
	std::vector<HE_Vertex*> m_vpVertices;
	std::vector<Texture> m_vTextures;
	HE_Edge *m_pBoundaryEdge;

/***** GL BUFFER DATA STRUCTS *****/
private:	
	struct Vertex {
		glm::vec3 pos;
		glm::vec3 norm;
		glm::vec2 tex;
	};

    /*  Render data  */
    GLuint m_glVAO, m_glVBO, m_glEBO;

/*  Functions    */
private:
	void initializeVertices(std::vector<glm::vec3> vertices);

	void processFacesAndEdges(std::vector<GLuint> vuiIndices);

	unsigned int consolidateDuplicateVertices(float searchRadius = 0.0000001f); // 1 nm

	void removeFaceAndEdges(HE_Edge *e);

	void updateVertexIDs();

	void updateBoundaryEdgePointer();

	void makeBufferVertices(std::vector<Vertex> & vVertices, std::vector<GLuint> & vIndices);

	void checkVertices();

	void checkFaces();

	void checkEdges();

	// Initializes all the buffer objects/arrays
	void setupGL();

};



