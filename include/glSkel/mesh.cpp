#include "mesh.h"
#pragma once

Mesh::Mesh(std::vector<glm::vec3> vvec3Vertices, std::vector<GLuint> vuiIndices, std::vector<Texture> vTextures)
{
	this->m_vTextures = vTextures;
	this->m_pBoundaryEdge = NULL;

	std::cout << "\t*Initializing " << vvec3Vertices.size() << " vertices... ";
	this->initializeVertices(vvec3Vertices);
	std::cout << "done!" << std::endl;

	std::cout << "\t*Processing " << vuiIndices.size() / 3 << " faces... ";
	this->processFacesAndEdges(vuiIndices);
	std::cout << "done! (" << m_vpFaces.size() << " faces and " << m_vpEdges.size() << " half-edges created)" << std::endl;

	//checkVertices();
	//checkFaces();
	//checkEdges();

	std::cout << "\t*Consolidating vertices within " << consolidationSearchRadius * 100.f << "mm of each other... ";
	unsigned int nVertsConsolidated = this->consolidateDuplicateVertices(consolidationSearchRadius); // 1 mm
	std::cout << "done! (" << nVertsConsolidated << " vertices removed)" << std::endl;

	checkEdges();
	checkFaces();
	checkVertices();

	std::cout << "\t*Surface area: " << getSurfaceArea() << " cm^2" << std::endl;

	std::cout << "\t*Surface perimeter: " << getPerimeter() << " cm" << std::endl;

	// Now that we have all the required data, set the vertex buffers and its attribute pointers.
	this->setupGL();
}

GLuint Mesh::getVAO()
{
	return m_glVAO;
}

unsigned int Mesh::getVertexCount()
{
	return m_vpVertices.size();
}

unsigned int Mesh::getFaceCount()
{
	return m_vpFaces.size();
}

unsigned int Mesh::getHalfEdgeCount()
{
	return m_vpEdges.size();
}

unsigned int Mesh::getBoundaryEdgeCount()
{
	if (!m_pBoundaryEdge)
		return 0u;

	HE_Edge *e = m_pBoundaryEdge;

	unsigned int count = 0u;
	do
	{
		count++;
		e = e->next;
	} while (e != m_pBoundaryEdge);

	return count;
}

float Mesh::getSurfaceArea()
{
	float area = 0.f;

	// Each face area is half the magnitude of the face normal,
	// therefore the surface area is half of the sum of the face normal magnitudes
	for (int i = 0; i < m_vpFaces.size(); ++i)
		area += glm::length(m_vpFaces[i]->normal);

	return area / 2.f;
}

float Mesh::getPerimeter()
{
	if (!m_pBoundaryEdge)
		return 0.f;

	float p = 0.f;
	HE_Edge *e = m_pBoundaryEdge;
	do
	{
		p += glm::length(e->head->pos - e->opposite->head->pos);
		e = e->next;
	} while (e != m_pBoundaryEdge);

	return p;
}

void Mesh::getIndexedVertices(std::vector<int>& i, std::vector<glm::vec3>& v)
{
	for (std::vector<HE_Face*>::iterator it = m_vpFaces.begin(); it != m_vpFaces.end(); it++)
	{
		HE_Edge *begin = (*it)->edge;
		HE_Edge *e = begin;

		do
		{
			i.push_back(e->head->id);
			e = e->next;
		} while (e != begin);
	}

	for (auto vert : m_vpVertices)
		v.push_back(vert->pos);
}

bool Mesh::isBoundaryVertex(int index)
{
	if (!m_pBoundaryEdge)
		return false;

	HE_Edge* begin = m_pBoundaryEdge;
	HE_Edge* e = begin;
	do
	{
		if (index == e->head->id)
			return true;
		e = e->next;
	} while (e != begin);

	return false;
}

void Mesh::solidify(float distBetweenLayers)
{
	std::vector<HE_Vertex*> backVerts;
	std::vector<HE_Face*> backFaces, connectingFaces;
	std::vector<HE_Edge*> backEdges, connectingEdges;

	// back layer
	for (auto frontVert : m_vpVertices)
	{
		HE_Vertex *v = new HE_Vertex();
		v->pos = frontVert->pos;
		frontVert->pos.z += distBetweenLayers / 2.f;
		v->pos.z -= distBetweenLayers / 2.f;
		v->id = m_vpVertices.size() + frontVert->id;

		backVerts.push_back(v);

		m_vOpposingVertPairs[frontVert->id] = v->id;
	}

	EdgeMapT edgeMap;

	for (auto frontFace : m_vpFaces)
	{
		HE_Face *f = new HE_Face();
		HE_Edge *e1 = new HE_Edge();
		HE_Edge *e2 = new HE_Edge();
		HE_Edge *e3 = new HE_Edge();
		e1->next = e2;
		e2->next = e3;
		e3->next = e1;
		e1->face = f;
		e2->face = f;
		e3->face = f;
		f->edge = e1;
		e1->head = backVerts.at(frontFace->edge->opposite->head->id);
		e2->head = backVerts.at(frontFace->edge->getPrev()->opposite->head->id);
		e3->head = backVerts.at(frontFace->edge->next->opposite->head->id);

		if (!e1->head->halfedge) e1->head->halfedge = e2;
		if (!e2->head->halfedge) e2->head->halfedge = e3;
		if (!e3->head->halfedge) e3->head->halfedge = e1;

		f->normal = glm::cross(e1->head->pos - e3->head->pos, e2->head->pos - e3->head->pos);
		backFaces.push_back(f);
		backEdges.push_back(e1);
		backEdges.push_back(e2);
		backEdges.push_back(e3);

		// enter these 
		edgeMap[EdgeMapIndexT(e3->head->id, e1->head->id)] = e1;
		edgeMap[EdgeMapIndexT(e1->head->id, e2->head->id)] = e2;
		edgeMap[EdgeMapIndexT(e2->head->id, e3->head->id)] = e3;
	}

	// link up paired half edges
	for (EdgeMapT::iterator it = edgeMap.begin(); it != edgeMap.end(); it++)
	{
		if (it->second->opposite != NULL) continue;

		// look for an edge going the opposite way in the edge map
		EdgeMapT::iterator opp = edgeMap.find(std::pair<int, int>(it->first.second, it->first.first));
		if (opp != edgeMap.end()) // found a match, so link 'em up
		{
			it->second->opposite = opp->second;
			opp->second->opposite = it->second;
		}
		else // no match, therefore it's a boundary edge
		{
			HE_Edge *newBoundaryEdge = new HE_Edge();
			newBoundaryEdge->head = it->second->next->next->head; // a boundary edge always points to boundary vertex 
			newBoundaryEdge->face = NULL; // NULL face indicates a boundary edge

										  // link new edge to next, if it exists
			if (!newBoundaryEdge->head->halfedge->face)
				newBoundaryEdge->next = newBoundaryEdge->head->halfedge;

			// link opposite edges
			newBoundaryEdge->opposite = it->second;
			it->second->opposite = newBoundaryEdge;

			// change emanating vertex to point to new boundary edge.
			// edges of boundary are oriented in CW order
			it->second->head->halfedge = newBoundaryEdge;

			backEdges.push_back(newBoundaryEdge);
		}
	}

	// one more pass to fill in any unlinked boundary edges
	for (int i = 0; i < backEdges.size(); ++i)
		if (!backEdges[i]->next)
			backEdges[i]->next = backEdges[i]->head->halfedge;

	// stich them together
	HE_Edge *startEdge = m_pBoundaryEdge;
	HE_Edge *bEdge = startEdge;
	EdgeMapT unlinkedEdgeMap;
	do {
		HE_Edge *nextBEdge = bEdge->next;

		HE_Face *tri1 = new HE_Face();
		HE_Face *tri2 = new HE_Face();
		HE_Edge *tri1e1 = bEdge;
		HE_Edge *tri1e2 = new HE_Edge();
		HE_Edge *tri1e3 = new HE_Edge();
		HE_Edge *tri2e1 = backVerts.at(bEdge->head->id)->halfedge;
		HE_Edge *tri2e2 = new HE_Edge();
		HE_Edge *tri2e3 = new HE_Edge();

		tri1e1->face = tri1;
		tri1e1->next = tri1e2;

		tri1e2->face = tri1;
		tri1e2->next = tri1e3;
		tri1e2->head = backVerts.at(bEdge->opposite->head->id);
		tri1e2->opposite = tri2e2;

		tri1e3->face = tri1;
		tri1e3->next = tri1e1;
		tri1e3->head = bEdge->opposite->head;
		unlinkedEdgeMap[EdgeMapIndexT(tri1e2->head->id, tri1e3->head->id)] = tri1e3;

		tri2e1->face = tri2;
		tri2e1->next = tri2e2;

		tri2e2->face = tri2;
		tri2e2->next = tri2e3;
		tri2e2->head = bEdge->head;
		tri2e2->opposite = tri1e2;

		tri2e3->face = tri2;
		tri2e3->next = tri2e1;
		tri2e3->head = backVerts.at(bEdge->head->id);
		unlinkedEdgeMap[EdgeMapIndexT(tri2e2->head->id, tri2e3->head->id)] = tri2e3;

		tri1->edge = tri1e1;
		tri2->edge = tri2e1;

		tri1->normal = glm::cross(tri1e1->head->pos - tri1e3->head->pos, tri1e2->head->pos - tri1e3->head->pos);
		tri2->normal = glm::cross(tri2e1->head->pos - tri2e3->head->pos, tri2e2->head->pos - tri2e3->head->pos);

		connectingFaces.push_back(tri1);
		connectingFaces.push_back(tri2);
		connectingEdges.push_back(tri1e1);
		connectingEdges.push_back(tri1e2);
		connectingEdges.push_back(tri1e3);
		connectingEdges.push_back(tri2e1);
		connectingEdges.push_back(tri2e2);
		connectingEdges.push_back(tri2e3);

		bEdge = nextBEdge;
	} while (bEdge != startEdge);

	for (EdgeMapT::iterator it = unlinkedEdgeMap.begin(); it != unlinkedEdgeMap.end(); it++)
	{
		if (it->second->opposite != NULL) continue;

		// look for an edge going the opposite way in the edge map
		EdgeMapT::iterator opp = unlinkedEdgeMap.find(std::pair<int, int>(it->first.second, it->first.first));
		if (opp != unlinkedEdgeMap.end()) // found a match, so link 'em up
		{
			it->second->opposite = opp->second;
			opp->second->opposite = it->second;
		}
	}

	m_vpVertices.insert(std::end(m_vpVertices), std::begin(backVerts), std::end(backVerts));

	m_vpFaces.insert(std::end(m_vpFaces), std::begin(backFaces), std::end(backFaces));
	m_vpFaces.insert(std::end(m_vpFaces), std::begin(connectingFaces), std::end(connectingFaces));

	m_vpEdges.insert(std::end(m_vpEdges), std::begin(backEdges), std::end(backEdges));
	m_vpEdges.insert(std::end(m_vpEdges), std::begin(connectingEdges), std::end(connectingEdges));

	m_pBoundaryEdge = NULL;

	setupGL();
}

void Mesh::updateMeshSerial(std::vector<float>& data)
{
	glBindBuffer(GL_ARRAY_BUFFER, this->m_glVBO);
	glBufferData(GL_ARRAY_BUFFER, data.size(), 0, GL_STREAM_DRAW);
	glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STREAM_DRAW);
}

//Brute force; not very pretty but it works for now
int Mesh::getClosestVertexIndex(float x, float y, float z)
{
	float dist_sq = FLT_MAX;
	
	HE_Vertex *closestVert = NULL;

	for (auto const &v : m_vpVertices)
	{
		float temp_dist_sq = (v->pos.x - x)*(v->pos.x - x) + (v->pos.y - y)*(v->pos.y - y) + (v->pos.z - z)*(v->pos.z - z);

		if (temp_dist_sq < dist_sq)
		{
			closestVert = v;
		}
	}

	return closestVert ? closestVert->id : -1;
}

//Brute force; not very pretty but it works for now
std::vector<int> Mesh::getClosestVertexIndicesKernel(float x, float y, float z, float radius)
{
	float r_sq = radius * radius;
	std::vector<int> foundIndices;

	for (auto const &v : m_vpVertices)
	{
		float d_sq = (v->pos.x - x)*(v->pos.x - x) + (v->pos.y - y)*(v->pos.y - y) + (v->pos.z - z)*(v->pos.z - z);

		if (d_sq <= r_sq)
		{
			foundIndices.push_back(v->id);
		}
	}

	return foundIndices;
}

//Brute force; not very pretty but it works for now
std::vector<int> Mesh::getClosestVertexIndicesKernel(float x, float y, float z, float rx, float ry, float rz)
{
	float rx_sq = rx * rx;
	float ry_sq = ry * ry;
	float rz_sq = rz * rz;

	std::vector<int> foundIndices;

	for (auto const &v : m_vpVertices)
	{
		float dx_sq = (v->pos.x - x) * (v->pos.x - x);
		float dy_sq = (v->pos.y - y) * (v->pos.y - y);
		float dz_sq = (v->pos.z - z) * (v->pos.z - z);

		float res = dx_sq / rx_sq + dy_sq / ry_sq + dz_sq / rz_sq;

		if (res <= 1)
		{
			foundIndices.push_back(v->id);
		}
	}

	return foundIndices;
}

glm::vec3 Mesh::getCentroidPosition(const std::vector<int> &indices)
{
	if (indices.size() < 1)
		return glm::vec3(0.f);

	glm::vec3 sumPts = glm::vec3(0.f);

	for (auto const &i : indices)	
		sumPts += m_vpVertices[i]->pos;
	
	sumPts /= static_cast<float>(indices.size());

	return sumPts;
}

glm::vec3 Mesh::getPositionAtIndex(const int & index)
{
	return m_vpVertices[index]->pos;
}

void Mesh::initializeVertices(std::vector<glm::vec3> vertices)
{
	for (int i = 0; i < vertices.size(); ++i)
	{
		HE_Vertex *v = new HE_Vertex();
		v->id = i;
		v->pos = vertices[i];
		this->m_vpVertices.push_back(v);
	}
}

void Mesh::processFacesAndEdges(std::vector<GLuint> vuiIndices)
{
	// make an edge map to hold the index pairs of the vertices that make up
	// the start and end of the half edge, respectively
	EdgeMapT edgeMap;

	for (int i = 0; i < vuiIndices.size(); i += 3)
	{
		HE_Face *f = new HE_Face();

		// Create the three halfedges of the face in CCW order
		HE_Edge *e1, *e2, *e3;
		e1 = new HE_Edge();
		e2 = new HE_Edge();
		e3 = new HE_Edge();

		e1->face = f;
		e1->next = e2;
		e1->head = m_vpVertices[vuiIndices[i + 1]];

		e2->face = f;
		e2->next = e3;
		e2->head = m_vpVertices[vuiIndices[i + 2]];

		e3->face = f;
		e3->next = e1;
		e3->head = m_vpVertices[vuiIndices[i]];

		if (!e1->head->halfedge) e1->head->halfedge = e2;
		if (!e2->head->halfedge) e2->head->halfedge = e3;
		if (!e3->head->halfedge) e3->head->halfedge = e1;

		f->edge = e1; // doesn't matter which edge it points to
		f->normal = glm::cross(e1->head->pos - e3->head->pos, e2->head->pos - e3->head->pos);
		m_vpFaces.push_back(f);
		m_vpEdges.push_back(e1);
		m_vpEdges.push_back(e2);
		m_vpEdges.push_back(e3);

		// enter these 
		edgeMap[EdgeMapIndexT(vuiIndices[i], vuiIndices[i + 1])] = e1;
		edgeMap[EdgeMapIndexT(vuiIndices[i + 1], vuiIndices[i + 2])] = e2;
		edgeMap[EdgeMapIndexT(vuiIndices[i + 2], vuiIndices[i])] = e3;
	}

	// link up paired half edges
	for (EdgeMapT::iterator it = edgeMap.begin(); it != edgeMap.end(); it++)
	{
		if (it->second->opposite != NULL) continue;

		// look for an edge going the opposite way in the edge map
		EdgeMapT::iterator opp = edgeMap.find(std::pair<int, int>(it->first.second, it->first.first));
		if (opp != edgeMap.end()) // found a match, so link 'em up
		{
			it->second->opposite = opp->second;
			opp->second->opposite = it->second;
		}
		else // no match, therefore it's a boundary edge
		{
			HE_Edge *newBoundaryEdge = new HE_Edge();
			newBoundaryEdge->head = it->second->next->next->head; // a boundary edge always points to boundary vertex 
			newBoundaryEdge->face = NULL; // NULL face indicates a boundary edge

										  // link new edge to next, if it exists
			if (!newBoundaryEdge->head->halfedge->face)
				newBoundaryEdge->next = newBoundaryEdge->head->halfedge;

			// link opposite edges
			newBoundaryEdge->opposite = it->second;
			it->second->opposite = newBoundaryEdge;

			// change emanating vertex to point to new boundary edge.
			// edges of boundary are oriented in CW order
			it->second->head->halfedge = newBoundaryEdge;

			m_vpEdges.push_back(newBoundaryEdge);

			// set the mesh boundary edge pointer the first time.
			// NULL boundary edge pointer afterwards means it's a closed mesh
			if (!m_pBoundaryEdge) m_pBoundaryEdge = newBoundaryEdge;
		}
	}

	// one more pass to fill in any unlinked boundary edges
	for (int i = 0; i < m_vpEdges.size(); ++i)
		if (!m_vpEdges[i]->next)
			m_vpEdges[i]->next = m_vpEdges[i]->head->halfedge;
}

unsigned int Mesh::consolidateDuplicateVertices(float searchRadius)
{
	float searchRadius_sq = searchRadius * searchRadius;

	bool vertexRemoved = false;

	size_t nVertsBeforeConsolidation = m_vpVertices.size();

	for (std::vector<HE_Vertex*>::iterator it = m_vpVertices.begin(); it != m_vpVertices.end(); it++)
	{
		HE_Edge *beginEdge = (*it)->halfedge;
		bool skipBeginEdgeCheck;
		HE_Edge *currentEdge = beginEdge;

		do
		{
			skipBeginEdgeCheck = false;
			glm::vec3 vecToNeighborVert = currentEdge->head->pos - (*it)->pos;
			float len_sq = vecToNeighborVert.x * vecToNeighborVert.x + vecToNeighborVert.y * vecToNeighborVert.y + vecToNeighborVert.z * vecToNeighborVert.z;

			// If the neighboring vertex is too close, merge it with current vertex
			if (len_sq < searchRadius_sq)// && (!currentEdge->head->halfedge->isBoundaryEdge() || currentEdge->isBoundaryEdge() || currentEdge->opposite->isBoundaryEdge()))
			{
				//std::cout << "Removing vertex " << currentEdge->head->id << " because it is " << sqrtf(len_sq) << "cm away from vertex " << (*it)->id << std::endl;

				HE_Edge *nextEdge;

				if (currentEdge->isBoundaryEdge())
					nextEdge = currentEdge->next;
				else
					nextEdge = currentEdge->next->next->opposite;

				HE_Edge *currEdgePrev = currentEdge->getPrev();
				HE_Vertex *doomedVert = currentEdge->head;

				if (currEdgePrev->opposite == beginEdge)
					skipBeginEdgeCheck = true;

				// connect doomed vert's incoming half-edge heads to current vert
				HE_Edge *doomedVertIncomingEdge = currentEdge->next->opposite;

				while (doomedVertIncomingEdge != currentEdge)
				{
					//std::cout << "\tVertex " << (*it)->id << ": Reassigning " << (doomedVertIncomingEdge->isBoundaryEdge() ? "boundary half-edge" : "half-edge") << " pointing to vertex " << doomedVert->id << " from vertex " << doomedVertIncomingEdge->opposite->head->id << std::endl;
					// set the head pointer to point to current vertex
					doomedVertIncomingEdge->head = (*it);

					HE_Edge *nextIncomingEdge = doomedVertIncomingEdge->next->opposite;
					// if the incoming edge is a boundary edge, we need to update its next pointer as well
					if (doomedVertIncomingEdge->isBoundaryEdge())
					{
						if (currentEdge->opposite->isBoundaryEdge())
							doomedVertIncomingEdge->next = doomedVertIncomingEdge->next->next;
						else if (currentEdge->opposite->next->opposite->isBoundaryEdge() && currentEdge->opposite->next->next->opposite->isBoundaryEdge())
							doomedVertIncomingEdge->next = doomedVertIncomingEdge->next->next->next;
					}
					// go to the next incoming edge
					doomedVertIncomingEdge = nextIncomingEdge;
				}

				/*
				discard any faces incident to current half-edge pair since their contribution is negligible
				-if the half edge has a face associated with it, remove the face:
				--first set proceding and preceding half-edges' opposite half-edges to point to one another
				--then discard the face
				--discard the preceding and proceding half-edges, as well as the current half-edge pair
				----if the half-edge to be discarded is what the boundary edge pointer points to, advance it to the next boundary half-edge
				*/
				removeFaceAndEdges(currentEdge->opposite);

				if (currentEdge->isBoundaryEdge())
				{
					currEdgePrev->next = currentEdge->next;

					(*it)->halfedge = currentEdge->next;
					beginEdge = (*it)->halfedge;
					skipBeginEdgeCheck = true;

					removeFaceAndEdges(currentEdge);
				}
				else
				{
					removeFaceAndEdges(currentEdge);
				}

				// discard the doomed vertex
				m_vpVertices.erase(std::remove(m_vpVertices.begin(), m_vpVertices.end(), doomedVert), m_vpVertices.end());
				delete doomedVert;

				vertexRemoved = true;

				currentEdge = nextEdge;
			}
			else // vertex at head of current edge is far enough away
			{
				currentEdge = currentEdge->opposite->next;
			}
		} while (currentEdge != beginEdge || skipBeginEdgeCheck);
	}

	if (vertexRemoved)
		updateVertexIDs();

	this->updateBoundaryEdgePointer();

	return static_cast<unsigned int>(nVertsBeforeConsolidation - m_vpVertices.size());
}

void Mesh::removeFaceAndEdges(HE_Edge * e)
{
	if (e->isBoundaryEdge())
	{
		//std::cout << "\t\tRemoving boundary edge pointing at vertex " << e->head->id << std::endl;
		m_vpEdges.erase(std::remove(m_vpEdges.begin(), m_vpEdges.end(), e), m_vpEdges.end());
		delete e;
		return;
	}

	// link new opposites
	e->next->next->opposite->opposite = e->next->opposite;
	e->next->opposite->opposite = e->next->next->opposite;

	HE_Vertex* v = NULL;
	if (e->next->next->opposite->isBoundaryEdge() && e->next->opposite->isBoundaryEdge())
		v = e->next->head;

	// discard face
	m_vpFaces.erase(std::remove(m_vpFaces.begin(), m_vpFaces.end(), e->face), m_vpFaces.end());
	delete e->face;

	// if the neighboring vertex's halfedge points to the edge being removed, assign a new half-edge
	if (e->next->next == e->next->head->halfedge)
		e->next->head->halfedge = e->next->opposite;

	// now delete edges
	m_vpEdges.erase(std::remove(m_vpEdges.begin(), m_vpEdges.end(), e->next->next), m_vpEdges.end());
	delete e->next->next;

	// if this vertex's halfedge points to the edge being removed, assign a new half-edge
	if (e->next == e->head->halfedge)
		e->head->halfedge = e->next->opposite->next;

	m_vpEdges.erase(std::remove(m_vpEdges.begin(), m_vpEdges.end(), e->next), m_vpEdges.end());
	delete e->next;

	m_vpEdges.erase(std::remove(m_vpEdges.begin(), m_vpEdges.end(), e), m_vpEdges.end());
	delete e;

	e = NULL;

	// if the half-edge collapse creates a degenerate vertex/half-edge pair, prune it
	if (v != NULL)
	{
		//std::cout << "\t\tRemoving degenerate vertex " << v->id << std::endl;
		m_vpEdges.erase(std::remove(m_vpEdges.begin(), m_vpEdges.end(), v->halfedge->opposite), m_vpEdges.end());
		delete v->halfedge->opposite;

		m_vpEdges.erase(std::remove(m_vpEdges.begin(), m_vpEdges.end(), v->halfedge), m_vpEdges.end());
		delete v->halfedge;

		m_vpVertices.erase(std::remove(m_vpVertices.begin(), m_vpVertices.end(), v), m_vpVertices.end());
		delete v;
	}
}

void Mesh::updateVertexIDs()
{
	for (size_t i = 0; i < m_vpVertices.size(); ++i)
		m_vpVertices[i]->id = i;
}

void Mesh::updateBoundaryEdgePointer()
{
	for (std::vector<HE_Edge*>::iterator it = m_vpEdges.begin(); it != m_vpEdges.end(); it++)
	{
		if ((*it)->isBoundaryEdge())
		{
			m_pBoundaryEdge = (*it);
			break;
		}
	}
}

void Mesh::makeBufferVertices(std::vector<Vertex>& vVertices, std::vector<GLuint>& vIndices)
{
	vVertices.resize(m_vpVertices.size()); // reserve memory for vertices

	for (std::vector<HE_Vertex*>::iterator it = m_vpVertices.begin(); it != m_vpVertices.end(); it++)
	{
		Vertex v;
		v.pos = (*it)->pos;
		v.norm = glm::vec3(0.f);
		v.tex = glm::vec2(0.5f);

		// calc vertex normals as sum of (unnormalized) incident face normals
		HE_Edge *beginEdge = (*it)->halfedge;
		HE_Edge *e = beginEdge;
		do
		{
			if (e->face)
				v.norm += e->face->normal;
			e = e->opposite->next;
		} while (e != beginEdge);

		v.norm = glm::normalize(v.norm);

		vVertices.at((*it)->id) = v;
	}

	for (std::vector<HE_Face*>::iterator it = m_vpFaces.begin(); it != m_vpFaces.end(); it++)
	{
		HE_Edge *begin = (*it)->edge;
		HE_Edge *e = begin;

		do
		{
			vIndices.push_back(e->head->id);
			e = e->next;
		} while (e != begin);
	}
}

void Mesh::checkVertices()
{
	for (std::vector<HE_Vertex*>::iterator it = m_vpVertices.begin(); it != m_vpVertices.end(); it++)
	{
		assert((*it)->id >= 0);
		assert((*it)->halfedge != NULL);
	}
}

void Mesh::checkFaces()
{
	for (std::vector<HE_Face*>::iterator it = m_vpFaces.begin(); it != m_vpFaces.end(); it++)
	{
		assert((*it)->edge != NULL);
	}
}

void Mesh::checkEdges()
{
	for (std::vector<HE_Edge*>::iterator it = m_vpEdges.begin(); it != m_vpEdges.end(); it++)
	{
		assert((*it)->head != NULL);
		assert((*it)->next != NULL);
		assert((*it)->opposite != NULL);
		assert((*it)->opposite->opposite == *it);
		if ((*it)->face == NULL)
		{
			assert((*it)->next->face == NULL);
			assert((*it)->opposite->head->halfedge == *it);
		}
		assert((*it)->getPrev()->next == *it);

	}
}

void Mesh::setupGL()
{
	std::vector<Vertex> bufferVertices;
	std::vector<GLuint> bufferIndices;

	makeBufferVertices(bufferVertices, bufferIndices);

	// Create buffers/arrays
	if (!this->m_glVAO) glGenVertexArrays(1, &this->m_glVAO);
	if (!this->m_glVBO) glGenBuffers(1, &this->m_glVBO);
	if (!this->m_glEBO) glGenBuffers(1, &this->m_glEBO);

	glBindVertexArray(this->m_glVAO);
	// Load data into vertex buffers
	glBindBuffer(GL_ARRAY_BUFFER, this->m_glVBO);
	// A great thing about structs is that their memory layout is sequential for all its items.
	// The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
	// again translates to 3/2 floats which translates to a byte array.
	glBufferData(GL_ARRAY_BUFFER, bufferVertices.size() * sizeof(Vertex), &bufferVertices[0], GL_STREAM_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->m_glEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, bufferIndices.size() * sizeof(GLuint), &bufferIndices[0], GL_STATIC_DRAW);

	// Set the vertex attribute pointers
	// Vertex Positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
	// Vertex Normals
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, norm));
	// Vertex Texture Coords
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, tex));

	glBindVertexArray(0);
}
