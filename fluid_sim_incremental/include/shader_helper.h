#ifndef SHADER_HELPER_H
#define SHADER_HELPER_H

#include <glad/glad.h>
#include <string>

class ShaderHelper {
public:
    static GLuint getCurrentProgram() {
        GLint program;
        glGetIntegerv(GL_CURRENT_PROGRAM, &program); // gets the program id and writes it to &program.
        return static_cast<GLuint>(program);  //program ID is returned.
    }
    
    static void setUniform1i(const char* name, int value) {
        GLuint program = getCurrentProgram();  //gets programID
        GLint loc = glGetUniformLocation(program, name);  //if uniform is present, then returns non-negative handle
        if (loc >= 0) glUniform1i(loc, value);  //sets the value of that unifrom using the handle to value that is passed here.
    }
    
    static void setUniform1f(const char* name, float value) {
        GLuint program = getCurrentProgram();
        GLint loc = glGetUniformLocation(program, name);
        if (loc >= 0) glUniform1f(loc, value);  // this one targets floating point unifroms and the one above targets integer unifroms.
    }
    
    static void setUniform2f(const char* name, float v1, float v2) {
        GLuint program = getCurrentProgram();
        GLint loc = glGetUniformLocation(program, name);
        if (loc >= 0) glUniform2f(loc, v1, v2);  // this one is the same as the floating point one but for a vec2 instead of a single value.
    }
    
    static void setUniform1fv(const char* name, int count, const float* values) {
        GLuint program = getCurrentProgram();
        GLint loc = glGetUniformLocation(program, name);
        if (loc >= 0) glUniform1fv(loc, count, values);  // this one allows for multiple values of the same unifrom to be set at the same time.
    }
};

#endif // SHADER_HELPER_H
