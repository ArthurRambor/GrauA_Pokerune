#include "Attack.h"

namespace deltarune
{

void AttackManager::StartAll(const Arena& arena)
{
    for (const std::unique_ptr<Attack>& attack : attacks_)
        attack->Start(arena);
}

void AttackManager::Update(float deltaTime, const Arena& arena,
                           const Player& player, float& health)
{
    // Every attack receives the same frame. New attacks can be added without
    // changing the battle scene, and multiple attacks can run together.
    for (const std::unique_ptr<Attack>& attack : attacks_)
        attack->Update(deltaTime, arena, player, health);
}

void AttackManager::Render(SpriteRenderer& renderer) const
{
    for (const std::unique_ptr<Attack>& attack : attacks_)
        attack->Render(renderer);
}

void AttackManager::Clear()
{
    attacks_.clear();
}

} // namespace deltarune
