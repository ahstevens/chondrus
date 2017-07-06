#include "Chondrus.h"

#include <GLFW\glfw3.h>
#include <glSkel/GeometryStrip.h>
#include <glSkel/BulletDebugDrawer.h>

#include <algorithm>
#include <cmath>
#include <random>
#include <fstream>
#include <sys/stat.h> // stat()

#include <glSkel/Renderer.h>
#include <glSkel/DebugDrawer.h>

extern std::default_random_engine generator;

const float lengthGridSpacing = 0.5f; // cm, approx
const unsigned int center_nVertsWide = 3u;
const unsigned int edge_nVertsWide = 5u;
const float edgeCutoffPercent = 0.05f;

const float rayStrength = 0.01f;

const float L_AVG = 148.1f;        // avg length
const float L_STD = 55.26f;        // std. dev. length
const float W_AVG = 23.73f;        // avg width
const float W_STD = 6.933f;        // std. dev. width
const float P_AVG = 19.97f;        // avg periodicity
const float P_STD = 3.446f;        // std. dev. periodicity
const float LW_RATIO_AVG = 6.277f;
const float LW_RATIO_STD = 2.010f;
const float LP_RATIO_AVG = 7.738f;
const float LP_RATIO_STD = 2.389f;

struct Payload
{
	int index;
	float x0;
	float y0;
	float z0;
};

Chondrus::Chondrus(float solidThickness, float length, float width, glm::vec3 position, glm::mat3 orientation)
	: Object(position, orientation)
	, m_glDiffTex(0)
	, m_glSpecTex(0)
	, m_bWireframe(false)
{
	m_fLength = length;
	m_fWidth = width;

	std::cout << "Length: " << m_fLength << " | Width: " << m_fWidth << std::endl;

	m_nVertsTall = static_cast<GLuint>(m_fLength / lengthGridSpacing);

	buildModel();

	m_pDebugDrawer = new BulletDebugDrawer();
}


Chondrus::~Chondrus()
{
	if (m_pMesh)
		delete(m_pMesh);
}

float Chondrus::getLength()
{
	return m_fLength;
}

float Chondrus::getWidth()
{
	return m_fWidth;
}


void Chondrus::update()
{
}

void Chondrus::receiveEvent(Object* obj, const int event, void * data)
{
	if (event == BroadcastSystem::EVENT::KEY_PRESS)
	{	
		int key;
		memcpy(&key, data, sizeof(key));

		if (key == GLFW_KEY_M)
			m_bWireframe = !m_bWireframe;

		if (key == GLFW_KEY_KP_ENTER)
			saveAsObj("test");
	}

	if (event == BroadcastSystem::EVENT::GROW_RAY || event == BroadcastSystem::EVENT::SHRINK_RAY)
	{
		glm::vec3 payload[2];
		memcpy(&payload, data, sizeof(payload));		
	}
}

void Chondrus::Draw()
{
	drawBranches(m_pRoot);

	glm::mat4 modelMat = glm::translate(glm::mat4(), m_vec3Position) * glm::mat4(m_mat3Rotation);

	Renderer::RendererSubmission rs;
	rs.primitiveType = GL_TRIANGLES;
	rs.shaderName = m_bWireframe ? "lightingWireframe" : "lighting";
	rs.VAO = m_pMesh->getVAO();
	rs.vertCount = m_pMesh->getFaceCount() * 3;
	rs.diffuseTex = m_glDiffTex;
	rs.specularTex = m_glSpecTex;
	rs.specularExponent = 32.f;
	rs.modelToWorldTransform = modelMat;

	Renderer::getInstance().addToDynamicRenderQueue(rs);
}

void Chondrus::buildModel()
{
	m_pRoot = new Segment();
	m_pRoot->begin = glm::vec3(0.f);
	m_pRoot->end = glm::vec3(0.f, 33.f, 0.f);

	Segment* branch = new Segment();
	branch->parent = m_pRoot;
	branch->begin = glm::vec3(0.f, 33.f, 0.f);
	branch->end = glm::vec3(20.f, 45.f, 0.f);
	m_pRoot->children.push_back(branch);

	branch = new Segment();
	branch->parent = m_pRoot;
	branch->begin = glm::vec3(0.f, 33.f, 0.f);
	branch->end = glm::vec3(-20.f, 45.f, 0.f);
	m_pRoot->children.push_back(branch);

	std::vector<std::vector<glm::vec3>> vertices; // row major
	glm::vec3 tempVert;

	float edgeWidthPercent = 0.5f;
	float edgeWidth = m_fWidth * edgeWidthPercent;
	
	for (GLuint row = 0; row < m_nVertsTall; ++row)
	{
		std::vector<glm::vec3> vecRow;
		GLfloat dy = static_cast<GLfloat>(row) / static_cast<GLfloat>(m_nVertsTall - 1);

		for (GLuint col = 0; col < edge_nVertsWide; ++col)
		{
			GLfloat dx = static_cast<GLfloat>(col) / static_cast<GLfloat>(edge_nVertsWide - 1);

			tempVert.x = (dx - 0.5f) * edgeWidth * calculateEnvelope(dy, 0.f, 0.85f, 0.95f, 1.f);
			
			tempVert.y = dy * m_fLength;

			tempVert.z = 0.f;

			vecRow.push_back(tempVert);
		}

		vertices.push_back(vecRow);
	}

	GeometryStrip leftStrip(vertices);
	
	vertices.clear();
	for (GLuint row = 0; row < m_nVertsTall; ++row)
	{
		std::vector<glm::vec3> vecRow;
		GLfloat dy = static_cast<GLfloat>(row) / static_cast<GLfloat>(m_nVertsTall - 1);

		for (GLuint col = 0; col < edge_nVertsWide; ++col)
		{
			GLfloat dx = static_cast<GLfloat>(col) / static_cast<GLfloat>(edge_nVertsWide - 1);

			tempVert.x = (dx + 0.5f) * edgeWidth * calculateEnvelope(dy, 0.f, 0.1f, 0.9f, 1.f);

			tempVert.y = dy * m_fLength;

			tempVert.z = 0.f;

			vecRow.push_back(tempVert);
		}

		vertices.push_back(vecRow);
	}

	GeometryStrip rightStrip(vertices);

	//leftStrip.glueRight(rightStrip);

	std::cout << "Creating DCEL mesh from geometry strip that is " << leftStrip.getWidthVertexCount() << " verts wide and " << leftStrip.getHeightVertexCount() << " verts long" << std::endl;
	m_pMesh = new Mesh(leftStrip.getVertices(), leftStrip.getIndices());
	
	loadTextures();
}

float Chondrus::calculateEnvelope(float currentRatio, float beginRatio, float maxRatio1, float maxRatio2, float endRatio)
{
	if (currentRatio < beginRatio || currentRatio > endRatio)
		return 0.f;

	if (currentRatio > maxRatio1 && currentRatio < maxRatio2)
		return 1.f;
	
	if (currentRatio < maxRatio1)
	{
		float r = (maxRatio1 - currentRatio) / (maxRatio1 - beginRatio);
		return sin((1.f - r) * glm::half_pi<GLfloat>());
	}
	else // currentRatio > maxRatio2
	{
		float r = (maxRatio2 - currentRatio) / (maxRatio2 - endRatio);
		return sin((1.f - r) * glm::half_pi<GLfloat>());
	}
}

float Chondrus::getRandRatio()
{
	return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

void Chondrus::loadTextures()
{
	// Load textures
	glGenTextures(1, &m_glDiffTex);
	glGenTextures(1, &m_glSpecTex);
	int width = 1, height = 1;
	unsigned char image[3] = { 0x88, 0xFF, 0x88 };

	// Diffuse map
	glBindTexture(GL_TEXTURE_2D, m_glDiffTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, &image);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Specular map
	//image[0] = 0xFF;
	//image[1] = 0xFF;
	//image[2] = 0xFF;
	glBindTexture(GL_TEXTURE_2D, m_glSpecTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, &image);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void Chondrus::drawBranches(Segment * root)
{
	for (auto const &child : root->children)
		drawBranches(child);

	DebugDrawer::getInstance().drawLine(root->begin, root->end);
}

bool fileExists(const std::string &fname)
{
	struct stat buffer;
	return (stat(fname.c_str(), &buffer) == 0);
}

bool Chondrus::saveAsObj(std::string name)
{
	std::string outFileName = std::string("export/" + name + ".obj");

	// if file exists, keep trying until we find a filename that doesn't already exist
	for (int i = 0; fileExists(outFileName); ++i)
		outFileName = std::string("export/" + name + "_" + std::to_string(i) + ".obj");

	std::ofstream outFile;
	outFile.open(outFileName);

	if (!outFile.is_open())
	{
		std::cout << "Error opening file " << outFileName << " for writing output" << std::endl;
		return false;
	}

	std::cout << "Opened file " << outFileName << " for writing output" << std::endl;

	outFile << "#" << outFileName << std::endl;

	outFile << "#vertex data" << std::endl;
	outFile << "#v x y z (w = 1.0)" << std::endl;
	outFile << "#vn i j k" << std::endl;
	outFile << "#vt u v" << std::endl;
	outFile << std::endl;

	//btAlignedObjectArray<btSoftBody::Node> nodes = m_pSoftBody->m_nodes;
	//for (size_t i = 0; i < nodes.size(); ++i)
	//{
	//	outFile << "v " << nodes[i].m_x.getX() << " " << nodes[i].m_x.getY() << " " << nodes[i].m_x.getZ() << std::endl;
	//	outFile << "vn " << nodes[i].m_n.getX() << " " << nodes[i].m_n.getY() << " " << nodes[i].m_n.getZ() << std::endl;
	//	outFile << "vt " << 0.5 << " " << 0.5 << std::endl;
	//}

	outFile << std::endl;
	outFile << "#face data" << std::endl;
	outFile << "#f vertex1Num/texCoord1Num/vertNormal1Num";
	outFile << " vertex2Num/texCoord2Num/vertNormal2Num";
	outFile << " vertex3Num/texCoord3Num/vertNormal3Num" << std::endl;
	outFile << std::endl;

	std::vector<int> inds;
	std::vector<glm::vec3> verts;
	m_pMesh->getIndexedVertices(inds, verts);

	for (int i = 0; i < inds.size(); i += 3)
	{
		outFile << "f ";
		outFile << inds[i] + 1 << "/" << inds[i] + 1 << "/" << inds[i] + 1 << " ";
		outFile << inds[i + 1] + 1 << "/" << inds[i + 1] + 1 << "/" << inds[i + 1] + 1 << " ";
		outFile << inds[i + 2] + 1 << "/" << inds[i + 2] + 1 << "/" << inds[i + 2] + 1 << std::endl;
	}

	outFile << std::endl << "#end " << outFileName;

	outFile.close();

	std::cout << "Saved file " << outFileName << " successfully" << std::endl;

	return true;
}
