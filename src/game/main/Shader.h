#pragma once

#include <string>

#include <glad/glad.h>
#include <glm/glm.hpp>

namespace deltarune
{

class Shader
{
public:
    Shader() = default;
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    Shader& Compile(const char* vertexSource, const char* fragmentSource);
    Shader& Use();

    void SetBool(const std::string& name, bool value) const;
    void SetInteger(const std::string& name, int value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetVector2f(const std::string& name, const glm::vec2& value) const;
    void SetVector3f(const std::string& name, const glm::vec3& value) const;
    void SetVector4f(const std::string& name, const glm::vec4& value) const;
    void SetMatrix4(const std::string& name, const glm::mat4& value) const;

private:
    GLuint id_ = 0;

    static GLuint CompileStage(GLenum type, const char* source, const char* label);
    static void CheckShaderErrors(GLuint object, GLenum status, const char* label);
};

} // namespace deltarune
