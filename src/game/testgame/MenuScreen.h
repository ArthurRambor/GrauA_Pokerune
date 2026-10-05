#pragma once

#include <vector>
#include <functional>

#include "Button.h"
#include "RenderContext.h"

enum class Theme { LIGHT, DARK };

// Tela inicial: titulo, selecao de tema, selecao de inimigo, selecao de
// dificuldade e botao "Jogar".
class MenuScreen
{
public:
    // Texturas que a tela precisa para desenhar (titulo, previews de
    // inimigo). Todas carregadas previamente no main() com loadTexture().
    struct Assets
    {
        GLuint fontTexture = 0; // atlas "font.png" (6x7), usado para titulo e labels

        // EM ABERTO: hoje so existe o sprite do chinchou (rain.png/chinchou.png).
        // Adicionar novas texturas de inimigo aqui para o seletor funcionar
        // com mais de uma opcao.
        std::vector<GLuint> enemyPreviewTextures;
    };

    void setup(const Assets& assets, std::function<void()> onPlay)
    {
        m_assets = assets;
        m_onPlay = std::move(onPlay);
        rebuildButtons();
    }

    // Reconstroi a lista de botoes clicaveis com base no estado atual
    // (tema e dificuldade selecionados mudam a cor/estado visual deles).
    // Chame sempre que uma opcao mudar, ou ao reentrar no estado MENU.
    void rebuildButtons()
    {
        m_buttons.clear();

        // Botao "Jogar"
        m_buttons.push_back(Button{
            0.0f, -0.55f, 0.5f, 0.15f,
            0.10f, 0.85f, 0.18f,
            "JOGAR",
            [this]() { if (m_onPlay) m_onPlay(); }
        });

        // Selecao de tema (dois botoes lado a lado)
        m_buttons.push_back(Button{
            -0.3f, 0.15f, 0.25f, 0.12f,
            0.9f, 0.9f, 0.9f,
            "CLARO",
            [this]() { m_theme = Theme::LIGHT; rebuildButtons(); }
        });
        m_buttons.push_back(Button{
            0.3f, 0.15f, 0.25f, 0.12f,
            0.2f, 0.2f, 0.2f,
            "ESCURO",
            [this]() { m_theme = Theme::DARK; rebuildButtons(); }
        });

        // Setas de selecao de inimigo (anterior / proximo). O label vira
        // um marcador interno ("<<ARROW_PREV>>"/"<<ARROW_NEXT>>") que o
        // render() reconhece para desenhar uma seta triangular em vez de
        // texto (a fonte nao tem simbolo de seta).
        if (!m_assets.enemyPreviewTextures.empty())
        {
            m_buttons.push_back(Button{
                -0.35f, -0.05f, 0.12f, 0.12f,
                0.3f, 0.3f, 0.3f,
                "<<ARROW_PREV>>",
                [this]() {
                    int count = static_cast<int>(m_assets.enemyPreviewTextures.size());
                    m_enemyIndex = (m_enemyIndex - 1 + count) % count;
                }
            });
            m_buttons.push_back(Button{
                0.35f, -0.05f, 0.12f, 0.12f,
                0.3f, 0.3f, 0.3f,
                "<<ARROW_NEXT>>",
                [this]() {
                    int count = static_cast<int>(m_assets.enemyPreviewTextures.size());
                    m_enemyIndex = (m_enemyIndex + 1) % count;
                }
            });
        }

        // Selecao de dificuldade (3 botoes)
        const char* diffNames[3] = { "FACIL", "MEDIO", "DIFICIL" };
        for (int i = 0; i < 3; i++)
        {
            float x = -0.5f + i * 0.5f;
            bool active = (m_difficulty == i);
            m_buttons.push_back(Button{
                x, -0.30f, 0.4f, 0.10f,
                active ? 0.85f : 0.25f, active ? 0.65f : 0.25f, 0.10f,
                diffNames[i],
                [this, i]() { m_difficulty = i; rebuildButtons(); }
            });
        }
    }

    void render(const RenderContext& ctx) const
    {
        float bg = (m_theme == Theme::LIGHT) ? 0.92f : 0.05f;
        drawRect(ctx, 0.0f, 0.0f, 2.4f, 1.8f, bg, bg, bg);

        // EM ABERTO: a fonte e desenhada sempre na cor original do PNG
        // (provavelmente branco). Para o titulo ficar legivel em ambos os
        // temas, ou o font.png precisa ter contraste com os dois fundos,
        // ou o shader de sprite passa a aceitar um tint de cor (uniform
        // extra multiplicando o resultado da textura).
        drawTextCentered(ctx, m_assets.fontTexture, "RAIN ATTACK", 0.0f, 0.65f, 0.10f, 0.13f);

        if (!m_assets.enemyPreviewTextures.empty())
        {
            // A sprite sheet do chinchou (chinchou.png) tem uma grade real
            // de 4 colunas x 4 linhas dentro da area util (240x183 de um
            // arquivo 403x183 - o resto e credito/sprite shiny). So 12 das
            // 16 celulas tem pose desenhada; o frame 0 (topo-esquerda) e
            // uma pose completa e limpa, boa para preview estatico.
            drawSpriteCtx(ctx, m_assets.enemyPreviewTextures[m_enemyIndex],
                          0.0f, -0.05f, 0.3f, 0.3f, 4, 4, 0, 240.0f / 403.0f, 1.0f, true);
        }

        for (const Button& button : m_buttons)
        {
            drawRect(ctx, button.x, button.y, button.width, button.height,
                     button.r, button.g, button.b);

            if (button.label == "<<ARROW_PREV>>")
            {
                drawArrow(ctx, button.x, button.y, button.width * 0.5f, false, 1.0f, 1.0f, 1.0f);
            }
            else if (button.label == "<<ARROW_NEXT>>")
            {
                drawArrow(ctx, button.x, button.y, button.width * 0.5f, true, 1.0f, 1.0f, 1.0f);
            }
            else
            {
                // drawButtonLabel encolhe a fonte automaticamente se o
                // texto for largo demais para o botao, evitando o texto
                // vazar pra fora dele.
                drawButtonLabel(ctx, m_assets.fontTexture, button);
            }
        }
    }

    const std::vector<Button>& buttons() const { return m_buttons; }

    Theme theme() const { return m_theme; }
    int enemyIndex() const { return m_enemyIndex; }
    int difficulty() const { return m_difficulty; } // 0=facil, 1=medio, 2=dificil

private:
    Assets m_assets;
    std::vector<Button> m_buttons;
    std::function<void()> m_onPlay;

    Theme m_theme = Theme::DARK;
    int m_enemyIndex = 0;
    int m_difficulty = 1;
};