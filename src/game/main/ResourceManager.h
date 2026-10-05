#pragma once

#include <string>
#include <unordered_map>

#include "Shader.h"
#include "Texture2D.h"

namespace deltarune
{

class ResourceManager
{
public:
    // Resources are owned centrally so scenes can request shared assets by name.
    static bool LoadShader(const char* vertexSource, const char* fragmentSource,
                           const std::string& name);
    static Shader& GetShader(const std::string& name);

    static bool LoadTexture(const std::string& file, const std::string& name);
    static Texture2D& GetTexture(const std::string& name);

    static void Clear();

private:
    static std::unordered_map<std::string, Shader> shaders_;
    static std::unordered_map<std::string, Texture2D> textures_;
};

} // namespace deltarune
