#include "Arena.h"

namespace deltarune
{

Arena::Arena(const glm::vec2& position, const glm::vec2& size, float wallThickness)
    : bounds_{position, size}, wallThickness_(wallThickness)
{
}

glm::vec2 Arena::ConstrainCenter(const glm::vec2& center, const glm::vec2& objectSize) const
{
    const glm::vec2 halfSize = objectSize * 0.5f;
    const glm::vec2 minimum = bounds_.position + glm::vec2(wallThickness_) + halfSize;
    const glm::vec2 maximum = bounds_.position + bounds_.size - glm::vec2(wallThickness_) - halfSize;

    return glm::clamp(center, minimum, maximum);
}

} // namespace deltarune
