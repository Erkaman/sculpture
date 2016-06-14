#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <vector>

#include "glm/gtx/string_cast.hpp"

typedef unsigned int uint;

#define _DEBUG

inline void CheckOpenGLError(const char* stmt, const char* fname, int line)
{
    GLenum err = glGetError();
    //  const GLubyte* sError = gluErrorString(err);

    if (err != GL_NO_ERROR)
    {
        printf("OpenGL error %08x, at %s:%i - for %s. Error Message\n", err, fname, line, stmt);
	exit(1);
    }
}


// GL Check.
#ifdef _DEBUG
    #define GL_C(stmt) do { \
            stmt; \
            CheckOpenGLError(#stmt, __FILE__, __LINE__); \
        } while (0)
#else
    #define GL_C(stmt) stmt
#endif

struct Tri {

public:
    GLuint i[3];

    Tri(GLuint i0, GLuint i1, GLuint i2):
	i{i0,i1,i2}{

    }
};

struct Mesh {

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<Tri> indices;


    GLuint indexVbo;
    GLuint vertexVbo;
    GLuint normalVbo;


};
