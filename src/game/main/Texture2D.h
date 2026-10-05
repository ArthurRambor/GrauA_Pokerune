#pragma once

#include <glad/glad.h>

namespace deltarune
{

class Texture2D
{
public:
    GLuint ID = 0;
    GLuint Width = 0;
    GLuint Height = 0;
    GLenum InternalFormat = GL_RGBA;
    GLenum ImageFormat = GL_RGBA;
    GLenum WrapS = GL_CLAMP_TO_EDGE;
    GLenum WrapT = GL_CLAMP_TO_EDGE;
    GLenum FilterMin = GL_NEAREST;
    GLenum FilterMag = GL_NEAREST;

    Texture2D();
    ~Texture2D();

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;
    Texture2D(Texture2D&& other) noexcept;
    Texture2D& operator=(Texture2D&& other) noexcept;

    void Generate(GLuint width, GLuint height, const unsigned char* data);
    void Bind() const;
};

} // namespace deltarune
