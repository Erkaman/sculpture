#include <stdio.h>


#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.hpp"
#include "gl_common.hpp"

#include "marching_cubes.hpp"


using std::vector;

using glm::vec3;

#ifndef M_PI
#define M_PI 3.14159
#endif

/**********************************************************************
 * Default shader programs
 *********************************************************************/

GLuint vao;

const char* vertex_shader_text =
    "#version 330\n"
    "layout(location = 0) in vec3 aPos;"

    "uniform mat4 uProject;\n"
    "uniform mat4 uView;\n"

    "out vec3 vPos;"

    "\n"
    "void main()\n"
    "{\n"
    "   vPos = aPos; "
    "   gl_Position = uProject * uView * vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\n";

const char* fragment_shader_text =
    "#version 330\n"
    "out vec4 color;\n"
    "in vec3 vPos;"

    "void main()\n"
    "{\n"
    "    color = vec4(abs(vPos), 1.0); \n"
    "}\n";

/*
std::vector<glm::vec3> my_map_vertices;
std::vector<GLuint> map_line_indices;
*/



Mesh mesh;

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

GLFWwindow* window;

float cameraTheta = 0.8f;
float cameraPhi = 0.8 * M_PI/2.0f;
float cameraR = 3.0;


glm::mat4 viewMatrix;

void updateViewMatrix() {

	glm::vec3 up(0.0f, 1.0f, 0.0f);
	glm::vec3 center(0.0f, 0.0f, 0.0f);
	glm::vec3 position(
	    cameraR * sinf(cameraTheta)*sinf(cameraPhi),
	    cameraR * cosf(cameraPhi),
	    cameraR * cosf(cameraTheta)*sinf(cameraPhi));

	viewMatrix = glm::lookAt(
	    position,
	    center,
	    up
	    );


}

float Capsule(float x_, float y_, float z_, glm::vec3 p0, glm::vec3 p1, float r) {

    // Source of the below formula:
    // see equation (4.40) of http://image.diku.dk/projects/media/kelager.06.pdf

    glm::vec3 x(x_,y_,z_);

    float t = - glm::dot(p0 - x, p1 - p0) / glm::dot(p1 - p0, p1 - p0);
    t = std::min(1.0f, std::max(0.0f,t));

    glm::vec3 q = p0 + t * (p1-p0);

    return glm::length(q - x) - r;
}

float Torus(float x, float y, float z, float R, float r) {

    return (R - sqrt(x*x + y*y) )*(R - sqrt(x*x + y*y) ) + z*z - r*r;

}

float Union(float v1, float v2) {
    return std::min(v1,v2);
}

vector<glm::vec3> points;

void InitSculpt() {
    for(float s = 0; s < 12.0f; s +=0.5f) {

	vec3 p(
	    cos(s / sqrt(2) ),
	    sin(s / sqrt(2) ),
	    s / sqrt(2)
	    );
	points.push_back(p);

    }
}

struct Density {

    float eval(float x, float y, float z) const{

	float v = FLT_MAX;

//	v = x*x + y*y + z*z - 1;



	for(int i = 1; i < points.size(); ++i) {
	    	v = Union(v, Capsule(x,y,z, points[i-1] , points[i-0], 0.6));

		}



	/*
	v = Union(v, Capsule(x,y,z, glm::vec3(0,-5,0), glm::vec3(0,3,0), 0.3));
	v = Union(v, Capsule(x,y,z, glm::vec3(-3,3,0), glm::vec3(3,3,0), 1.0) );
	v = Union(v, Capsule(x,y,z, glm::vec3(-3,-5,-3), glm::vec3(3,-5,3), 1.0) );
	*/

	return v;



//	return torus(x,y,z, 3, 1);

    }
};


void init_map(void)
{

    Density d;
    mesh = MarchingCubes(d,
		  50,
		  -10, +10,
		  -10, +10,
		  -10, +10

	);


    /*
    mesh.vertices.push_back(glm::vec3(1,0,0));
    mesh.vertices.push_back(glm::vec3(1,0,1));
    mesh.vertices.push_back(glm::vec3(0,0,0));

    mesh.indices.push_back(2);
    mesh.indices.push_back(1);
    mesh.indices.push_back(0);
    */

}

/**********************************************************************
 * OpenGL helper functions
 *********************************************************************/

/* Create VBO, IBO and VAO objects for the heightmap geometry and bind them to
 * the specified program object
 */
void make_mesh(){
    GL_C(glGenBuffers(1, &mesh.indexVbo));
    /* Prepare the data for drawing through a buffer inidices */
    GL_C(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexVbo));
    GL_C(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)* mesh.indices.size()
		      , mesh.indices.data(), GL_STATIC_DRAW));

    GL_C(glGenBuffers(1, &mesh.vertexVbo));
    GL_C(glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexVbo));
    GL_C(glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*3*mesh.vertices.size(),

		      mesh.vertices.data() , GL_STATIC_DRAW));


    GL_C(glEnableVertexAttribArray(0));
    GL_C(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0));
}
/**********************************************************************
 * GLFW callback functions
 *********************************************************************/

void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset){
    cameraR += yoffset;
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods){
    switch(key){
        case GLFW_KEY_ESCAPE:
            /* Exit program on Escape */
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
    }
}



void InitGlfw() {
    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Sculpture", NULL, NULL);
    if (! window ) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetScrollCallback(window, ScrollCallback);
    glfwSetKeyCallback(window, KeyCallback);

    glfwMakeContextCurrent(window);

    // load GLAD.
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // Bind and create VAO, otherwise, we can't do anything in OpenGL.

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);


}





int main(int argc, char** argv)
{

    InitSculpt();



    InitGlfw();

    Shader shader(vertex_shader_text, fragment_shader_text);

    // projection matrix.
    glm::mat4 projectionMatrix = glm::perspective(0.9f, (float)WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 1000.0f);


    /* Create mesh data */
    init_map();
    make_mesh();



    double prevMouseX = 0;
    double prevMouseY = 0;

    double curMouseX = 0;
    double curMouseY = 0;




	GL_C(glEnable(GL_CULL_FACE));
	GL_C(glEnable(GL_DEPTH_TEST));

	glFlush();
	glFinish();


    while (!glfwWindowShouldClose(window)) {

	int fbWidth, fbHeight;
	glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
	GL_C(glViewport(0, 0, fbWidth, fbHeight));
	GL_C(glClearColor(0.0f, 0.0f, 1.0f, 0.0f));
        GL_C(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

	shader.Bind();


	updateViewMatrix();

	shader.SetUniform("uProject", projectionMatrix );
	shader.SetUniform("uView",viewMatrix );



	GL_C(glDrawElements(GL_TRIANGLES, mesh.indices.size() , GL_UNSIGNED_INT, 0));


	prevMouseX = curMouseX;
	prevMouseY = curMouseY;
	glfwGetCursorPos(window, &curMouseX, &curMouseY);

	const float MOUSE_SENSITIVITY = 0.005;


	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	if (state == GLFW_PRESS) {

	    cameraTheta += (curMouseX - prevMouseX ) * MOUSE_SENSITIVITY;
	    cameraPhi += (curMouseY - prevMouseY ) * MOUSE_SENSITIVITY;
	}

//	printf("delta x: %f\n",  curMouseX - prevMouseX );


        /* display and process events through callbacks */
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
