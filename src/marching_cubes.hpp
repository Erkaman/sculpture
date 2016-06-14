#pragma once

#include "marching_cubes_tables.hpp"

#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <functional>


inline std::pair<int,int> sort(int x, int y) {
    return x <= y ? std::pair<int,int>(x,y) : std::pair<int,int>(y,x);
}



struct pair_hash {
    std::size_t operator () (const std::pair<int,int>& p) const {
        int h1 = p.first;
        int h2 = p.second;

        // Mainly for demonstration purposes, i.e. works but is overly simple
        // In the real world, use sth. like boost.hash_combine
        return h1 ^ h2;
    }
};

inline int XyzToId(int* C, int resolution) {
    return
	(C[0])*resolution*resolution +
	(C[1])*resolution            +
	(C[2]);
}

inline int XyzToId(int* C, int i, int resolution) {
    return
	(C[0] + cubeVerticesTable[i][0])*resolution*resolution +
	(C[1] + cubeVerticesTable[i][1])*resolution            +
	(C[2] + cubeVerticesTable[i][2]);
}


template<typename F>
Mesh MarchingCubes(
    const F& density,

    // how many grid-vertices there are per axis. So the total number of cells is
    // (resolution-1)^3
    const int resolution,

    const float xMin, const float xMax,
    const float yMin, const float yMax,
    const float zMin, const float zMax
    ) {

    float bounds[2][3] = {
	{ xMin, yMin, zMin },
	{ xMax, yMax, zMax },

    };

    Mesh mesh;

    // the sizes of the cells.
    float cellSizes[3];

    for(int i = 0; i < 3; ++i) {
	cellSizes[i] = (bounds[1][i] - bounds[0][i]) / (float)(resolution-1);
    }

    float gridCellValues[8];
    int edgeIndices[12];

    GLuint index = 0;


    float* densityValues = new float[resolution*resolution*resolution];
    float* smoothedDensityValues = new float[resolution*resolution*resolution];

    glm::vec3* normals = new glm::vec3[resolution*resolution*resolution];

    std::unordered_map<std::pair<int,int>, int ,pair_hash> edgeIndicesCache;

    // Represents (x,y,z)
    int C[3];
    int A[3];
    int B[3];

    // precompute all the density values for the entire grid:
    for(C[0] = 0; C[0] < (resolution); ++C[0])
	for(C[1] = 0; C[1] < (resolution); ++C[1])
	    for(C[2] = 0; C[2] < (resolution); ++C[2]) {

		densityValues[
		    XyzToId(C, resolution)] = density.eval(
		    bounds[0][0] + (C[0]) * cellSizes[0],
		    bounds[0][1] + (C[1]) * cellSizes[1],
		    bounds[0][2] + (C[2]) * cellSizes[2]
		    );

	    }



    int offsetTable[3][3] =
	{
	    {1,0,0},
	    {0,1,0},
	    {0,0,1},
	};
/*
    for(C[0] = 2; C[0] < (resolution-2); ++C[0])
	for(C[1] = 2; C[1] < (resolution-2); ++C[1])
	    for(C[2] = 2; C[2] < (resolution-2); ++C[2]) {

		float sum = 0;

		for(int i = -2; i <= +2; ++i) {


		    for(int j = -2; j <= +2; ++j) {


			for(int k = -2; k <= +2; ++k) {

			    A[0] = C[0] + i;
			    A[1] = C[1] + j;
			    A[2] = C[2] + k;

			    sum += densityValues[XyzToId(A, resolution)];
			}

		    }

		}

		smoothedDensityValues[XyzToId(C, resolution)] = sum / 125.0f;


	    }
*/

    //  int F[3];

    for(C[0] = 1; C[0] < (resolution-1); ++C[0])
	for(C[1] = 1; C[1] < (resolution-1); ++C[1])
	    for(C[2] = 1; C[2] < (resolution-1); ++C[2]) {



		for(int j = 0; j < 3; ++j) {

		    int* offsets = offsetTable[j];

		    A[0] = C[0] + offsets[0];
		    A[1] = C[1] + offsets[1];
		    A[2] = C[2] + offsets[2];

		    B[0] = C[0] - offsets[0];
		    B[1] = C[1] - offsets[1];
		    B[2] = C[2] - offsets[2];

		    float a = densityValues[XyzToId(A, resolution)];

		    float b = densityValues[XyzToId(B, resolution)];

		    float g = 0.5f * (a - b);

		    //	    printf(": %f\n", g);

		    normals[XyzToId(C, resolution)][j] = g;

		}




		normals[XyzToId(C, resolution)] = glm::normalize(normals[XyzToId(C, resolution)]);

		glm::vec3 n0 = normals[XyzToId(C, resolution)];

		//	printf("n0: %f, %f, %f\n",  n0[0], n0[1], n0[2]);



	    }




    // we iterate through all the cells, and create geometry for them, one by one.
    for(C[0] = 0; C[0] < (resolution-1); ++C[0])
	for(C[1] = 0; C[1] < (resolution-1); ++C[1])
	    for(C[2] = 0; C[2] < (resolution-1); ++C[2]) {

		int cellIndex = 0;

		// compute the values at the cell vertices, and create the cell index.
		for(int i = 0; i < 8; ++i) {

		    gridCellValues[i] = densityValues[XyzToId(C, i, resolution)];

		    if( gridCellValues[i] > 0 ) {
			cellIndex |= ( 1 << i );
		    }
		}

		int edgeTableMask = edgeTable[cellIndex];

		if(edgeTableMask == 0)
		    continue; // no geometry in this cell!


		//  for all edges where the surface passes through, we create vertices.
		// and we keep track of the indices for the vertices through the
		// array edgeIndices
		for(int i = 0; i < 12; ++i) {

		    if(  ((1 << i) & edgeTableMask) == 0 )
			continue; // no geometry!

		    int* e = edges[i];


		    /*
		      Only one interpolated vertex between every edge is necessary.
		      Once we have interpolated and computed one such vertex,
		      we save its index in the hashmap edgeIndicesCache.

		      By doing this, the vertexcount of the created geometry is
		      MUCH lowered.
		     */
		    int i0 = XyzToId(C, e[0], resolution);
		    int i1 = XyzToId(C, e[1], resolution);
		    std::pair<int,int> pair = sort(i0, i1);
		    auto value = edgeIndicesCache.find(pair);

		    if(value !=  edgeIndicesCache.end() ) {
			// we have already computed the vertex between this edge.
			// so reuse it.

			edgeIndices[i] = value->second;

		    } else {

			// compute the lerp-factor t.
			float v0 = gridCellValues[e[0]];
			float v1 = gridCellValues[e[1]];
			float d = v0 - v1;
			float t = 0.0;
			if(fabs(d) > 0.00001) {
			    t =v0 / d;
			}

			float p[3];

			// to compute the vertex, we interpolate between the vertices at the edge-point.

			for(int j = 0; j < 3; ++j) {
			    float e0 = bounds[0][j] + (C[j] + cubeVerticesTable[ e[0]  ][j]) * cellSizes[j];
			    float e1 = bounds[0][j] + (C[j] + cubeVerticesTable[ e[1]  ][j]) * cellSizes[j];
			    p[j] = (e1-e0)*t + e0;
			}


			glm::vec3 n0 = normals[i0];
			glm::vec3 n1 = normals[i1];

			//printf("n0: %f, %f, %f\n",  n0[0], n0[1], n0[2]);

			glm::vec3 n = glm::normalize((n1-n0)*t + n0);


			// now add the interpolated vertex.
			edgeIndices[i] = index++;
			mesh.vertices.push_back( glm::vec3(p[0], p[1], p[2])  );
			mesh.normals.push_back( n  );

			edgeIndicesCache[pair] = edgeIndices[i];

		    }



		}

		int* tri = triTable[cellIndex];

		// finally, we now create all the triangle faces.
		for(int i = 0; i < 16; i+=3) {

		    if(tri[i] == -1)
			break; // no more triangles!

		    int i0 = edgeIndices[ tri[i+0] ];
		    int i1 = edgeIndices[ tri[i+1] ];
		    int i2 = edgeIndices[ tri[i+2] ];

		    mesh.indices.push_back(i0);
		    mesh.indices.push_back(i1);
		    mesh.indices.push_back(i2);

		}
	    }


    printf("vertices: %ld\n", mesh.vertices.size() );

    printf("normals: %ld\n", mesh.normals.size() );

/*
  for(const glm::vec3& p : mesh.vertices ) {
  printf("p: %f, %f, %f\n", p.x, p.y, p.z );
  }
*/

    printf("indices: %ld\n", mesh.indices.size() );

    delete[] densityValues;

    return mesh;
}
