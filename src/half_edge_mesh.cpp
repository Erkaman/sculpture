#include "half_edge_mesh.hpp"

#include <map>

using std::pair;
using std::map;

typedef pair<GLuint,GLuint> HalfEdgeId;
typedef pair<GLuint,GLuint> EdgeId;

inline EdgeId Sort(GLuint x, GLuint y) {
    return x <= y ? EdgeId(x,y) : EdgeId(y,x);
}

struct pair_hash {
    std::size_t operator () (const HalfEdgeId& p) const {
        int h1 = p.first;
        int h2 = p.second;

        return h1 ^ h2;
    }
};

HalfEdgeMesh::HalfEdgeMesh(const Mesh& mesh) {

    map< pair<GLuint, GLuint>, HalfEdgeIter > addedHalfEdges;
    map< pair<GLuint, GLuint>, EdgeIter > addedEdges;
    map< GLuint, VertexIter > addedVertices;


    /*
      First iterate through all faces and create all faces and all half edges.

     */
    for(const Tri& tri : mesh.faces) {

	FaceIter face = NewFace();
	face->halfEdge = m_halfEdges.end(); // initial value.

//	printf("Iterate tri: %d, %d, %d\n",  tri.i[0], tri.i[1], tri.i[2] );

	//  Create half-edges and make sure that all half-edges gets assigned the face to the left of it.
	for(int i = 0; i < 3; ++i) {

	    GLuint u = tri.i[(i+0)%3];
	    GLuint v = tri.i[(i+1)%3];

	    HalfEdgeId halfEdgeId = HalfEdgeId(u, v);
	    EdgeId     edgeId     = Sort(u, v);

	    // sanity check.
	    if(addedHalfEdges.count(halfEdgeId)>0) {
		printf("SOMETHING IS RONG!!!\n");
		exit(1);
	    }




	    HalfEdgeIter halfEdge =
		NewHalfEdge();


	    EdgeIter edge;
	    if(addedEdges.count(edgeId) == 0) {
		// create edge on the fly:
		edge = NewEdge();
		addedEdges[edgeId] = edge;
		edge->halfEdge = halfEdge;
	    } else {
		edge = addedEdges[edgeId];
	    }




	    if(addedVertices.count(u) == 0) {
		// create vertex.
		glm::vec3 p = mesh.vertices[u];

		VertexIter vertex = NewVertex();
		vertex->p = p;

		addedVertices[u] = vertex;
		vertex->halfEdge = halfEdge;
	    }

	    addedHalfEdges[halfEdgeId] = halfEdge;
	    halfEdge->face = face;
	    halfEdge->edge = edge;

	    if(i == 0) {

		if(face->halfEdge != m_halfEdges.end() ) {
		    printf("face->edge is not null!\n");
		    exit(1);
		}

		face->halfEdge = halfEdge;
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
	    if(addedHalfEdges.count(idCur) == 0 || addedHalfEdges.count(idNext) == 0) {
		printf("RRRRRONGGGGG!\n");
		exit(1);
	    }


	    addedHalfEdges[idCur]->next = addedHalfEdges[idNext];

	    // add vertex located at the root of the half-edge
	    addedHalfEdges[idCur]->vertex = addedVertices[idCur.first];


	    HalfEdgeId idCurTwin = HalfEdgeId(
		idCur.second,
		idCur.first);

	    if(addedHalfEdges.count(idCurTwin)>0) {

		addedHalfEdges[idCurTwin]->twin = addedHalfEdges[idCur];
		addedHalfEdges[idCur    ]->twin = addedHalfEdges[idCurTwin];

	    }


	}

    }

    printf("Faces: %ld\n", m_faces.size() );
    printf("HalfEdges: %ld\n", m_halfEdges.size() );
    printf("Vertices: %ld\n", m_vertices.size() );
    printf("Edges: %ld\n", m_edges.size() );

}

int Face::NumEdges()const {
    int numEdges = 0;

    HalfEdgeIter h = this->halfEdge;
    HalfEdgeIter start = h;

    do {

	h = h->next;
	++numEdges;

    } while(h != start);

    return numEdges;
}


int Vertex::Degree()const {

    HalfEdgeCIter halfEdge = this->halfEdge;
    int degree = 0;

    do {

	halfEdge = halfEdge->twin->next;
	++degree;

    }while(halfEdge != this->halfEdge);

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

	HalfEdgeIter halfEdge = it->halfEdge;

	Tri tri;
	GLuint i = 0;

	do {

	    tri.i[i++] = verticesMap[halfEdge->vertex];
	    halfEdge = halfEdge->next;

	} while(halfEdge != it->halfEdge);

	mesh.faces.push_back(tri);

    }

    return mesh;
}

void Edge::GetEdgePoints(glm::vec3& a, glm::vec3& b) {
    a = halfEdge->vertex->p;
    b = halfEdge->twin->vertex->p;
}


void HalfEdgeMesh::Flip(EdgeIter e0) {

    // HALF EDGES
    HalfEdgeIter h0 = e0->halfEdge;
    HalfEdgeIter h1 = h0->next;
    HalfEdgeIter h2 = h1->next;

    HalfEdgeIter h3 = h0->twin;
    HalfEdgeIter h4 = h3->next;
    HalfEdgeIter h5 = h4->next;

    HalfEdgeIter h6 = h1->twin;
    HalfEdgeIter h7 = h2->twin;
    HalfEdgeIter h8 = h4->twin;
    HalfEdgeIter h9 = h5->twin;

    // VERTICES
    VertexIter v0 = h3->vertex;
    VertexIter v1 = h0->vertex;
    VertexIter v2 = h6->vertex;
    VertexIter v3 = h8->vertex;

    // EDGES
    EdgeIter e1 = h2->edge;
    EdgeIter e2 = h1->edge;
    EdgeIter e3 = h5->edge;
    EdgeIter e4 = h4->edge;

    // FACES
    FaceIter f0 = h0->face;
    FaceIter f1 = h3->face;


    // Update HALF-EDGES

    h0->next = h1;
    h0->twin = h3;
    h0->vertex = v2;
    h0->edge = e0;
    h0->face = f0;

    h1->next = h2;
    h1->twin = h9;
    h1->vertex = v3;
    h1->edge = e3;
    h1->face = f0;

    h2->next = h0;
    h2->twin = h6;
    h2->vertex = v0;
    h2->edge = e2;
    h2->face = f0;

    h3->next = h4;
    h3->twin = h0;
    h3->vertex = v3;
    h3->edge = e0;
    h3->face = f1;

    h4->next = h5;
    h4->twin = h7;
    h4->vertex = v2;
    h4->edge = e1;
    h4->face = f1;

    h5->next = h3;
    h5->twin = h8;
    h5->vertex = v1;
    h5->edge = e4;
    h5->face = f1;


    h6->next = h6->next;
    h6->twin = h2;
    h6->vertex = v2;
    h6->edge = e2;
    h6->face = h6->face;


    // now do outer half-edges.
    h7->next = h7->next;
    h7->twin = h4;
    h7->vertex = v1;
    h7->edge = e1;
    h7->face = h7->face;


    h8->next = h8->next;
    h8->twin = h5;
    h8->vertex = v3;
    h8->edge = e4;
    h8->face = h8->face;

    h9->next = h9->next;
    h9->twin = h1;
    h9->vertex = v0;
    h9->edge = e3;
    h9->face = h9->face;

    // VERTICES.
    v0->halfEdge = h2;
    v1->halfEdge = h5;
    v2->halfEdge = h4;
    v3->halfEdge = h1;

    // EDGES
    e0->halfEdge = h3;
    e1->halfEdge = h4;
    e2->halfEdge = h2;
    e3->halfEdge = h1;
    e4->halfEdge = h5;

    // FACES

    f0->halfEdge = h0;
    f1->halfEdge = h3;




    /*
    printf("v0 %s\n", v0->ToString().c_str() );
    printf("v1 %s\n", v1->ToString().c_str() );
    printf("v2 %s\n", v2->ToString().c_str() );
    printf("v3 %s\n", v3->ToString().c_str() );
*/
}
