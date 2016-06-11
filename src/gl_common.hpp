#pragma once

#include <vector>

typedef unsigned int uint;

#define _DEBUG

void CheckOpenGLError(const char* stmt, const char* fname, int line)
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

struct Mesh {

    std::vector<glm::vec3> vertices;
    std::vector<uint> indices;

    GLuint indexVbo;
    GLuint vertexVbo;


};
