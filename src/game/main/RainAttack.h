#pragma once

#include <random>
#include <vector>

#include "Attack.h"

namespace deltarune
{

struct RainSettings
{
    // These values belong to an attack instance, making level-specific
    // patterns possible without changing the renderer or the game loop.
    float initialDelay = 3.0f;
    float activeDuration = 5.0f;
    float cooldown = 3.0f;
    int dropCount = 20;
    float dropSpeed = 500.0f;
    float spawnHeight = 55.0f;
    float targetMargin = 20.0f;
    float minimumTargetDistance = 40.0f;
    glm::vec2 visualSize{43.0f};
    glm::vec2 hitboxSize{30.0f, 38.0f};
    float impactFrameDuration = 0.06f;
    int impactFrameCount = 8;
    float damage = 0.10f;
};

class RainAttack final : public Attack
{
public:
    explicit RainAttack(RainSettings settings = {});

    void Start(const Arena& arena) override;
    void Update(float deltaTime, const Arena& arena,
                const Player& player, float& health) override;
    void Render(SpriteRenderer& renderer) const override;

private:
    enum class DropState
    {
        Falling,
        Impacting
    };

    struct Drop
    {
        glm::vec2 position{0.0f};
        glm::vec2 target{0.0f};
        float animationTime = 0.0f;
        DropState state = DropState::Falling;
    };

    RainSettings settings_;
    std::mt19937 randomGenerator_{170};
    std::vector<Drop> drops_;
    float phaseTimer_ = 0.0f;
    bool active_ = false;

    void CreatePattern(const Arena& arena);
    void BeginAttack(const Arena& arena);
    void ResetDrop(Drop& drop, const Arena& arena) const;
    void BeginImpact(Drop& drop) const;
};

} // namespace deltarune
