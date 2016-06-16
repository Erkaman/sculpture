#include <stdio.h>


#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.hpp"
#include "gl_common.hpp"

#include "marching_cubes.hpp"

#include "deform.hpp"


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
    "float specularPower = 32.45;"

    "vec3 n = vNormal;"
    "vec3 l = normalize(lightDir);"
    "vec3 v = normalize(uEyePos - vPosition);"
    "vec3 ambient = ambientLight * diffuseColor;"
    "vec3 diffuse = diffuseColor * lightColor * dot(n, l) ;"
    "vec3 specular = vec3(pow(clamp(dot(normalize(l+v),n),0.0,1.0)  , specularPower));"


    "    vec3 diff = ambient + diffuse + specular; \n"
//    "    diff = vec3(dot(n,l)); \n"
    "    diff = vec3(abs(n)); \n"
//    "    diff = vec3( abs(vPosition) ); \n"

    "    color = vec4(diff, 1.0); \n"
    "}\n";


Mesh mesh;

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

GLFWwindow* window;

/*
float cameraTheta = 0.8f;
float cameraPhi = 0.8 * M_PI/2.0f;
float cameraR = 3.0;
*/

float cameraYaw = 0.0f;
float cameraPitch = 0.0f;
float cameraZoom = 10.0;

glm::vec3 cameraPos;

glm::mat4 viewMatrix;

/*
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
*/

void updateViewMatrix() {
    glm::vec3 camera_offset = vec3(cameraZoom, 0.0, 0.0);
// construct an arcball camera matrix


    glm::mat4 camera_transform;

    camera_transform = glm::rotate(camera_transform, cameraYaw, glm::vec3(0.f, 1.f, 0.f)); // add yaw
    camera_transform = glm::rotate(camera_transform, cameraPitch, glm::vec3(0.f, 0.f, 1.f)); // add pitch

    glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::vec3 center(0.0f, 0.0f, 0.0f);

    cameraPos = glm::vec3(camera_transform * glm::vec4(cameraZoom, 0.0, 0.0, 1.0));

    viewMatrix = glm::lookAt(
	cameraPos,
	center,
	up
	);


//viewMatrix = camera_transform;
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
    for(float s = 0; s < 16.0f; s +=1.0f) {

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
	    r = 0.5;


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


void CreateUVSphere() {

    int radius = 1.0;
    const int stacks = 10; // 100
    const int slices = 10; // 100

    // keeps track of the index of the next vertex that we create.

    int index = 0;

    /*
     First of all, we create all the faces that are NOT adjacent to the
     bottom(0,-R,0) and top(0,+R,0) vertices of the sphere.
     (it's easier this way, because for the bottom and top vertices, we need to add triangle faces.
     But for the faces between, we need to add quad faces. )
     */

    // loop through the stacks.
    for (int i = 1; i < stacks; ++i){

        float u  = (float)i / stacks;
        float phi = u * M_PI;

        GLuint stackBaseIndex = mesh.faces.size()/2;
        // loop through the slices.
        for (int j = 0; j < slices; ++j){

            float v = (float)j / slices;
            float theta = v * (M_PI * 2);


            float R = radius;
            // use spherical coordinates to calculate the positions.
            float x = cos (theta) * sin (phi);
            float y = cos (phi);
            float z =sin (theta) * sin (phi);

	    mesh.vertices.emplace_back(R*x,R*y,R*z);

            if((i +1) != stacks ) { // for the last stack, we don't need to add faces.

                GLuint i1, i2, i3, i4;

                if((j+1)==slices) {
                    // for the last vertex in the slice, we need to wrap around to create the face.
                    i1 = index;
                    i2 = stackBaseIndex;
                    i3 = index  + slices;
                    i4 = stackBaseIndex  + slices;

                } else {
                    // use the indices from the current slice, and indices from the next slice, to create the face.
                    i1 = index;
                    i2 = index + 1;
                    i3 = index  + slices;
                    i4 = index  + slices + 1;

                }

                // add quad face


		mesh.faces.emplace_back(i1, i2, i3);
		mesh.faces.emplace_back(i4, i3, i2);


            }

            index++;
        }
    }

    /*
     Next, we finish the sphere by adding the faces that are adjacent to the top and bottom vertices.
     */



    GLuint topIndex = index++;
    mesh.vertices.emplace_back(0.0,radius,0.0);
//    positions.push([0.0,radius,0.0]);
//    normals.push([0,1,0]);


    GLuint bottomIndex = index++;
//    positions.push([0, -radius, 0 ]);
    mesh.vertices.emplace_back(0.0,-radius,0.0);

//    normals.push([0,-1,0]);


    for (int i = 0; i < slices; ++i) {

        GLuint i1 = topIndex;
        GLuint i2 = (i+0);
        GLuint i3 = (i+1) % slices;

	mesh.faces.emplace_back(i3, i2, i1);



        i1 = bottomIndex;
        i2 = (bottomIndex-1) - slices +  (i+0);
        i3 = (bottomIndex-1) - slices + ((i+1)%slices);

	mesh.faces.emplace_back(i1, i2, i3);

    }


}
/*
void AddCubeFace(Mesh& mesh, int i) {

    const GLuint base = (GLuint)(mesh.vertices.size());

    GLuint start = mesh.vertices.size();

    int N = 20; // degree of tesselation. means  quads.

    float xmin = -0.5;
    float xmax = +0.5;
    float ymin = -0.5;
    float ymax = +0.5;

    for(int row = 0; row <= N; ++row) {

	float y = (row / (float)N)*(ymax-ymin) + ymin;

	for(int col = 0; col <= N; ++col) {

	    float x= (col / (float)N)*(xmax-xmin) + xmin;

	    mesh.vertices.emplace_back(x,y,0.5f);

//	    printf("add: %f, %f, %f\n",  x,y,0.5f );


	}

    }

    int end = mesh.vertices.size();

    for(int j = start; j < end; j+=1) {

	vec3 p = mesh.vertices[j];

	if(i == 0) { // front

	} else if(i==1) { // back
	    p.z *= -1.0;
	    p.x *= -1.0;
	} else if(i==2) { // top

	    float x2 = -p.y;
	    float y2 = +p.z;
	    float z2 = -p.x;

	    p.x = x2;
	    p.y = y2;
	    p.z = z2;
	} else if(i==3) { // bottom.

	    float x2 = +p.y;
	    float y2 = -p.z;
	    float z2 = -p.x;

	    p.x = x2;
	    p.y = y2;
	    p.z = z2;
	} else if(i==4) { // right

	    float x2 = +p.z;
	    float y2 = +p.y;
	    float z2 = -p.x;

	    p.x = x2;
	    p.y = y2;
	    p.z = z2;
	} else if(i==5) { // left

	    float x2 = -p.z;
	    float y2 = +p.y;
	    float z2 = +p.x;

	    p.x = x2;
	    p.y = y2;
	    p.z = z2;
	}

	mesh.vertices[j] = p;
	    //glm::normalize(p);


    }

    for(int row = 0; row <= (N-1); ++row) {

	for(int col = 0; col <= (N-1); ++col) {

	    int i = row * (N+1) + col;

	    int i0 = i+0;
	    int i1 = i+1;
	    int i2 = i + (N+1) + 0;
	    int i3 = i + (N+1) + 1;


	    mesh.faces.push_back(base + i0);
	    mesh.faces.push_back(base + i1);
	    mesh.faces.push_back(base + i2);

	    	    mesh.faces.push_back(base + i3);

	    mesh.faces.push_back(base + i2);

	    	    mesh.faces.push_back(base + i1);

	}
    }

}
*/


void InitSphere(void) {

/*
    AddCubeFace(mesh, 0);
    AddCubeFace(mesh, 1);
    AddCubeFace(mesh, 2);
    AddCubeFace(mesh, 3);
    AddCubeFace(mesh, 4);
    AddCubeFace(mesh, 5);
*/

    CreateUVSphere();

    printf("vertices: %ld\n", mesh.vertices.size() );
    printf("faces: %ld\n", mesh.faces.size() );

//    ComputeNormals(mesh);

    Sweep(mesh);



}


void InitMC(void)
{

    Density d;

    mesh = MarchingCubes(d,
			100,
			 -10, +10,
			 -10,  +10,
			 -10, +10
	);


    //  ComputeNormals();

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
    GL_C(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)* mesh.faces.size()*3, mesh.faces.data(), GL_STATIC_DRAW));


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
//    cameraR += yoffset;

    cameraZoom += yoffset;


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
//    InitMC();
  InitSphere();
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
	GL_C(glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ));


	int fbWidth, fbHeight;
	glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
	GL_C(glViewport(0, 0, fbWidth, fbHeight));
	GL_C(glClearColor(0.0f, 0.0f, 0.3f, 0.0f));
        GL_C(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

	shader.Bind();


	updateViewMatrix();

	shader.SetUniform("uProject", projectionMatrix );
	shader.SetUniform("uView",viewMatrix );
//	shader.SetUniform("uEyePos",cameraPos );



	GL_C(glDrawElements(GL_TRIANGLES, mesh.faces.size()*3 , GL_UNSIGNED_INT, 0));


	prevMouseX = curMouseX;
	prevMouseY = curMouseY;
	glfwGetCursorPos(window, &curMouseX, &curMouseY);

	const float MOUSE_SENSITIVITY = 0.005;

	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	if (state == GLFW_PRESS) {

	    cameraYaw += (curMouseX - prevMouseX ) * MOUSE_SENSITIVITY;
	    cameraPitch += (curMouseY - prevMouseY ) * MOUSE_SENSITIVITY;
	}

//	printf("delta x: %f\n",  curMouseX - prevMouseX );


        /* display and process events through callbacks */
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
