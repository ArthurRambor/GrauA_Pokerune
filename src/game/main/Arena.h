#pragma once

#include "GameTypes.h"

namespace deltarune
{

class Arena
{
public:
    Arena(const glm::vec2& position, const glm::vec2& size, float wallThickness);

    const Rect& Bounds() const { return bounds_; }
    float WallThickness() const { return wallThickness_; }

    // Keeps a centered object inside the playable area without making
    // attacks collide with the arena walls.
    glm::vec2 ConstrainCenter(const glm::vec2& center, const glm::vec2& objectSize) const;

private:
    Rect bounds_;
    float wallThickness_;
};

} // namespace deltarune
