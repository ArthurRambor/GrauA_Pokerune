#pragma once

#include <algorithm>

#include <glm/glm.hpp>

namespace deltarune
{

constexpr unsigned int LOGICAL_WIDTH = 800;
constexpr unsigned int LOGICAL_HEIGHT = 600;

// Positions use a top-left origin in logical pixels. The renderer applies
// one orthographic projection, so gameplay coordinates never depend on the
// physical size or aspect ratio of the window.
struct Rect
{
    glm::vec2 position{0.0f};
    glm::vec2 size{0.0f};
};

inline bool Intersects(const Rect& first, const Rect& second)
{
    return first.position.x < second.position.x + second.size.x &&
           first.position.x + first.size.x > second.position.x &&
           first.position.y < second.position.y + second.size.y &&
           first.position.y + first.size.y > second.position.y;
}

inline glm::vec2 CenterOf(const Rect& rectangle)
{
    return rectangle.position + rectangle.size * 0.5f;
}

inline Rect RectFromCenter(const glm::vec2& center, const glm::vec2& size)
{
    return {center - size * 0.5f, size};
}

enum class GameState
{
    Home,
    Playing,
    GameOver
};

} // namespace deltarune
