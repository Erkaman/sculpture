#include "half_edge_mesh.hpp"

#include <map>

using std::pair;
using std::map;

typedef pair<GLuint,GLuint> HalfEdgeId;


struct pair_hash {
    std::size_t operator () (const HalfEdgeId& p) const {
        int h1 = p.first;
        int h2 = p.second;

        return h1 ^ h2;
    }
};

HalfEdgeMesh::HalfEdgeMesh(const Mesh& mesh) {

    map< pair<GLuint, GLuint>, HalfEdgeIter > edges;
    map< GLuint, VertexIter > vertices;


    /*
      First iterate through all faces and create all faces and all half edges.

     */
    for(const Tri& tri : mesh.faces) {

	FaceIter face = m_faces.insert(m_faces.end(), Face()  );
	face->edge = m_halfEdges.end(); // initial value.

//	printf("Iterate tri: %d, %d, %d\n",  tri.i[0], tri.i[1], tri.i[2] );

	//  Create half-edges and make sure that all half-edges gets assigned the face to the left of it.
	for(int i = 0; i < 3; ++i) {

	    GLuint u = tri.i[(i+0)%3];
	    GLuint v = tri.i[(i+1)%3];

	    HalfEdgeId id = HalfEdgeId(u, v);

	    // sanity check.
	    if(edges.count(id)>0) {
		printf("SOMETHING IS RONG!!!\n");
		exit(1);
	    }



	    HalfEdgeIter halfEdge = m_halfEdges.insert(m_halfEdges.end(), HalfEdge() );


	    if(vertices.count(u) == 0) {
		// create vertex.
		glm::vec3 p = mesh.vertices[u];



		VertexIter vertex = m_vertices.insert(m_vertices.end(), Vertex(p) );
		vertices[u] = vertex;
		vertex->edge = halfEdge;
	    }

	    edges[id] = halfEdge;
	    halfEdge->face = face;

	    if(i == 0) {

		if(face->edge != m_halfEdges.end() ) {
		    printf("face->edge is not null!\n");
		    exit(1);
		}

		face->edge = halfEdge;
	    }
	}

//	printf("num half edges: %d\n", edges.size() );


	// make sure that every half-edge points to the next half-edge around the face.
	// also, each half-edge should point to its twin.
	for(int i = 0; i < 3; ++i) {

	    HalfEdgeId idCur = HalfEdgeId(
		tri.i[(i+0)%3],
		tri.i[(i+1)%3]);

	    HalfEdgeId idNext = HalfEdgeId(
		tri.i[(i+1)%3],
		tri.i[(i+2)%3]);

	    /*printf("idCur: %d, %d\n",  idCur.first, idCur.second );
	    printf("idNext: %d, %d\n",  idNext.first, idNext.second );

*/
	    if(edges.count(idCur) == 0 || edges.count(idNext) == 0) {
		printf("RRRRRONGGGGG!\n");
		exit(1);
	    }


	    edges[idCur]->next = edges[idNext];

	    // add vertex that half-edge points to.
	    edges[idCur]->vertex = vertices[idCur.second];


	    HalfEdgeId idCurTwin = HalfEdgeId(
		idCur.second,
		idCur.first);

	    if(edges.count(idCurTwin)>0) {

		edges[idCurTwin]->twin = edges[idCur];
		edges[idCur    ]->twin = edges[idCurTwin];

	    }


	}

    }

    printf("Faces: %ld\n", m_faces.size() );
    printf("HalfEdges: %ld\n", m_halfEdges.size() );
    printf("Vertices: %ld\n", m_vertices.size() );

}

int Face::NumEdges()const {
    int numEdges = 0;

    HalfEdgeIter edge = this->edge;
    HalfEdgeIter start = edge;

    do {

	edge = edge->next;
	++numEdges;

    } while(edge != start);

    return numEdges;
}


int Vertex::Degree()const {

    HalfEdgeCIter halfEdge = this->edge;
    int degree = 0;

    do {

	halfEdge = halfEdge->twin->next;
	++degree;

    }while(halfEdge != this->edge);

    return degree;
}


Mesh HalfEdgeMesh::ToMesh()const {

    Mesh mesh;

    map< VertexCIter, GLuint > verticesMap;

    GLuint index = 0;
    for(VertexCIter it = beginVertices(); it != endVertices(); ++it) {
	mesh.vertices.push_back(it->p);
	verticesMap[it] = index++;
    }

    for(FaceCIter it = beginFaces(); it != endFaces(); ++it) {

	HalfEdgeIter halfEdge = it->edge;

	Tri tri;
	GLuint i = 0;

	do {

	    tri.i[i++] = verticesMap[halfEdge->twin->vertex];
	    halfEdge = halfEdge->next;

	} while(halfEdge != it->edge);

	mesh.faces.push_back(tri);

    }

    return mesh;
}
