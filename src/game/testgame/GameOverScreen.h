#pragma once

#include <vector>
#include <functional>

#include "Button.h"
#include "RenderContext.h"

// Tela de fim de jogo: mensagem de derrota, tempo de sobrevivencia e os
// botoes "Tentar novamente" / "Menu inicial".
class GameOverScreen
{
public:
    struct Assets
    {
        GLuint fontTexture = 0; // atlas "font.png" (6x7), usado para titulo, tempo e labels
    };

    void setup(const Assets& assets, std::function<void()> onRetry, std::function<void()> onMenu)
    {
        m_assets = assets;
        m_onRetry = std::move(onRetry);
        m_onMenu = std::move(onMenu);
        rebuildButtons();
    }

    void rebuildButtons()
    {
        m_buttons.clear();

        m_buttons.push_back(Button{
            0.0f, -0.2f, 0.6f, 0.15f,
            0.10f, 0.85f, 0.18f,
            "TENTAR NOVAMENTE",
            [this]() { if (m_onRetry) m_onRetry(); }
        });

        m_buttons.push_back(Button{
            0.0f, -0.45f, 0.6f, 0.15f,
            0.30f, 0.30f, 0.30f,
            "MENU",
            [this]() { if (m_onMenu) m_onMenu(); }
        });
    }

    // Chamado uma vez, ao transicionar para GAME_OVER, para fixar o tempo
    // de sobrevivencia que sera exibido nesta tela.
    void setSurvivalTime(double seconds)
    {
        m_survivalTimeSeconds = seconds;
    }

    void render(const RenderContext& ctx) const
    {
        drawRect(ctx, 0.0f, 0.0f, 2.4f, 1.8f, 0.05f, 0.0f, 0.0f);

        drawTextCentered(ctx, m_assets.fontTexture, "GAME OVER", 0.0f, 0.4f, 0.10f, 0.14f);

        int wholeSeconds = static_cast<int>(m_survivalTimeSeconds);
        drawTextCentered(ctx, m_assets.fontTexture, "TEMPO " + std::to_string(wholeSeconds),
                         0.0f, 0.15f, 0.07f, 0.09f);

        for (const Button& button : m_buttons)
        {
            drawRect(ctx, button.x, button.y, button.width, button.height,
                     button.r, button.g, button.b);
            drawButtonLabel(ctx, m_assets.fontTexture, button);
        }
    }

    const std::vector<Button>& buttons() const { return m_buttons; }

private:
    Assets m_assets;
    std::vector<Button> m_buttons;
    std::function<void()> m_onRetry;
    std::function<void()> m_onMenu;
    double m_survivalTimeSeconds = 0.0;
};
