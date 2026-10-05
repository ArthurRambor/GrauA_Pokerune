#include "BattleScene.h"

#include <algorithm>

#include <glm/glm.hpp>

#include "ResourceManager.h"
#include "RainAttack.h"

namespace deltarune
{

namespace
{
const glm::vec4 ARENA_COLOR(0.0f, 0.85f, 0.18f, 1.0f);
const glm::vec4 HEALTH_BACKGROUND(0.25f, 0.04f, 0.04f, 1.0f);
const glm::vec4 HEALTH_FILL(0.10f, 0.85f, 0.18f, 1.0f);
}

BattleScene::BattleScene()
    : arena_({120.0f, 70.0f}, {500.0f, 400.0f}, 12.0f)
{
}

void BattleScene::LoadLevel(int levelNumber)
{
    currentLevel_ = levelNumber;
    health_ = 1.0f;
    player_.Reset(arena_);
    attacks_.Clear();

    // Levels will eventually construct different attack sets here. Each
    // attack is independent, so adding a second one does not change the loop.
    RainSettings rainSettings;
    attacks_.Add<RainAttack>(rainSettings);
    attacks_.StartAll(arena_);
}

void BattleScene::ProcessInput(float deltaTime, const std::array<bool, 1024>& keys)
{
    player_.ProcessInput(deltaTime, keys, arena_);
}

void BattleScene::Update(float deltaTime)
{
    attacks_.Update(deltaTime, arena_, player_, health_);
}

void BattleScene::Render(SpriteRenderer& renderer) const
{
    RenderHealthBar(renderer);
    RenderArena(renderer);
    RenderChinchou(renderer);
    attacks_.Render(renderer);
    player_.Render(renderer, ResourceManager::GetTexture("heart"));
}

void BattleScene::RenderArena(SpriteRenderer& renderer) const
{
    const Rect& bounds = arena_.Bounds();
    const float thickness = arena_.WallThickness();

    renderer.DrawQuad(
        {bounds.position.x, bounds.position.y},
        {bounds.size.x, thickness},
        ARENA_COLOR
    );
    renderer.DrawQuad(
        {bounds.position.x, bounds.position.y + bounds.size.y - thickness},
        {bounds.size.x, thickness},
        ARENA_COLOR
    );
    renderer.DrawQuad(
        {bounds.position.x, bounds.position.y},
        {thickness, bounds.size.y},
        ARENA_COLOR
    );
    renderer.DrawQuad(
        {bounds.position.x + bounds.size.x - thickness, bounds.position.y},
        {thickness, bounds.size.y},
        ARENA_COLOR
    );
}

void BattleScene::RenderHealthBar(SpriteRenderer& renderer) const
{
    const Rect& bounds = arena_.Bounds();
    const float barHeight = 14.0f;
    const float barY = bounds.position.y + bounds.size.y + 24.0f;
    const float clampedHealth = std::clamp(health_, 0.0f, 1.0f);

    renderer.DrawQuad(
        {bounds.position.x, barY},
        {bounds.size.x, barHeight},
        HEALTH_BACKGROUND
    );
    renderer.DrawQuad(
        {bounds.position.x, barY},
        {bounds.size.x * clampedHealth, barHeight},
        HEALTH_FILL
    );
}

void BattleScene::RenderChinchou(SpriteRenderer& renderer) const
{
    // The first cell is the only frame used for now. The UV is kept explicit
    // so a future animation component can replace this without moving scene code.
    const glm::vec4 firstFrame(
        0.0f,
        0.0f,
        (240.0f / 403.0f) / 5.0f,
        1.0f / 3.0f
    );
    renderer.DrawSprite(
        ResourceManager::GetTexture("chinchou"),
        {650.0f, 180.0f},
        {100.0f, 120.0f},
        glm::vec4(1.0f),
        0.0f,
        firstFrame,
        true,
        {199.0f / 255.0f, 225.0f / 255.0f, 209.0f / 255.0f}
    );
}

} // namespace deltarune
