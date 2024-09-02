#pragma once
#include <glad/glad.h>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <glm/glm.hpp>
#define LOG_LEN 1024

enum CompileType{
    VERTEX,
    FRAGMENT,
    PROGRAM,
};

class Shader {
public:
    unsigned int ID;
    Shader() {};
    Shader(const char* vertexPath, const char* fragmentPath){
        vertex = compileShader(getFileContent(vertexPath).c_str(),VERTEX);
        fragment = compileShader(getFileContent(fragmentPath).c_str(),FRAGMENT);
        linkPrograme(vertex, fragment);
    }

    void use() const {
        glUseProgram(ID);
    }

    void setBool(const std::string& name, bool value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }

    void setInt(const std::string &name, int value) const
    { 
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value); 
    }

    void setFloat(const std::string &name, float value) const
    { 
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value); 
    }

    void setVec2(const std::string &name, const glm::vec2 &value) const
    { 
        glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]); 
    }

    void setVec2(const std::string &name, float x, float y) const
    { 
        glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y); 
    }
    
    void setVec3(const std::string &name, const glm::vec3 &value) const
    { 
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]); 
    }
    void setVec3(const std::string &name, float x, float y, float z) const
    { 
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z); 
    }

    void setVec4(const std::string &name, const glm::vec4 &value) const
    { 
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]); 
    }

    void setVec4(const std::string &name, float x, float y, float z, float w) const
    { 
        glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w); 
    }

    void setMat2(const std::string &name, const glm::mat2 &mat) const
    {
        glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    void setMat3(const std::string &name, const glm::mat3 &mat) const
    {
        glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    void setMat4(const std::string &name, const glm::mat4 &mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
private:
    unsigned int vertex, fragment;

    std::string getFileContent(const char* path) {
        std::ifstream shaderFile;
        shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        shaderFile.open(path);
        std::stringstream shaderStream;
        shaderStream << shaderFile.rdbuf();
        shaderFile.close();
        return shaderStream.str();
    }

    GLuint compileShader(const char* shaderCode,CompileType type) {
        unsigned int shader;
        shader = glCreateShader(type == VERTEX?GL_VERTEX_SHADER:GL_FRAGMENT_SHADER);
        glShaderSource(shader, 1, &shaderCode, NULL);
        glCompileShader(shader);
        checkErrors(shader,type);
        return shader;
    }

    void linkPrograme(GLuint vertex,GLuint fragment){
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkErrors(ID, PROGRAM);
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    void checkErrors(GLuint id,CompileType type) {
        GLint success = 0;
        GLchar infoLog[LOG_LEN];
        if (type == PROGRAM) {
            glGetProgramiv(id, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(id, LOG_LEN, NULL, infoLog);
                printf("Shader Program Link Error: %s\n", infoLog);
            }
        }
        else {
            glGetShaderiv(id, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(id, LOG_LEN, NULL, infoLog);
                printf("Shader Compile Error: %s\n", infoLog);
            }
        }
    }
};
