#ifndef SHADER_HELPER_H
#define SHADER_HELPER_H

#include <glad/glad.h>
#include <string>

class ShaderHelper {
public:
    static GLuint getCurrentProgram() {
        GLint program;
        glGetIntegerv(GL_CURRENT_PROGRAM, &program);
        return static_cast<GLuint>(program);
    }
    
    static void setUniform1i(const char* name, int value) {
        GLuint program = getCurrentProgram();
        GLint loc = glGetUniformLocation(program, name);
        if (loc >= 0) glUniform1i(loc, value);
    }
    
    static void setUniform1f(const char* name, float value) {
        GLuint program = getCurrentProgram();
        GLint loc = glGetUniformLocation(program, name);
        if (loc >= 0) glUniform1f(loc, value);
    }
    
    static void setUniform2f(const char* name, float v1, float v2) {
        GLuint program = getCurrentProgram();
        GLint loc = glGetUniformLocation(program, name);
        if (loc >= 0) glUniform2f(loc, v1, v2);
    }
    
    static void setUniform1fv(const char* name, int count, const float* values) {
        GLuint program = getCurrentProgram();
        GLint loc = glGetUniformLocation(program, name);
        if (loc >= 0) glUniform1fv(loc, count, values);
    }
};

#endif // SHADER_HELPER_H
