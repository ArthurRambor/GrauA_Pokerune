#pragma once

#include <array>

#include "Attack.h"
#include "Player.h"

namespace deltarune
{

class BattleScene
{
public:
    BattleScene();

    void LoadLevel(int levelNumber);
    void ProcessInput(float deltaTime, const std::array<bool, 1024>& keys);
    void Update(float deltaTime);
    void Render(SpriteRenderer& renderer) const;

    bool IsGameOver() const { return health_ <= 0.0f; }

private:
    Arena arena_;
    Player player_;
    AttackManager attacks_;
    float health_ = 1.0f;
    int currentLevel_ = 1;

    void RenderArena(SpriteRenderer& renderer) const;
    void RenderHealthBar(SpriteRenderer& renderer) const;
    void RenderChinchou(SpriteRenderer& renderer) const;
};

} // namespace deltarune
