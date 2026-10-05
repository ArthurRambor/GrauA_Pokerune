#include "Shader.h"

#include <iostream>

#include <glm/gtc/type_ptr.hpp>

namespace deltarune
{

Shader::~Shader()
{
    if (id_ != 0)
        glDeleteProgram(id_);
}

Shader::Shader(Shader&& other) noexcept : id_(other.id_)
{
    other.id_ = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if (this == &other)
        return *this;

    if (id_ != 0)
        glDeleteProgram(id_);

    id_ = other.id_;
    other.id_ = 0;
    return *this;
}

Shader& Shader::Compile(const char* vertexSource, const char* fragmentSource)
{
    const GLuint vertexShader = CompileStage(GL_VERTEX_SHADER, vertexSource, "vertex");
    const GLuint fragmentShader = CompileStage(GL_FRAGMENT_SHADER, fragmentSource, "fragment");

    const GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    CheckShaderErrors(program, GL_LINK_STATUS, "program linking");
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    if (id_ != 0)
        glDeleteProgram(id_);
    id_ = program;
    return *this;
}

Shader& Shader::Use()
{
    glUseProgram(id_);
    return *this;
}

void Shader::SetBool(const std::string& name, bool value) const
{
    glUniform1i(glGetUniformLocation(id_, name.c_str()), value ? GL_TRUE : GL_FALSE);
}

void Shader::SetInteger(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(id_, name.c_str()), value);
}

void Shader::SetFloat(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(id_, name.c_str()), value);
}

void Shader::SetVector2f(const std::string& name, const glm::vec2& value) const
{
    glUniform2f(glGetUniformLocation(id_, name.c_str()), value.x, value.y);
}

void Shader::SetVector3f(const std::string& name, const glm::vec3& value) const
{
    glUniform3f(glGetUniformLocation(id_, name.c_str()), value.x, value.y, value.z);
}

void Shader::SetVector4f(const std::string& name, const glm::vec4& value) const
{
    glUniform4f(glGetUniformLocation(id_, name.c_str()), value.x, value.y, value.z, value.w);
}

void Shader::SetMatrix4(const std::string& name, const glm::mat4& value) const
{
    glUniformMatrix4fv(
        glGetUniformLocation(id_, name.c_str()),
        1,
        GL_FALSE,
        glm::value_ptr(value)
    );
}

GLuint Shader::CompileStage(GLenum type, const char* source, const char* label)
{
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    CheckShaderErrors(shader, GL_COMPILE_STATUS, label);
    return shader;
}

void Shader::CheckShaderErrors(GLuint object, GLenum status, const char* label)
{
    GLint success = GL_FALSE;
    if (status == GL_LINK_STATUS)
        glGetProgramiv(object, status, &success);
    else
        glGetShaderiv(object, status, &success);

    if (success == GL_TRUE)
        return;

    GLchar infoLog[1024]{};
    if (status == GL_LINK_STATUS)
        glGetProgramInfoLog(object, sizeof(infoLog), nullptr, infoLog);
    else
        glGetShaderInfoLog(object, sizeof(infoLog), nullptr, infoLog);

    std::cerr << "OpenGL " << label << " failed:\n" << infoLog << std::endl;
}

} // namespace deltarune
