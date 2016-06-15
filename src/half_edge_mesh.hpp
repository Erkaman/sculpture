#pragma once

#include "gl_common.hpp"

#include <list>

/*
  https://fgiesen.wordpress.com/2012/02/21/half-edge-based-mesh-representations-theory/

  http://www.leonardofischer.com/dcel-data-structure-c-plus-plus-implementation/
*/


class HalfEdge;
class Vertex;
class Face;

typedef std::list<HalfEdge>::iterator HalfEdgeIter;
typedef std::list<Face>::iterator FaceIter;
typedef std::list<Vertex>::iterator VertexIter;

typedef std::list<HalfEdge>::const_iterator HalfEdgeCIter;
typedef std::list<Face>::const_iterator FaceCIter;
typedef std::list<Vertex>::const_iterator VertexCIter;

struct HalfEdge {
    HalfEdgeIter twin;
    HalfEdgeIter next; // the next half-edge around the face.

    VertexIter vertex; // the vertex that this half-edge points to..

    FaceIter face; // the face to the left of this half-edge

};

struct Vertex {
    glm::vec3 p;

    HalfEdgeIter halfEdge; // one of the half-edges emanating from this vertex.

    Vertex(glm::vec3 p_):p(p_) {}

    int Degree()const;
};

struct Face {
    HalfEdgeIter halfEdge; // one of the half-edges bordering the face.

    // compute the number of edges in a face.
    int NumEdges()const;

};


// we need these, if we want to use the iterators with std::maps.
inline bool operator<( const HalfEdgeIter& i, const HalfEdgeIter& j ) { return &*i < &*j; }
inline bool operator<( const   VertexIter& i, const   VertexIter& j ) { return &*i < &*j; }
inline bool operator<( const     FaceIter& i, const     FaceIter& j ) { return &*i < &*j; }

inline bool operator<( const HalfEdgeCIter& i, const HalfEdgeCIter& j ) { return &*i < &*j; }
inline bool operator<( const   VertexCIter& i, const   VertexCIter& j ) { return &*i < &*j; }
inline bool operator<( const     FaceCIter& i, const     FaceCIter& j ) { return &*i < &*j; }


class HalfEdgeMesh {

public:



private:

    std::list<HalfEdge> m_halfEdges;
    std::list<Face> m_faces;
    std::list<Vertex> m_vertices;

public:

    HalfEdgeMesh(const Mesh& mesh);


    Mesh ToMesh()const;


    /*
      begin/end
    */
    HalfEdgeIter beginHalfEdges() { return m_halfEdges.begin(); }
    HalfEdgeIter   endHalfEdges() { return m_halfEdges.end(); }

    FaceIter beginFaces() { return m_faces.begin(); }
    FaceIter   endFaces() { return m_faces.end(); }

    VertexIter beginVertices() { return m_vertices.begin(); }
    VertexIter   endVertices() { return m_vertices.end(); }

    /*
      const begin/end
    */
    HalfEdgeCIter beginHalfEdges()const { return m_halfEdges.cbegin(); }
    HalfEdgeCIter   endHalfEdges()const { return m_halfEdges.cend(); }

    FaceCIter beginFaces()const { return m_faces.cbegin(); }
    FaceCIter   endFaces()const { return m_faces.cend(); }

    VertexCIter beginVertices()const { return m_vertices.cbegin(); }
    VertexCIter   endVertices()const { return m_vertices.cend(); }

};
