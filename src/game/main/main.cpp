#include <algorithm>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Game.h"
#include "GameTypes.h"
#include "ResourceManager.h"

namespace
{

void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    auto* game = static_cast<deltarune::Game*>(glfwGetWindowUserPointer(window));
    if (game != nullptr)
        game->OnFramebufferResized(width, height);
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)scancode;
    (void)mods;

    auto* game = static_cast<deltarune::Game*>(glfwGetWindowUserPointer(window));
    if (game != nullptr)
        game->OnKey(key, action);

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

} // namespace

int main()
{
    if (!glfwInit())
    {
        std::cerr << "Could not initialize GLFW" << std::endl;
        return -1;
    }

    // Core profile keeps the renderer on modern OpenGL APIs. The game does
    // not rely on deprecated immediate-mode or fixed-function rendering.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(
        deltarune::LOGICAL_WIDTH,
        deltarune::LOGICAL_HEIGHT,
        "Deltarune Battle Prototype",
        nullptr,
        nullptr
    );
    if (window == nullptr)
    {
        std::cerr << "Could not create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        std::cerr << "Could not initialize GLAD" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int exitCode = 0;
    {
        deltarune::Game game(deltarune::LOGICAL_WIDTH, deltarune::LOGICAL_HEIGHT);
        glfwSetWindowUserPointer(window, &game);
        glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
        glfwSetKeyCallback(window, KeyCallback);

        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
        game.OnFramebufferResized(framebufferWidth, framebufferHeight);

        if (!game.Init())
        {
            std::cerr << "Could not initialize game resources" << std::endl;
            exitCode = -1;
        }
        else
        {
            double previousTime = glfwGetTime();
            while (!glfwWindowShouldClose(window))
            {
                const double currentTime = glfwGetTime();
                const float deltaTime = std::min(
                    static_cast<float>(currentTime - previousTime),
                    0.05f
                );
                previousTime = currentTime;

                glfwPollEvents();
                game.ProcessInput(deltaTime);
                game.Update(deltaTime);
                game.Render();
                glfwSwapBuffers(window);
            }
        }

    }

    // ResourceManager is cleared after Game so the renderer no longer holds
    // references to resource objects when their OpenGL handles are deleted.
    deltarune::ResourceManager::Clear();
    glfwDestroyWindow(window);
    glfwTerminate();
    return exitCode;
}
