#include "Texture2D.h"

namespace deltarune
{

Texture2D::Texture2D()
{
    glGenTextures(1, &ID);
}

Texture2D::~Texture2D()
{
    if (ID != 0)
        glDeleteTextures(1, &ID);
}

Texture2D::Texture2D(Texture2D&& other) noexcept
    : ID(other.ID), Width(other.Width), Height(other.Height),
      InternalFormat(other.InternalFormat), ImageFormat(other.ImageFormat),
      WrapS(other.WrapS), WrapT(other.WrapT),
      FilterMin(other.FilterMin), FilterMag(other.FilterMag)
{
    other.ID = 0;
}

Texture2D& Texture2D::operator=(Texture2D&& other) noexcept
{
    if (this == &other)
        return *this;

    if (ID != 0)
        glDeleteTextures(1, &ID);

    ID = other.ID;
    Width = other.Width;
    Height = other.Height;
    InternalFormat = other.InternalFormat;
    ImageFormat = other.ImageFormat;
    WrapS = other.WrapS;
    WrapT = other.WrapT;
    FilterMin = other.FilterMin;
    FilterMag = other.FilterMag;
    other.ID = 0;
    return *this;
}

void Texture2D::Generate(GLuint width, GLuint height, const unsigned char* data)
{
    Width = width;
    Height = height;

    glBindTexture(GL_TEXTURE_2D, ID);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        InternalFormat,
        static_cast<GLsizei>(width),
        static_cast<GLsizei>(height),
        0,
        ImageFormat,
        GL_UNSIGNED_BYTE,
        data
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, WrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, WrapT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, FilterMin);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, FilterMag);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture2D::Bind() const
{
    glBindTexture(GL_TEXTURE_2D, ID);
}

} // namespace deltarune
