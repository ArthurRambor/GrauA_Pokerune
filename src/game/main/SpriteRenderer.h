#pragma once

#include <glm/glm.hpp>

#include "Shader.h"
#include "Texture2D.h"

namespace deltarune
{

class SpriteRenderer
{
public:
    explicit SpriteRenderer(Shader& shader);
    ~SpriteRenderer();

    SpriteRenderer(const SpriteRenderer&) = delete;
    SpriteRenderer& operator=(const SpriteRenderer&) = delete;

    void SetProjection(const glm::mat4& projection);
    void DrawSprite(const Texture2D& texture, const glm::vec2& position,
                    const glm::vec2& size, const glm::vec4& color = glm::vec4(1.0f),
                    float rotate = 0.0f,
                    const glm::vec4& uvRect = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f),
                    bool useChromaKey = false,
                    const glm::vec3& chromaKeyColor = glm::vec3(0.0f)) const;
    void DrawQuad(const glm::vec2& position, const glm::vec2& size,
                  const glm::vec4& color) const;

private:
    Shader& shader_;
    GLuint quadVAO_ = 0;
    GLuint quadVBO_ = 0;
    glm::mat4 projection_{1.0f};

    void InitRenderData();
    void DrawGeometry() const;
};

} // namespace deltarune
