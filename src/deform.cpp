#include "deform.hpp"

#include "glm/gtc/epsilon.hpp"

const float EPS = 0.0001;

/*
  Given the unit vector v, find two unit vectos u and w such
  that u,v, and w together form an orthonormal basis.
*/

void FindBasis(const glm::vec3& v, glm::vec3& u, glm::vec3& w) {

    glm::vec3 a(1.0, 0.0, 0.0);

    if(all(glm::epsilonEqual(v, a, EPS ))) {
	a = glm::vec3(0.0f, 1.0, 0.0f);
    }

    u = glm::normalize( glm::cross(a,v) );
    w = glm::normalize(  glm::cross(u,v) );

}

template <typename F>
glm::vec3 Gradient(const F& f, const glm::vec3& p ) {
    const float D = 1e-5;
    return glm::vec3(
	(f(glm::vec3(p.x + D, p.y, p.z)) - f(glm::vec3(p.x - D, p.y, p.z))) / ( 2.0f * D ),
	(f(glm::vec3(p.x, p.y + D, p.z)) - f(glm::vec3(p.x, p.y - D, p.z))) / ( 2.0f * D ),
	(f(glm::vec3(p.x, p.y, p.z + D)) - f(glm::vec3(p.x, p.y, p.z - D))) / ( 2.0f * D ));
}

void ComputeNormals(Mesh& mesh) {
          for(size_t i = 0; i < mesh.vertices.size(); ++i) {
      mesh.normals.push_back( glm::vec3(0.0f, 0.0f, 0.0f) );
      }

      // sum all adjacent face normals for the vertices.
      for(size_t i = 0; i < mesh.indices.size(); i+=3) {
	  glm::vec3 p0 = mesh.vertices[mesh.indices[i+0]];
	  glm::vec3 p1 = mesh.vertices[mesh.indices[i+1]];
	  glm::vec3 p2 = mesh.vertices[mesh.indices[i+2]];

	  glm::vec3 u = p2 - p0;
	  glm::vec3 v = p1 - p0;

	  glm::vec3 fn = glm::normalize(glm::cross(u,v));


      mesh.normals[mesh.indices[i+0]] += fn;
      mesh.normals[mesh.indices[i+1]] += fn;
      mesh.normals[mesh.indices[i+2]] += fn;

      }

      for(size_t i = 0; i < mesh.vertices.size(); ++i) {
      mesh.normals[i] = glm::normalize(mesh.normals[i]);
      }
}


void SweepHelper(Mesh& mesh) {
    float r_i = 0.2;
    float r_o = 0.7;

    glm::vec3 startSweep(0.0f, 0.0f, 1.0f);
    glm::vec3   endSweep(0.0f, 0.0f, 3.5f);

    // the sweep curve.
    auto sweepCurve = [=](float t) {
	return
	(1.0f-t)*startSweep + t * endSweep

//	glm::vec3(0.0f, -0.9f * t*t, 0.0f );

//	glm::vec3(0.0f,0.5f,0.0f) * (float)sin(0.8f*2.0f*M_PI * t);
	; };


    const float STEP_LENGTH = 0.01;

    for(float t = 0.0f; t <= 1.0; t+=STEP_LENGTH) {

	float t2 = t+STEP_LENGTH;
	float t1 = t;

	glm::vec3 c = sweepCurve(t1);

	glm::vec3 v = sweepCurve(t2) - sweepCurve(t1);
	float deltaLength = glm::length(v);
	v = glm::normalize(v);

	glm::vec3 u;
	glm::vec3 w;

	FindBasis(v, u, w);

	auto e = [&](glm::vec3 x) { return glm::dot(u, x-c ); };
	auto f = [&](glm::vec3 x) { return glm::dot(w, x-c ); };

	auto b = [=](float rx) {
	    float a = (rx - r_i) / (r_o - r_i);
	    return 3*a*a*a*a - 4*a*a*a + 1;
	};


	auto p = [&](const glm::vec3& x) {
	    float rx = glm::length(x - c);

	    if(rx < r_i) {
		return e(x);
	    }else if(rx >= r_i && rx <= r_o) {
		return e(x) * b(rx);
	    } else {
		return 0.0f;
	    }
	};

	auto q = [&](const glm::vec3& x) {
	    float rx = glm::length(x - c);

	    if(rx < r_i) {
		return f(x);
	    }else if(rx >= r_i && rx <= r_o) {
		return f(x) * b(rx);
	    } else {
		return 0.0f;
	    }
	};

	for(glm::vec3& x : mesh.vertices) {

	    glm::vec3 grad_p = Gradient(p, x );
	    glm::vec3 grad_q = Gradient(q, x );

	    /*
	      float rx = glm::length(x - c);
	      float s = 1.0f;
	      if(rx < r_i) {

	      s *= deltaLength;
	      } else if(rx >= r_i && rx <= r_o) {
	      s *= b(rx);
	      } else {
	      s *= 0.0f;
	      }

	      // deformation field.
	      //glm::vec3 D = (glm::cross(div_e, div_f));
	      glm::vec3 D = s * v;
	    */

	    glm::vec3 D = deltaLength * glm::cross(grad_q, grad_p  );

	    x += D;

	}


    }

}


void Sweep(Mesh& mesh) {

//    SweepHelper(mesh);

    ComputeNormals(mesh);


    /*


      compute normals.

      for all edges, find this info: we need to have the indices of (c,d) for every triangle. (a,b) is already stored in edge ID.
      we keep this in a hash named edgeJacadcenties.(we find this as follows: iterate over all faces, for every edge in that
      face, we already have one of the opposite vertices, so add to pair! we do this for all faces).


      iterate over all faces:
           iterate over all edges, and add SORTED edge to stack(use set to make sure we process all edges only once)

      we keep a set of all added triangles.


      iterate until stack is empty:
           pop edge e of stack.
	   if e should be split according to criterion
	       split it. so compute midpoint m from (a and b) and add to list.
	       now use midpoint to create four new triangles. use all of a,b,c,d to compute normal of m.

	       now add all four new edges. BUT WHEN WE SPLIT, ENSURE THAT edgeJacadcenties is properly updated.

	   else
	       add both triangles(opposite to e), if necessary.

    */

}

/*
  we just have to use a central difference quotient to estimate the derivative of the middle equation in (3)!

  or no, we do it on the whole function (3), will be better for derivatives accross boundaries.
*/
