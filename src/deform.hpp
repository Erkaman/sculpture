#pragma once

#include "gl_common.hpp"

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
glm::vec3 Gradient(F f, glm::vec3 p ) {
    const float D = 1e-5;
    return glm::vec3(
	(f(glm::vec3(p.x + D, p.y, p.z)) - f(glm::vec3(p.x - D, p.y, p.z))) / ( 2.0f * D ),
	(f(glm::vec3(p.x, p.y + D, p.z)) - f(glm::vec3(p.x, p.y - D, p.z))) / ( 2.0f * D ),
	(f(glm::vec3(p.x, p.y, p.z + D)) - f(glm::vec3(p.x, p.y, p.z - D))) / ( 2.0f * D ));
}

void Sweep(Mesh& mesh) {

/*
    auto g = [](float x, float y, float z) {
	return 3.0f*x*x + 10.0f*y*y*y*y*y*  y*y*y*y*y + 1.0f*z;
    };

    glm::vec3 grad = Gradient(g, 3.0f, 4.0f, 5.0f );

    printf("x+: %f\n",  g(3.0f + 0.0001, 4.0f, 5.0f) );
    printf("x-: %f\n",  g(3.0f - 0.0001, 4.0f, 5.0f) );

    printf("g: %f\n",

	   (g(3.0f + 0.0001f, 4.0f, 5.0f) - g(3.0f - 0.0001f, 4.0f, 5.0f) ) / ( 2.0f * 0.0001f )

	);


    printf("grad: %s\n",  glm::to_string(grad).c_str() );


    exit(1);

*/

    // we sweep from (1,0,0) to (3,0,0)

    float r_i = 0.2;
    float r_o = 0.8;

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

	auto e = [=](glm::vec3 x) { return glm::dot(u, x-c ); };
	auto f = [=](glm::vec3 x) { return glm::dot(w, x-c ); };

	auto b = [=](float rx) {
	    float a = (rx - r_i) / (r_o - r_i);
	    return 3*a*a*a*a - 4*a*a*a + 1;
	};


	auto p = [=](glm::vec3 x) {
	    float rx = glm::length(x - c);

	    if(rx < r_i) {
		return e(x);
	    }else if(rx >= r_i && rx <= r_o) {
		return e(x) * b(rx);
	    } else {
		return 0.0f;
	    }
	};

	auto q = [=](glm::vec3 x) {
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

/*
we just have to use a central difference quotient to estimate the derivative of the middle equation in (3)!

    or no, we do it on the whole function (3), will be better for derivatives accross boundaries.
*/
