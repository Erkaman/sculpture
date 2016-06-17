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

	printf("p: %s\n", glm::to_string(halfEdge->twin->vertex->p).c_str() );
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

void HalfEdgeMesh::Split(EdgeIter e0) {

    // FIRST WE COLLECT INFO

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

    // ALLOCATE NEW

    // HALF EDGES
    HalfEdgeIter h10 = NewHalfEdge();
    HalfEdgeIter h11 = NewHalfEdge();
    HalfEdgeIter h12 = NewHalfEdge();
    HalfEdgeIter h13 = NewHalfEdge();
    HalfEdgeIter h14 = NewHalfEdge();
    HalfEdgeIter h15 = NewHalfEdge();

    // VERTICES
    VertexIter v4 = NewVertex();
    glm::vec3 m = (e0->halfEdge->vertex->p + e0->halfEdge->twin->vertex->p) * 0.5f;
    v4->p = m;

//    glm::vec3 m = e0->halfEdge->vertex->p;

//    printf("m :%s\n", glm::to_string(m).c_str() );

    // EDGES
    EdgeIter e5 = NewEdge();
    EdgeIter e6 = NewEdge();
    EdgeIter e7 = NewEdge();

    // FACES
    FaceIter f2 = NewFace();
    FaceIter f3 = NewFace();


    // NOW WE START ASSIGNING
    h0->next = h11;
    h0->twin = h3;
    h0->vertex = v1;
    h0->edge = e0;
    h0->face = f0;

    h1->next = h12;
    h1->twin = h6;
    h1->vertex = v0;
    h1->edge = e2;
    h1->face = f3;

    h2->next = h0;
    h2->twin = h7;
    h2->vertex = v2;
    h2->edge = e1;
    h2->face = f0;

    h3->next = h4;
    h3->twin = h0;
    h3->vertex = v4;
    h3->edge = e0;
    h3->face = f1;

    h4->next = h10;
    h4->twin = h8;
    h4->vertex = v1;
    h4->edge = e4;
    h4->face = f1;

    h5->next = h14;
    h5->twin = h9;
    h5->vertex = v3;
    h5->edge = e3;
    h5->face = f2;

    h6->next = h6->next;
    h6->twin = h1;
    h6->vertex = v2;
    h6->edge = e2;
    h6->face = h6->face;

    h7->next = h7->next;
    h7->twin = h2;
    h7->vertex = v1;
    h7->edge = e1;
    h7->face = h7->face;

    h8->next = h8->next;
    h8->twin = h4;
    h8->vertex = v3;
    h8->edge = e4;
    h8->face = h8->face;

    h9->next = h9->next;
    h9->twin = h5;
    h9->vertex = v0;
    h9->edge = e3;
    h9->face = h9->face;

    h10->next = h3;
    h10->twin = h15;
    h10->vertex = v3;
    h10->edge = e6;
    h10->face = f1;

    h11->next = h2;
    h11->twin = h12;
    h11->vertex = v4;
    h11->edge = e5;
    h11->face = f0;

    h12->next = h13;
    h12->twin = h11;
    h12->vertex = v2;
    h12->edge = e5;
    h12->face = f3;

    h13->next = h1;
    h13->twin = h14;
    h13->vertex = v4;
    h13->edge = e7;
    h13->face = f3;

    h14->next = h15;
    h14->twin = h13;
    h14->vertex = v0;
    h14->edge = e7;
    h14->face = f2;

    h15->next = h5;
    h15->twin = h10;
    h15->vertex = v4;
    h15->edge = e6;
    h15->face = f2;

    // VERTICES
    v0->halfEdge = h1;
    v1->halfEdge = h0;
    v2->halfEdge = h2;
    v3->halfEdge = h5;
    v4->halfEdge = h3;

    // EDGES
    e0->halfEdge = h0;
    e1->halfEdge = h2;
    e2->halfEdge = h1;
    e3->halfEdge = h5;
    e4->halfEdge = h4;

    e5->halfEdge = h11;
    e6->halfEdge = h10;
    e7->halfEdge = h13;


    // FACES
    f0->halfEdge = h2;
    f1->halfEdge = h3;
    f2->halfEdge = h5;
    f3->halfEdge = h1;


}

VertexIter HalfEdgeMesh::Collapse(EdgeIter e8) {

    // FIRST WE COLLECT INFO

    // HALF EDGES

    // f4
    HalfEdgeIter h18 = e8->halfEdge;
    HalfEdgeIter h16 = h18->next;
    HalfEdgeIter h17 = h16->next;

    // f5
    HalfEdgeIter h19 = h18->twin;
    HalfEdgeIter h20 = h19->next;
    HalfEdgeIter h21 = h20->next;

    // f6
    HalfEdgeIter h26 = h21->twin;
//    HalfEdgeIter h28 = h26->next->next;

    // f7
    HalfEdgeIter h23 = h16->twin;
    HalfEdgeIter h24 = h23->next;
//    HalfEdgeIter h22 = h24->next;

    // f8
//    HalfEdgeIter h31 = h24->twin;
    HalfEdgeIter h32 = h24->twin->next;
//    HalfEdgeIter h30 = h32->next;

    // f9
    HalfEdgeIter h35 = h26->next->next->twin;
//    HalfEdgeIter h36 = h35->next;
//    HalfEdgeIter h34 = h36->next;

    // f2
    HalfEdgeIter h10 = h17->twin;
//    HalfEdgeIter h8  = h10->next;
//    HalfEdgeIter h9  = h8->next;

    // f3
    HalfEdgeIter h12 = h20->twin;
//    HalfEdgeIter h13 = h12->next;
//    HalfEdgeIter h14 = h13->next;

    // f1
/*    HalfEdgeIter h4  = h13->twin;
    HalfEdgeIter h5  =  h4->next;
    HalfEdgeIter h6  =  h5->next;
*/
    // f0, not affected
    /*  HalfEdgeIter h2  =  h9->twin;
    HalfEdgeIter h0  =  h2->next;
    HalfEdgeIter h1  =  h0->next;
*/

    // outer edges:
/*    HalfEdgeIter h3  =  h0->twin;
    HalfEdgeIter h7  =  h6->twin;
    HalfEdgeIter h15  =  h14->twin;
    HalfEdgeIter h29  =  h27->twin;
    HalfEdgeIter h37  =  h36->twin;
    HalfEdgeIter h33  =  h30->twin;
    HalfEdgeIter h25  =  h22->twin;
    HalfEdgeIter h11  =  h8->twin;
*/


    // VERTICES:
//    VertexIter v0 = h0->vertex;
    VertexIter v1 = h17->vertex;
//    VertexIter v2 = h22->vertex;

//    VertexIter v3 = h1->vertex;
    VertexIter v4 = h18->vertex;
    VertexIter v5 = h24->vertex;
//    VertexIter v6 = h30->vertex;

//    VertexIter v7 = h4->vertex;
    VertexIter v8 = h12->vertex;
//    VertexIter v9 = h28->vertex;


    // EDGES
//    EdgeIter e0 = h3->edge;
//    EdgeIter e1 = h1->edge;
//    EdgeIter e2 = h6->edge;
//    EdgeIter e3 = h11->edge;
//    EdgeIter e4 = h2->edge;
//    EdgeIter e5 = h4->edge;
//    EdgeIter e6 = h14->edge;
    EdgeIter e7 = h10->edge;
    EdgeIter e9 = h12->edge;
//    EdgeIter e10 = h22->edge;
    EdgeIter e11 = h23->edge;
    EdgeIter e12 = h26->edge;
//    EdgeIter e13 = h27->edge;
    EdgeIter e14 = h24->edge;
    EdgeIter e15 = h32->edge;
    EdgeIter e16 = h35->edge;
//    EdgeIter e17 = h33->edge;
//    EdgeIter e18 = h36->edge;

    // FACES
//    FaceIter f0 = h0->face;
//    FaceIter f1 = h4->face;
//    FaceIter f2 = h8->face;
//    FaceIter f3 = h13->face;
    FaceIter f4 = h17->face;
    FaceIter f5 = h19->face;
    FaceIter f6 = h26->face;
//    FaceIter f7 = h22->face;
    FaceIter f8 = h32->face;
//    FaceIter f9 = h34->face;

    glm::vec3 m = (v4->p + v5->p) * 0.5f;
//    v4->p = m;

    v4->p = m;

    // NOW WE START ASSIGNING.

    // HALF EDGES

/*    h0->next = h1;
    h0->twin = h3;
    h0->vertex = v0;
    h0->edge = e0;
    h0->face = f0;

    h1->next = h2;
    h1->twin = h5;
    h1->vertex = v3;
    h1->edge = e1;
    h1->face = f0;

    h2->next = h0;
    h2->twin = h9;
    h2->vertex = v4;
    h2->edge = e4;
    h2->face = f0;

    h3->next = h3->next;
    h3->twin = h0;
    h3->vertex = v3;
    h3->edge = e0;
    h3->face = h3->face;

    h4->next = h5;
    h4->twin = h13;
    h4->vertex = v7;
    h4->edge = e5;
    h4->face = f1;

    h5->next = h6;
    h5->twin = h1;
    h5->vertex = v4;
    h5->edge = e1;
    h5->face = f1;

    h6->next = h4;
    h6->twin = h7;
    h6->vertex = v3;
    h6->edge = e2;
    h6->face = f1;

    h7->next = h7->next;
    h7->twin = h6;
    h7->vertex = v7;
    h7->edge = e2;
    h7->face = h7->face;

    h8->next = h9;
    h8->twin = h11;
    h8->vertex = v1;
    h8->edge = e3;
    h8->face = f2;

    h9->next = h10;
    h9->twin = h2;
    h9->vertex = v0;
    h9->edge = e4;
    h9->face = f2;
*/
//    h10->next = h8;
    h10->twin = h23;
    h10->vertex = v4;
    h10->edge = e7;
//    h10->face = f2;
/*
    h11->next = h11->next;
    h11->twin = h8;
    h11->vertex = v0;
    h11->edge = e3;
    h11->face = h11->face;
*/
//    h12->next = h13;
    h12->twin = h26;
    h12->vertex = v8;
    h12->edge = e9;
//    h12->face = f3;
/*
    h13->next = h14;
    h13->twin = h4;
    h13->vertex = v4;
    h13->edge = e5;
    h13->face = f3;

    h14->next = h12;
    h14->twin = h15;
    h14->vertex = v7;
    h14->edge = e6;
    h14->face = f3;

    h15->next = h15->next;
    h15->twin = h14;
    h15->vertex = v8;
    h15->edge = e6;
    h15->face = h15->face;
*/
    // h16 will be removed.
    // h17 will be removed.
    // h18 will be removed.

    // h19 will be removed.
    // h20 will be removed.
    // h21 will be removed.
/*
    h22->next = h23;
//    h22->twin = h25;
    h22->vertex = v2;
    h22->edge = e10;
    h22->face = f7;
*/
    h23->next = h24;
    h23->twin = h10;
    h23->vertex = v1;
    h23->edge = e7;
//    h23->face = f7;

//    h24->next = h22;
//    h24->twin = h31;
    h24->vertex = v4;
    h24->edge = e14;
//    h24->face = f7;
/*
    h25->next = h25->next;
    h25->twin = h22;
    h25->vertex = v1;
    h25->edge = e10;
    h25->face = h25->face;
*/
//    h26->next = h27;
    h26->twin = h12;
    h26->vertex = v4;
    h26->edge = e9;
    h26->face = f6;
/*
    h27->next = h28;
//    h27->twin = h29;
    h27->vertex = v8;
    h27->edge = e13;
    h27->face = f6;
*/
/*    h28->next = h26;
    h28->twin = h35;
    h28->vertex = v9;
    h28->edge = e16;
    h28->face = f6;

    h29->next = h29->next;
    h29->twin = h27;
    h29->vertex = v9;
    h29->edge = e13;
    h29->face = h29->face;
*/
/*
    h30->next = h31;
    h30->twin = h33;
    h30->vertex = v6;
    h30->edge = e17;
    h30->face = f8;

    h31->next = h32;
    h31->twin = h24;
    h31->vertex = v2;
    h31->edge = e14;
    h31->face = f8;
*/
//    h32->next = h30;
//    h32->twin = h34;
    h32->vertex = v4;
    h32->edge = e15;
    h32->face = f8;
/*
    h33->next = h33->next;
    h33->twin = h30;
    h33->vertex = v2;
    h33->edge = e17;
    h33->face = h33->face;

    h34->next = h35;
    h34->twin = h32;
    h34->vertex = v6;
    h34->edge = e15;
    h34->face = f9;
*/
//    h35->next = h36;
//    h35->twin = h28;
    h35->vertex = v4;
    h35->edge = e16;
//    h35->face = f9;
/*
    h36->next = h34;
    h36->twin = h37;
    h36->vertex = v9;
    h36->edge = e18;
    h36->face = f9;

    h37->next = h37->next;
    h37->twin = h36;
    h37->vertex = v6;
    h37->edge = e18;
    h37->face = h37->face;
*/
    // VERTICES
//    v0->halfEdge = h0;
    v1->halfEdge = h23;
//    v2->halfEdge = h22;
//    v3->halfEdge = h1;
    v4->halfEdge = h10;
    // v5 will be removed
//    v6->halfEdge = h30;
//    v7->halfEdge = h4;
//    v8->halfEdge = h27;
//    v9->halfEdge = h36;

    // EDGES
//    e0->halfEdge = h0;
//    e1->halfEdge = h1;
//    e2->halfEdge = h6;
//    e3->halfEdge = h8;
//    e4->halfEdge = h9;
//    e5->halfEdge = h13;
//    e6->halfEdge = h14;
    e7->halfEdge = h10;
    // e8 will be removed
    e9->halfEdge = h12;
//    e10->halfEdge = h22;
    // e11 will be removed
    // e12 will be removed
//    e13->halfEdge = h27;
    e14->halfEdge = h24;
    e15->halfEdge = h32;
//    e16->halfEdge = h28;
//    e17->halfEdge = h30;
//    e18->halfEdge = h36;


    // FACES
//    f0->halfEdge = h0;
//    f1->halfEdge = h5;
//    f2->halfEdge = h8;
//    f3->halfEdge = h13;
    // f4 will be removed
    // f5 will be removed
    f6->halfEdge = h26;
//    f7->halfEdge = h24;
    f8->halfEdge = h32;
//    f9->halfEdge = h34;

    RemoveFace(f4);
    RemoveFace(f5);

    RemoveVertex(v5);

    RemoveEdge(e8);
    RemoveEdge(e11);
    RemoveEdge(e12);

    RemoveHalfEdge(h16);
    RemoveHalfEdge(h17);
    RemoveHalfEdge(h18);
    RemoveHalfEdge(h19);
    RemoveHalfEdge(h20);
    RemoveHalfEdge(h21);


    // return the vertex that the edge was collapsed into:
    return v4;
}
