#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "Arena.h"
#include "Player.h"

namespace deltarune
{

class Attack
{
public:
    virtual ~Attack() = default;

    virtual void Start(const Arena& arena) = 0;
    virtual void Update(float deltaTime, const Arena& arena,
                        const Player& player, float& health) = 0;
    virtual void Render(SpriteRenderer& renderer) const = 0;
};

class AttackManager
{
public:
    template <typename T, typename... Arguments>
    T& Add(Arguments&&... arguments)
    {
        auto attack = std::make_unique<T>(std::forward<Arguments>(arguments)...);
        T& result = *attack;
        attacks_.push_back(std::move(attack));
        return result;
    }

    void StartAll(const Arena& arena);
    void Update(float deltaTime, const Arena& arena,
                const Player& player, float& health);
    void Render(SpriteRenderer& renderer) const;
    void Clear();

private:
    std::vector<std::unique_ptr<Attack>> attacks_;
};

} // namespace deltarune
