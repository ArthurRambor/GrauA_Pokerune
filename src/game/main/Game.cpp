#include "Game.h"

#include <algorithm>

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "ResourceManager.h"

namespace deltarune
{

namespace
{
const char* SPRITE_VERTEX_SHADER = R"glsl(
    #version 330 core

    layout (location = 0) in vec3 position;
    layout (location = 1) in vec2 texCoord;

    uniform mat4 projection;
    uniform mat4 model;
    uniform vec4 uvRect;

    out vec2 TexCoord;

    void main()
    {
        gl_Position = projection * model * vec4(position, 1.0);
        TexCoord = vec2(
            mix(uvRect.x, uvRect.z, texCoord.x),
            mix(uvRect.y, uvRect.w, texCoord.y)
        );
    }
)glsl";

const char* SPRITE_FRAGMENT_SHADER = R"glsl(
    #version 330 core

    in vec2 TexCoord;

    uniform sampler2D image;
    uniform bool useTexture;
    uniform vec4 spriteColor;
    uniform bool useChromaKey;
    uniform vec3 chromaKeyColor;

    out vec4 color;

    void main()
    {
        vec4 sampledColor = useTexture
            ? texture(image, TexCoord)
            : vec4(1.0);

        if (sampledColor.a < 0.05)
            discard;

        if (useChromaKey && distance(sampledColor.rgb, chromaKeyColor) < 0.08)
            discard;

        color = sampledColor * spriteColor;
    }
)glsl";
}

Game::Game(unsigned int logicalWidth, unsigned int logicalHeight)
    : logicalWidth_(logicalWidth), logicalHeight_(logicalHeight)
{
}

bool Game::Init()
{
    if (!ResourceManager::LoadShader(SPRITE_VERTEX_SHADER, SPRITE_FRAGMENT_SHADER, "sprite"))
        return false;

    const bool loadedTextures =
        ResourceManager::LoadTexture("heart.png", "heart") &&
        ResourceManager::LoadTexture("chinchou.png", "chinchou") &&
        ResourceManager::LoadTexture("rain.png", "rain");
    if (!loadedTextures)
        return false;

    renderer_ = std::make_unique<SpriteRenderer>(ResourceManager::GetShader("sprite"));
    StartLevel(1);
    return true;
}

void Game::ProcessInput(float deltaTime)
{
    if (state_ == GameState::Playing)
        battleScene_.ProcessInput(deltaTime, keys_);
}

void Game::Update(float deltaTime)
{
    if (state_ != GameState::Playing)
        return;

    battleScene_.Update(deltaTime);
    if (battleScene_.IsGameOver())
        state_ = GameState::GameOver;
}

void Game::Render()
{
    if (renderer_ == nullptr || framebufferWidth_ <= 0 || framebufferHeight_ <= 0)
        return;

    // Clear the full framebuffer first, including the letterbox bars.
    glViewport(0, 0, framebufferWidth_, framebufferHeight_);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ConfigureGameViewport();
    renderer_->SetProjection(Projection());

    switch (state_)
    {
    case GameState::Home:
        RenderHome();
        break;
    case GameState::Playing:
        battleScene_.Render(*renderer_);
        break;
    case GameState::GameOver:
        RenderGameOver();
        break;
    }
}

void Game::OnKey(int key, int action)
{
    if (key < 0 || key >= static_cast<int>(keys_.size()))
        return;

    if (action == GLFW_PRESS || action == GLFW_REPEAT)
        keys_[key] = true;
    else if (action == GLFW_RELEASE)
        keys_[key] = false;

    if (action != GLFW_PRESS)
        return;

    if (state_ == GameState::Home && key == GLFW_KEY_ENTER)
    {
        StartLevel(1);
    }
    else if (state_ == GameState::GameOver && key == GLFW_KEY_ENTER)
    {
        state_ = GameState::Home;
    }
}

void Game::OnFramebufferResized(int width, int height)
{
    framebufferWidth_ = width;
    framebufferHeight_ = height;
}

glm::mat4 Game::Projection() const
{
    return glm::ortho(
        0.0f,
        static_cast<float>(logicalWidth_),
        static_cast<float>(logicalHeight_),
        0.0f,
        -1.0f,
        1.0f
    );
}

void Game::ConfigureGameViewport() const
{
    const float scale = std::min(
        static_cast<float>(framebufferWidth_) / logicalWidth_,
        static_cast<float>(framebufferHeight_) / logicalHeight_
    );
    const int viewportWidth = static_cast<int>(logicalWidth_ * scale);
    const int viewportHeight = static_cast<int>(logicalHeight_ * scale);
    const int viewportX = (framebufferWidth_ - viewportWidth) / 2;
    const int viewportY = (framebufferHeight_ - viewportHeight) / 2;
    glViewport(viewportX, viewportY, viewportWidth, viewportHeight);
}

void Game::RenderHome() const
{
    renderer_->DrawQuad(
        {0.0f, 0.0f},
        {static_cast<float>(logicalWidth_), static_cast<float>(logicalHeight_)},
        {0.02f, 0.03f, 0.05f, 1.0f}
    );
    renderer_->DrawQuad(
        {160.0f, 145.0f},
        {480.0f, 310.0f},
        {0.04f, 0.10f, 0.08f, 1.0f}
    );
}

void Game::RenderGameOver() const
{
    renderer_->DrawQuad(
        {0.0f, 0.0f},
        {static_cast<float>(logicalWidth_), static_cast<float>(logicalHeight_)},
        {0.08f, 0.015f, 0.02f, 1.0f}
    );
    renderer_->DrawQuad(
        {160.0f, 145.0f},
        {480.0f, 310.0f},
        {0.22f, 0.03f, 0.04f, 1.0f}
    );
}

void Game::StartLevel(int levelNumber)
{
    battleScene_.LoadLevel(levelNumber);
    state_ = GameState::Playing;
}

} // namespace deltarune
