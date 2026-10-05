#include "SpriteRenderer.h"

#include <glm/gtc/matrix_transform.hpp>

namespace deltarune
{

SpriteRenderer::SpriteRenderer(Shader& shader) : shader_(shader)
{
    shader_.Use().SetInteger("image", 0);
    InitRenderData();
}

SpriteRenderer::~SpriteRenderer()
{
    if (quadVBO_ != 0)
        glDeleteBuffers(1, &quadVBO_);
    if (quadVAO_ != 0)
        glDeleteVertexArrays(1, &quadVAO_);
}

void SpriteRenderer::SetProjection(const glm::mat4& projection)
{
    projection_ = projection;
}

void SpriteRenderer::DrawSprite(const Texture2D& texture, const glm::vec2& position,
                                const glm::vec2& size, const glm::vec4& color,
                                float rotate, const glm::vec4& uvRect,
                                bool useChromaKey,
                                const glm::vec3& chromaKeyColor) const
{
    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(position + size * 0.5f, 0.0f));
    model = glm::rotate(model, glm::radians(rotate), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::translate(model, glm::vec3(-size * 0.5f, 0.0f));
    model = glm::scale(model, glm::vec3(size, 1.0f));

    shader_.Use();
    shader_.SetMatrix4("projection", projection_);
    shader_.SetMatrix4("model", model);
    shader_.SetVector4f("spriteColor", color);
    shader_.SetVector4f("uvRect", uvRect);
    shader_.SetBool("useTexture", true);
    shader_.SetBool("useChromaKey", useChromaKey);
    shader_.SetVector3f("chromaKeyColor", chromaKeyColor);
    texture.Bind();
    DrawGeometry();
}

void SpriteRenderer::DrawQuad(const glm::vec2& position, const glm::vec2& size,
                              const glm::vec4& color) const
{
    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(position + size * 0.5f, 0.0f));
    model = glm::scale(model, glm::vec3(size, 1.0f));

    shader_.Use();
    shader_.SetMatrix4("projection", projection_);
    shader_.SetMatrix4("model", model);
    shader_.SetVector4f("spriteColor", color);
    shader_.SetVector4f("uvRect", glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    shader_.SetBool("useTexture", false);
    shader_.SetBool("useChromaKey", false);
    DrawGeometry();
}

void SpriteRenderer::InitRenderData()
{
    const GLfloat vertices[] =
    {
        // position        // texture coordinates
        0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
        1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
        0.0f, 0.0f, 0.0f,  0.0f, 0.0f,

        0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
        1.0f, 1.0f, 0.0f,  1.0f, 1.0f,
        1.0f, 0.0f, 0.0f,  1.0f, 0.0f
    };

    glGenVertexArrays(1, &quadVAO_);
    glGenBuffers(1, &quadVBO_);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindVertexArray(quadVAO_);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                          reinterpret_cast<void*>(3 * sizeof(GLfloat)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void SpriteRenderer::DrawGeometry() const
{
    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

} // namespace deltarune
