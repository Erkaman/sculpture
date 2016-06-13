#pragma once

#include "gl_common.hpp"

#include "glm/gtc/epsilon.hpp"

const float EPS = 0.0001;

/*
  Given the unit vector u, find two unit vectos v and w such
  that u,v, and w together form an orthonormal basis.
 */
/*
void FindBasis(const glm::vec3& v, glm::vec3& u, glm::vec3& w) {

    glm::vec3 a(1.0, 0.0, 0.0);

    if(all(glm::epsilonEqual(u, a, EPS ))) {
	a = glm::vec3(0.0f, 1.0, 0.0f);
    }

    v = glm::normalize( glm::cross(a,u) );
    w = glm::normalize(  glm::cross(u,v) );

}
*/

template <typename F>
glm::vec3 Gradient(F f, float x, float y, float z) {
    const float D = 1e-5;
    return
	glm::vec3(
	    (f(x + D, y, z) - f(x - D, y, z)) / ( 2.0f * D ),
	    (f(x, y + D, z) - f(x, y - D, z)) / ( 2.0f * D ),
	    (f(x, y, z + D) - f(x, y, z - D)) / ( 2.0f * D )
	    );
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

    float r_i = 0.1;
    float r_o = 0.3;

    glm::vec3 startSweep(0.0f, 0.0f, 0.9f);
    glm::vec3   endSweep(0.0f, 0.0f, 3.5f);

    // the sweep curve.
    auto sweepCurve = [=](float t) { return (1.0f-t)*startSweep + t * endSweep; };


    const float STEP_LENGTH = 0.01;

    for(float t = 0.0f; t <= 1.0; t+=STEP_LENGTH) {

	float t2 = t+STEP_LENGTH;
	float t1 = t;

	glm::vec3 c = sweepCurve(t1);

	glm::vec3 v = sweepCurve(t2) - sweepCurve(t1);
	float deltaLength = glm::length(v);

	/*
	printf("1: %s\n",  glm::to_string(sweepCurve(t1)).c_str() );
	printf("2: %s\n",  glm::to_string(sweepCurve(t2)).c_str() );

	printf("u: %s\n\n",  glm::to_string(u).c_str() );
*/

//	v = glm::normalize(v);
/*
	glm::vec3 u;
	glm::vec3 w;

	FindBasis(u, v, w);
	*/
/*
	glm::vec3 div_e = u;
	glm::vec3 div_f = w;
*/

//	printf("v: %s\n",  glm::to_string(v).c_str() );

/*
	printf("div_e: %s\n",  glm::to_string(div_e).c_str() );
	printf("div_f: %s\n",  glm::to_string(div_f).c_str() );
*/
//	printf("\n");




	for(glm::vec3& x : mesh.vertices) {

//	    printf("x: %s\n",  glm::to_string(x).c_str() );

	    float rx = glm::length(x - c);

//	    glm::vec3 div_p;
//	    glm::vec3 div_q;

	    float s = 1.0f;

	    if(rx < r_i) {
		// nothing
///		div_p = div_e;
//		div_q = div_f;

		s *= deltaLength;

	    } else if(rx >= r_i && rx <= r_o) {
		// scale

		float a = (rx - r_i) / (r_o - r_i);
		float bx = 3*a*a*a*a - 4*a*a*a + 1;

		s *= bx;

//		div_p = div_e * bx;
//		div_q = div_f * bx;

	    } else {
		s *= 0.0f;
		// set to zero.
//		div_p = glm::vec3(0.0f);
//		div_q = glm::vec3(0.0f);
		//	printf("2\n");


	    }

	    // deformation field.
	    //glm::vec3 D = (glm::cross(div_e, div_f));
	    glm::vec3 D = s * v;
	    x += D;

	}


    }


}

/*
we just have to use a central difference quotient to estimate the derivative of the middle equation in (3)!

    or no, we do it on the whole function (3), will be better for derivatives accross boundaries.
*/
