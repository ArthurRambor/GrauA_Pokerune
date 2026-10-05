#include "RainAttack.h"

#include <algorithm>
#include <cmath>

#include "ResourceManager.h"

namespace deltarune
{

RainAttack::RainAttack(RainSettings settings)
    : settings_(settings), phaseTimer_(settings.initialDelay)
{
}

void RainAttack::Start(const Arena&)
{
    // Start means initialize this attack for a level. The first pattern still
    // respects its configured initial delay.
    active_ = false;
    phaseTimer_ = settings_.initialDelay;
    drops_.clear();
}

void RainAttack::Update(float deltaTime, const Arena& arena,
                        const Player& player, float& health)
{
    if (!active_)
    {
        phaseTimer_ -= deltaTime;
        if (phaseTimer_ <= 0.0f)
            BeginAttack(arena);
        return;
    }

    phaseTimer_ -= deltaTime;
    const Rect playerHitbox = player.Hitbox();

    for (Drop& drop : drops_)
    {
        if (drop.state == DropState::Impacting)
        {
            // The sprite sheet is used only for the short impact animation.
            drop.animationTime += deltaTime;
            if (drop.animationTime >=
                settings_.impactFrameDuration * settings_.impactFrameCount)
            {
                ResetDrop(drop, arena);
            }
            continue;
        }

        drop.position.y += settings_.dropSpeed * deltaTime;
        const Rect dropHitbox = RectFromCenter(drop.position, settings_.hitboxSize);

        if (Intersects(playerHitbox, dropHitbox))
        {
            health = std::max(0.0f, health - settings_.damage);
            BeginImpact(drop);
        }
        else if (drop.position.y >= drop.target.y)
        {
            drop.position.y = drop.target.y;
            BeginImpact(drop);
        }
    }

    if (phaseTimer_ <= 0.0f)
    {
        active_ = false;
        phaseTimer_ = settings_.cooldown;
        drops_.clear();
    }
}

void RainAttack::BeginAttack(const Arena& arena)
{
    CreatePattern(arena);
    active_ = true;
    phaseTimer_ = settings_.activeDuration;
}

void RainAttack::Render(SpriteRenderer& renderer) const
{
    if (!active_)
        return;

    const Texture2D& texture = ResourceManager::GetTexture("rain");
    for (const Drop& drop : drops_)
    {
        int frame = 0;
        if (drop.state == DropState::Impacting)
        {
            frame = std::min(
                settings_.impactFrameCount - 1,
                static_cast<int>(drop.animationTime / settings_.impactFrameDuration)
            );
        }

        const int columns = 4;
        const int row = frame / columns;
        const int column = frame % columns;
        const float frameWidth = 1.0f / columns;
        const float frameHeight = 1.0f / 2.0f;
        const glm::vec4 uvRect(
            column * frameWidth,
            row * frameHeight,
            (column + 1) * frameWidth,
            (row + 1) * frameHeight
        );

        renderer.DrawSprite(
            texture,
            drop.position - settings_.visualSize * 0.5f,
            settings_.visualSize,
            glm::vec4(1.0f),
            0.0f,
            uvRect
        );
    }
}

void RainAttack::CreatePattern(const Arena& arena)
{
    drops_.clear();
    drops_.reserve(static_cast<std::size_t>(std::max(0, settings_.dropCount)));

    const Rect& bounds = arena.Bounds();
    std::uniform_real_distribution<float> targetX(
        bounds.position.x + settings_.targetMargin,
        bounds.position.x + bounds.size.x - settings_.targetMargin
    );
    std::uniform_real_distribution<float> targetY(
        bounds.position.y + settings_.targetMargin,
        bounds.position.y + bounds.size.y - settings_.targetMargin
    );

    for (int index = 0; index < settings_.dropCount; ++index)
    {
        glm::vec2 target(0.0f);
        bool farEnough = false;

        // A minimum distance makes the pattern readable and prevents several
        // drops from becoming one visually impossible-to-dodge cluster.
        for (int attempt = 0; attempt < 100 && !farEnough; ++attempt)
        {
            target = {targetX(randomGenerator_), targetY(randomGenerator_)};
            farEnough = true;
            for (const Drop& existing : drops_)
            {
                if (glm::distance(target, existing.target) < settings_.minimumTargetDistance)
                {
                    farEnough = false;
                    break;
                }
            }
        }

        Drop drop;
        drop.target = target;
        ResetDrop(drop, arena);
        drops_.push_back(drop);
    }
}

void RainAttack::ResetDrop(Drop& drop, const Arena& arena) const
{
    drop.position = {drop.target.x, arena.Bounds().position.y - settings_.spawnHeight};
    drop.animationTime = 0.0f;
    drop.state = DropState::Falling;
}

void RainAttack::BeginImpact(Drop& drop) const
{
    drop.state = DropState::Impacting;
    drop.animationTime = 0.0f;
}

} // namespace deltarune
