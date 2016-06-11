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
    "layout(location = 0) in vec3 aPosition;"
    "layout(location = 1) in vec3 aNormal;"

    "uniform mat4 uProject;\n"
    "uniform mat4 uView;\n"

    "out vec3 vPosition;"
    "out vec3 vNormal;"

    "\n"
    "void main()\n"
    "{\n"
    "   vPosition = aPosition; "
    "   vNormal = aNormal; "

    "   gl_Position = uProject * uView * vec4(aPosition, 1.0);\n"
    "}\n";

const char* fragment_shader_text =
    "#version 330\n"
    "out vec4 color;\n"
    "in vec3 vPosition;"
    "in vec3 vNormal;"

    "uniform vec3 uEyePos;"


    "void main()\n"
    "{\n"

    "vec3 diffuseColor = vec3(0.42, 0.34, 0.0);"
    "vec3 ambientLight = vec3(0.87, 0.82, 0.69);"
    "vec3 lightColor = vec3(0.40, 0.47, 0.0);"
    "vec3 lightDir = normalize(vec3(-0.69, 1.33, 0.57));"
    "float specularPower = 12.45;"

    "vec3 n = vNormal;"
    "vec3 l = normalize(lightDir);"
    "vec3 v = normalize(uEyePos - vPosition);"
    "vec3 ambient = ambientLight * diffuseColor;"
    "vec3 diffuse = diffuseColor * lightColor * dot(n, l) ;"
    "vec3 specular = vec3(pow(clamp(dot(normalize(l+v),n),0.0,1.0)  , specularPower));"


    "    vec3 diff = ambient + diffuse + specular; \n"

    "    color = vec4(diff, 1.0); \n"
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

glm::vec3 cameraPos;

glm::mat4 viewMatrix;

void updateViewMatrix() {

    glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::vec3 center(0.0f, 0.0f, 0.0f);
    cameraPos = glm::vec3(
	cameraR * sinf(cameraTheta)*sinf(cameraPhi),
	cameraR * cosf(cameraPhi),
	cameraR * cosf(cameraTheta)*sinf(cameraPhi));

    viewMatrix = glm::lookAt(
	cameraPos,
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
    for(float s = 0; s < 16.0f; s +=0.5f) {

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

	    float t= 0.5 + 0.5*sin( i * 0.8f );

	    float r = 0.3 + (0.9 - 0.3) * t;
	    r = 0.6;


	    v = Union(v, Capsule(x,y,z, points[i-1] , points[i-0],

				 r


			  ));


	}





/*
  v = Union(v, Capsule(x,y,z, glm::vec3(0,-5,0), glm::vec3(0,3,0), 0.3));
  v = Union(v, Capsule(x,y,z, glm::vec3(-3,3,0), glm::vec3(3,3,0), 1.0) );	v = Union(v, Capsule(x,y,z, glm::vec3(-3,-5,-3), glm::vec3(3,-5,3), 1.0) );
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
			 -10,  +10,
			 -10, +10
	);


    /*
      for(size_t i = 0; i < mesh.vertices.size(); ++i) {
      mesh.normals.push_back( glm::vec3(0.0f, 0.0f, 0.0f) );
      }

      // sum all adjacent face normals for the vertices.
      for(size_t i = 0; i < mesh.indices.size(); i+=3) {
      vec3 p0 = mesh.vertices[mesh.indices[i+0]];
      vec3 p1 = mesh.vertices[mesh.indices[i+1]];
      vec3 p2 = mesh.vertices[mesh.indices[i+2]];

      vec3 u = p2 - p0;
      vec3 v = p1 - p0;

      vec3 fn = glm::normalize(glm::cross(u,v));


      mesh.normals[mesh.indices[i+0]] += fn;
      mesh.normals[mesh.indices[i+1]] += fn;
      mesh.normals[mesh.indices[i+2]] += fn;

      }

      for(size_t i = 0; i < mesh.vertices.size(); ++i) {
      mesh.normals[i] = glm::normalize(mesh.normals[i]);
      }

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
    GL_C(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexVbo));
    GL_C(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)* mesh.indices.size(), mesh.indices.data(), GL_STATIC_DRAW));


    // create

    GL_C(glGenBuffers(1, &mesh.vertexVbo));
    GL_C(glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexVbo));
    GL_C(glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*3*mesh.vertices.size(), mesh.vertices.data() , GL_STATIC_DRAW));

    GL_C(glGenBuffers(1, &mesh.normalVbo));
    GL_C(glBindBuffer(GL_ARRAY_BUFFER, mesh.normalVbo));
    GL_C(glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*3*mesh.normals.size(), mesh.normals.data() , GL_STATIC_DRAW));





    // enable
    GL_C(glEnableVertexAttribArray(0));
    GL_C(glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexVbo));
    GL_C(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0));

    GL_C(glEnableVertexAttribArray(1));
    GL_C(glBindBuffer(GL_ARRAY_BUFFER, mesh.normalVbo));
    GL_C(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0));



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

//	GL_C(glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ));


	int fbWidth, fbHeight;
	glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
	GL_C(glViewport(0, 0, fbWidth, fbHeight));
	GL_C(glClearColor(0.0f, 0.0f, 1.0f, 0.0f));
        GL_C(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

	shader.Bind();


	updateViewMatrix();

	shader.SetUniform("uProject", projectionMatrix );
	shader.SetUniform("uView",viewMatrix );
	shader.SetUniform("uEyePos",cameraPos );



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
