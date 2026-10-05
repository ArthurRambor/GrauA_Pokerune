#pragma once

#include <functional>
#include <string>

// Botao simples: retangulo clicavel em coordenadas de mundo (mesmo sistema
// usado pelo heart/box no jogo), com uma cor de fundo e uma callback.
struct Button
{
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;

    // EM ABERTO: por enquanto o botao e so um retangulo colorido.
    // Quando o atlas de texto/labels estiver pronto, "label" pode indicar
    // qual textura desenhar por cima do retangulo (ver drawSpriteCtx).
    std::string label;

    std::function<void()> onClick;

    bool contains(float worldX, float worldY) const
    {
        return worldX >= x - width / 2.0f && worldX <= x + width / 2.0f &&
               worldY >= y - height / 2.0f && worldY <= y + height / 2.0f;
    }
};
