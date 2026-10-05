#include "Player.h"

#include <GLFW/glfw3.h>

namespace deltarune
{

Player::Player() = default;

void Player::Reset(const Arena& arena)
{
    position_ = CenterOf(arena.Bounds());
}

void Player::ProcessInput(float deltaTime, const std::array<bool, 1024>& keys,
                          const Arena& arena)
{
    glm::vec2 direction(0.0f);
    if (keys[GLFW_KEY_W])
        direction.y -= 1.0f;
    if (keys[GLFW_KEY_S])
        direction.y += 1.0f;
    if (keys[GLFW_KEY_A])
        direction.x -= 1.0f;
    if (keys[GLFW_KEY_D])
        direction.x += 1.0f;

    if (glm::length(direction) > 0.0f)
        direction = glm::normalize(direction);

    position_ += direction * speed_ * deltaTime;
    position_ = arena.ConstrainCenter(position_, size_);
}

void Player::Render(SpriteRenderer& renderer, const Texture2D& texture) const
{
    renderer.DrawSprite(texture, position_ - size_ * 0.5f, size_);
}

Rect Player::Hitbox() const
{
    return RectFromCenter(position_, size_);
}

} // namespace deltarune
