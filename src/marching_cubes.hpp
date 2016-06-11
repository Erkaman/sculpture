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



inline int XyzToId(int x, int y, int z, int i, int resolution) {
    return
	(x + cubeVerticesTable[i][0])*resolution*resolution +
	(y + cubeVerticesTable[i][1])*resolution            +
	(z + cubeVerticesTable[i][2]);

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

    std::unordered_map<std::pair<int,int>, int ,pair_hash> edgeIndicesCache;


    // precompute all the density values for the entire grid:
    for(int x = 0; x < (resolution); ++x)
	for(int y = 0; y < (resolution); ++y)
	    for(int z = 0; z < (resolution); ++z) {

		densityValues[z+y*resolution+x*resolution*resolution] = density.eval(



		    bounds[0][0] + (x) * cellSizes[0],
		    bounds[0][1] + (y) * cellSizes[1],
		    bounds[0][2] + (z) * cellSizes[2]
		    );

	    }


    // we iterate through all the cells, and create geometry for them, one by one.
    for(int x = 0; x < (resolution-1); ++x)
	for(int y = 0; y < (resolution-1); ++y)
	    for(int z = 0; z < (resolution-1); ++z) {

		int cellIndex = 0;

		// compute the values at the cell vertices, and create the cell index.
		for(int i = 0; i < 8; ++i) {

		    gridCellValues[i] = densityValues[XyzToId(x, y, z, i, resolution)];

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
		    int i0 = XyzToId(x, y, z, e[0], resolution);
		    int i1 = XyzToId(x, y, z, e[1], resolution);
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

			glm::vec3 p;

			// to compute the vertex, we interpolate between the vertices at the edge-point.

			int j = 0;
			float e0 = bounds[0][j] + (x + cubeVerticesTable[ e[0]  ][j]) * cellSizes[j];
			float e1 = bounds[0][j] + (x + cubeVerticesTable[ e[1]  ][j]) * cellSizes[j];
			p.x = (e1-e0)*t + e0;

			++j;

			e0 = bounds[0][j] + (y + cubeVerticesTable[ e[0]  ][j]) * cellSizes[j];
			e1 = bounds[0][j] + (y + cubeVerticesTable[ e[1]  ][j]) * cellSizes[j];
			p.y = (e1-e0)*t + e0;

			++j;

			e0 = bounds[0][j] + (z + cubeVerticesTable[ e[0]  ][j]) * cellSizes[j];
			e1 = bounds[0][j] + (z + cubeVerticesTable[ e[1]  ][j]) * cellSizes[j];
			p.z = (e1-e0)*t + e0;


			// now add the interpolated vertex.
			edgeIndices[i] = index++;
			mesh.vertices.push_back( p  );

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
/*
  for(const glm::vec3& p : mesh.vertices ) {
  printf("p: %f, %f, %f\n", p.x, p.y, p.z );
  }
*/

    printf("indices: %ld\n", mesh.indices.size() );

    delete[] densityValues;

    return mesh;
}
