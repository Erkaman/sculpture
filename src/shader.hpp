#pragma once

#include "gl_common.hpp"

#include <map>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader {

private:

    GLint m_shader;
    std::map<std::string, GLuint>  m_uniformLocations;

    char* GetShaderInfoLog(GLuint shader) {
	GLint len;
	char* infoLog = new char[len];
	GLsizei actualLen;

	GL_C(glGetShaderiv(shader,  GL_INFO_LOG_LENGTH, &len));
	GL_C(glGetShaderInfoLog(shader, len, &actualLen, infoLog));

	return infoLog;
    }

    bool GetCompileStatus(GLuint shader)  {
	GLint status;
	GL_C(glGetShaderiv(shader,  GL_COMPILE_STATUS, &status));
	return status == GL_TRUE;
    }

    GLuint CreateShaderFromString(const char* shaderSource, const GLenum shaderType) {

	GLuint shader;

	GL_C(shader = glCreateShader(shaderType));

	GL_C(glShaderSource(shader, 1, (const char**)&shaderSource, NULL));
	GL_C(glCompileShader(shader));

	if (!GetCompileStatus(shader)) {
	    printf("Could not compile shader\n\n%s \n\n%s",  shaderSource, GetShaderInfoLog(shader) );
	    exit(1);
	}

	return shader;
    }

    void Compile(const char* vertexShaderSource, const char* fragmentShaderSource) {


	// compile
	GLint vertexShader = CreateShaderFromString(vertexShaderSource, GL_VERTEX_SHADER);
	GLint fragmentShader = CreateShaderFromString(fragmentShaderSource, GL_FRAGMENT_SHADER);


	// attach
	GL_C(m_shader = glCreateProgram());

	// attach.
	GL_C(glAttachShader(m_shader, fragmentShader));
	GL_C(glAttachShader(m_shader, vertexShader));

	// clean up.
	GL_C(glDeleteShader( fragmentShader));
	GL_C(glDeleteShader(vertexShader));


	GL_C(glLinkProgram(m_shader));

	// check if linking succeeded
	GLint linkOk;
	glGetProgramiv(m_shader, GL_LINK_STATUS, &linkOk);
	if (linkOk == GL_FALSE) {
	    printf("Error linking program\n\n%s\n\n%s \n\n%s",  vertexShaderSource, fragmentShaderSource,  GetShaderInfoLog(m_shader) );
	    exit(1);
	}
    }

    void LoadUniforms() {

	// first number of uniforms in the shader:
	GLint numActiveUniforms;
	GL_C(glGetProgramiv(m_shader, GL_ACTIVE_UNIFORMS, &numActiveUniforms));

	// length of the uniform name string.
	GLsizei nameLength;

	// the uniform name string is written to this buffer.
	char nameBuffer[256];

	// the size of the uniform.
	GLsizei uniformSize;

	// the type of the uniform.
	GLenum type;

	for(int i = 0; i < numActiveUniforms; ++i) {

	    GL_C(glGetActiveUniform(m_shader, i, 256, &nameLength,
				    &uniformSize,
				    &type,
				    nameBuffer));


	    int uniformLocation;
	    GL_C(uniformLocation = glGetUniformLocation(m_shader, nameBuffer));

	    // If the uniform is located in a uniform block, then uniformLocation will be -1,
	    // and we ignore it.
	    if(uniformLocation != -1) {
		// use uniform name string as key, and uniform location as value.
		m_uniformLocations[std::string(nameBuffer)] = uniformLocation;
	    }
	}
    }

public:

    Shader(const char* vertexShaderSource, const char* fragmentShaderSource) {

	// compile
	Compile(vertexShaderSource, fragmentShaderSource);

	// here we load and save and save all uniforms locations.
	LoadUniforms();
    }

    void Bind() {
	GL_C(glUseProgram(m_shader));
    }

    void Unbind() {
	GL_C(glUseProgram(0));
    }

    void SetUniform(const std::string& uniformName,  const glm::mat4& matrix) {
	if (m_uniformLocations.count(uniformName) > 0) {
	    const GLuint location = m_uniformLocations[uniformName];
	    GL_C(glUniformMatrix4fv(location, 1, false, glm::value_ptr(matrix)));
	}
    }

    void SetUniform(const std::string& uniformName,  const glm::vec3& vec) {
	if (m_uniformLocations.count(uniformName) > 0) {
	    const GLuint location = m_uniformLocations[uniformName];
	    GL_C(glUniform3fv(location, 1, glm::value_ptr(vec)));
	}
    }

};
