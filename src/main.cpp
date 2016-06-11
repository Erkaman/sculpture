#include <stdio.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.hpp"
#include "gl_common.hpp"


#ifndef M_PI
#define M_PI 3.14159
#endif

/**********************************************************************
 * Default shader programs
 *********************************************************************/

static const char* vertex_shader_text =
    "#version 330\n"
    "uniform mat4 project;\n"
    "uniform mat4 view;\n"
    "layout(location = 0) in vec3 pos;"
    "\n"
    "void main()\n"
    "{\n"
    "float s = 10.0;"
    "   gl_Position = project * view * vec4(pos.x*s, pos.y*s, pos.z*s, 1.0);\n"
    "}\n";

static const char* fragment_shader_text =
    "#version 330\n"
    "out vec4 color;\n"
    "void main()\n"
    "{\n"
    "    color = vec4(0.2, 1.0, 0.2, 1.0); \n"
    "}\n";

GLfloat my_map_vertices[3 * 2];
GLuint  map_line_indices[3];

GLuint mesh_vbo;
GLuint vertexBuffer;

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

GLFWwindow* window;

float cameraTheta = 0.8f;
float cameraPhi = 0.8 * M_PI/2.0f;
float cameraR = 30.0;


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

static void init_map(void)
{
    int k;

    GLfloat x = 1.0f;
    GLfloat z = 0.0f;


    k = 0;
    my_map_vertices[k++] = 1.0;
    my_map_vertices[k++] = 0;
    my_map_vertices[k++] = 0.0;

    my_map_vertices[k++] = 1.0;
    my_map_vertices[k++] = 0;
    my_map_vertices[k++] = 1.0;

    my_map_vertices[k++] = 0;
    my_map_vertices[k++] = 0;
    my_map_vertices[k++] = 0;


    k = 0;
    map_line_indices[k++] = 2;
    map_line_indices[k++] = 1;
    map_line_indices[k++] = 0;
}

/**********************************************************************
 * OpenGL helper functions
 *********************************************************************/

/* Create VBO, IBO and VAO objects for the heightmap geometry and bind them to
 * the specified program object
 */
static void make_mesh()
{
    GLuint attrloc;


    GL_C(glGenBuffers(1, &mesh_vbo));
    /* Prepare the data for drawing through a buffer inidices */
    GL_C(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_vbo));
    GL_C(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)* 3, map_line_indices, GL_STATIC_DRAW));


    GL_C(glGenBuffers(1, &vertexBuffer));
    GL_C(glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer));
    GL_C(glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*3*3, my_map_vertices , GL_STATIC_DRAW));


    GL_C(glEnableVertexAttribArray(0));
    GL_C(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0));



}
/**********************************************************************
 * GLFW callback functions
 *********************************************************************/

void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset){
    cameraR += yoffset;
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


    glfwMakeContextCurrent(window);

    // load GLAD.
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // Bind and create VAO, otherwise, we can't do anything in OpenGL.
    static GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

}

int main(int argc, char** argv)
{
    InitGlfw();

    Shader shader(vertex_shader_text, fragment_shader_text);

    // projection matrix.
    glm::mat4 projectionMatrix = glm::perspective(0.9f, (float)WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 100.0f);



    /* Create mesh data */
    init_map();
    make_mesh();

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    GL_C(glViewport(0, 0, fbWidth, fbHeight));
    GL_C(glEnable(GL_CULL_FACE));

    GL_C(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));


    double prevMouseX = 0;
    double prevMouseY = 0;

    double curMouseX = 0;
    double curMouseY = 0;


    while (!glfwWindowShouldClose(window)) {

        glClear(GL_COLOR_BUFFER_BIT);


	shader.Bind();


	updateViewMatrix();

	shader.SetUniform("project", projectionMatrix );
	shader.SetUniform("view",viewMatrix );



	GL_C(glDrawElements(GL_TRIANGLES, 3 , GL_UNSIGNED_INT, 0));



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
