#pragma once

#include <array>

#include "Arena.h"
#include "SpriteRenderer.h"

namespace deltarune
{

class Player
{
public:
    Player();

    void Reset(const Arena& arena);
    void ProcessInput(float deltaTime, const std::array<bool, 1024>& keys,
                      const Arena& arena);
    void Render(SpriteRenderer& renderer, const Texture2D& texture) const;

    Rect Hitbox() const;
    const glm::vec2& Position() const { return position_; }
    const glm::vec2& Size() const { return size_; }

private:
    glm::vec2 position_{0.0f};
    glm::vec2 size_{34.0f};
    float speed_ = 270.0f;
};

} // namespace deltarune
