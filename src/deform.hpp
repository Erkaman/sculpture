#pragma once

#include "gl_common.hpp"



void Sweep(Mesh& mesh);


inline void ComputeNormals(Mesh& mesh) {
          for(size_t i = 0; i < mesh.vertices.size(); ++i) {
      mesh.normals.push_back( glm::vec3(0.0f, 0.0f, 0.0f) );
      }

      // sum all adjacent face normals for the vertices.
      for(size_t i = 0; i < mesh.indices.size(); i+=1) {
	  glm::vec3 p0 = mesh.vertices[mesh.indices[i].i[0]];
	  glm::vec3 p1 = mesh.vertices[mesh.indices[i].i[1]];
	  glm::vec3 p2 = mesh.vertices[mesh.indices[i].i[2]];

	  glm::vec3 u = p2 - p0;
	  glm::vec3 v = p1 - p0;

	  glm::vec3 fn = glm::normalize(glm::cross(u,v));


      mesh.normals[mesh.indices[i].i[0]] += fn;
      mesh.normals[mesh.indices[i].i[1]] += fn;
      mesh.normals[mesh.indices[i].i[2]] += fn;

      }

      for(size_t i = 0; i < mesh.vertices.size(); ++i) {
      mesh.normals[i] = glm::normalize(mesh.normals[i]);
      }
}
