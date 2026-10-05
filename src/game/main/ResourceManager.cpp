#include "ResourceManager.h"

#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace deltarune
{

std::unordered_map<std::string, Shader> ResourceManager::shaders_;
std::unordered_map<std::string, Texture2D> ResourceManager::textures_;

bool ResourceManager::LoadShader(const char* vertexSource, const char* fragmentSource,
                                 const std::string& name)
{
    Shader shader;
    shader.Compile(vertexSource, fragmentSource);
    shaders_[name] = std::move(shader);
    return true;
}

Shader& ResourceManager::GetShader(const std::string& name)
{
    return shaders_.at(name);
}

bool ResourceManager::LoadTexture(const std::string& file, const std::string& name)
{
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* data = stbi_load(file.c_str(), &width, &height, &channels, 0);

    if (data == nullptr)
    {
        std::cerr << "Could not load texture: " << file << std::endl;
        return false;
    }

    Texture2D texture;
    texture.InternalFormat = channels == 4 ? GL_RGBA : GL_RGB;
    texture.ImageFormat = channels == 4 ? GL_RGBA : GL_RGB;
    texture.Generate(static_cast<GLuint>(width), static_cast<GLuint>(height), data);
    stbi_image_free(data);
    textures_[name] = std::move(texture);
    return true;
}

Texture2D& ResourceManager::GetTexture(const std::string& name)
{
    return textures_.at(name);
}

void ResourceManager::Clear()
{
    // OpenGL objects must be destroyed while the context still exists.
    textures_.clear();
    shaders_.clear();
}

} // namespace deltarune
