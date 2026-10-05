#pragma once

#include <array>
#include <memory>

#include <glm/glm.hpp>

#include "BattleScene.h"
#include "GameTypes.h"
#include "SpriteRenderer.h"

struct GLFWwindow;

namespace deltarune
{

class Game
{
public:
    Game(unsigned int logicalWidth, unsigned int logicalHeight);
    ~Game() = default;

    bool Init();
    void ProcessInput(float deltaTime);
    void Update(float deltaTime);
    void Render();

    void OnKey(int key, int action);
    void OnFramebufferResized(int width, int height);

private:
    unsigned int logicalWidth_;
    unsigned int logicalHeight_;
    int framebufferWidth_ = 0;
    int framebufferHeight_ = 0;
    std::array<bool, 1024> keys_{};
    GameState state_ = GameState::Playing;
    std::unique_ptr<SpriteRenderer> renderer_;
    BattleScene battleScene_;

    glm::mat4 Projection() const;
    void ConfigureGameViewport() const;
    void RenderHome() const;
    void RenderGameOver() const;
    void StartLevel(int levelNumber);
};

} // namespace deltarune
