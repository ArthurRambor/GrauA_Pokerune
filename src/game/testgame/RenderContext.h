#pragma once

#include <glad/glad.h>
#include <algorithm>
#include <string>

#include "Button.h"

// Agrupa tudo que as antigas drawRectangle/drawSprite do testgame recebiam
// como parametros soltos (shader, VAO, localizacoes de uniform). Uma
// instancia e montada uma vez no main() e passada por referencia para o
// jogo e para as telas (MenuScreen, GameOverScreen).
struct RenderContext
{
    GLuint colorShader = 0;
    GLuint spriteShader = 0;
    GLuint quadVao = 0;
    GLuint lineVao = 0;
    GLuint triangleVao = 0; // usado pelas setas do carrossel de inimigos

    GLint colorProjectionLoc = -1;
    GLint useRectangleLoc = -1;
    GLint rectanglePositionLoc = -1;
    GLint rectangleSizeLoc = -1;
    GLint colorLoc = -1;

    GLint spriteProjectionLoc = -1;
    GLint spritePositionLoc = -1;
    GLint spriteSizeLoc = -1;
    GLint spriteUvLoc = -1;
    GLint textureLoc = -1;
    GLint chromaKeyLoc = -1;
    GLint chromaKeyColorLoc = -1;

    const GLfloat* projection = nullptr; // atualizado a cada frame no loop principal
};

// Equivalente a antiga drawRectangle(...) do testgame, so que lendo o
// shader/uniforms do RenderContext em vez de receber uma dezena de parametros.
inline void drawRect(const RenderContext& ctx, float x, float y, float width,
                      float height, float red, float green, float blue)
{
    glUseProgram(ctx.colorShader);
    glUniformMatrix4fv(ctx.colorProjectionLoc, 1, GL_FALSE, ctx.projection);
    glUniform1i(ctx.useRectangleLoc, GL_TRUE);
    glUniform2f(ctx.rectanglePositionLoc, x, y);
    glUniform2f(ctx.rectangleSizeLoc, width, height);
    glUniform4f(ctx.colorLoc, red, green, blue, 1.0f);
    glBindVertexArray(ctx.quadVao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

// Equivalente a antiga drawSprite(...) do testgame.
inline void drawSpriteCtx(const RenderContext& ctx, GLuint texture, float x, float y,
                           float width, float height, int columns, int rows, int frame,
                           float regionWidth, float regionHeight, bool useChromaKey)
{
    const int safeFrame = std::max(0, frame % (columns * rows));
    const int column = safeFrame % columns;
    const int row = safeFrame / columns;
    const float frameWidth = regionWidth / columns;
    const float frameHeight = regionHeight / rows;
    const GLfloat uv[] = {
        column * frameWidth,
        row * frameHeight,
        (column + 1) * frameWidth,
        (row + 1) * frameHeight
    };

    glUseProgram(ctx.spriteShader);
    glUniformMatrix4fv(ctx.spriteProjectionLoc, 1, GL_FALSE, ctx.projection);
    glUniform2f(ctx.spritePositionLoc, x, y);
    glUniform2f(ctx.spriteSizeLoc, width, height);
    glUniform4fv(ctx.spriteUvLoc, 1, uv);
    glUniform1i(ctx.textureLoc, 0);
    glUniform1i(ctx.chromaKeyLoc, useChromaKey ? GL_TRUE : GL_FALSE);
    glUniform3f(ctx.chromaKeyColorLoc, 199.0f / 255.0f, 225.0f / 255.0f, 209.0f / 255.0f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(ctx.quadVao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

// Desenha o contorno da arena (equivalente ao trecho de GL_LINES que ja
// existia solto no main() do testgame).
inline void drawBoxOutline(const RenderContext& ctx)
{
    glUseProgram(ctx.colorShader);
    glUniformMatrix4fv(ctx.colorProjectionLoc, 1, GL_FALSE, ctx.projection);
    glUniform1i(ctx.useRectangleLoc, GL_FALSE);
    glUniform4f(ctx.colorLoc, 0.0f, 1.0f, 0.0f, 1.0f);
    glBindVertexArray(ctx.lineVao);
    glLineWidth(5.0f);
    glDrawArrays(GL_LINES, 0, 8);
}

// Desenha uma seta triangular (VAO de 3 vertices, mesmo shader de cor das
// caixas). size e a "altura" da seta; para aponta-la para a esquerda basta
// passar pointRight=false, que espelha o triangulo (equivalente ao antigo
// par de botoes "A"/"P" do carrossel de inimigos).
inline void drawArrow(const RenderContext& ctx, float x, float y, float size,
                       bool pointRight, float red, float green, float blue)
{
    glUseProgram(ctx.colorShader);
    glUniformMatrix4fv(ctx.colorProjectionLoc, 1, GL_FALSE, ctx.projection);
    glUniform1i(ctx.useRectangleLoc, GL_TRUE);
    glUniform2f(ctx.rectanglePositionLoc, x, y);
    glUniform2f(ctx.rectangleSizeLoc, pointRight ? size : -size, size);
    glUniform4f(ctx.colorLoc, red, green, blue, 1.0f);
    glBindVertexArray(ctx.triangleVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

// Desenha texto usando o atlas de fonte "font.png" (grade 6 colunas x 7
// linhas). O indice de cada caractere na string charOrder corresponde
// exatamente a celula da grade (linha * 6 + coluna). A ordem real da
// ultima linha do atlas e "!", ".", "?", e so entao a celula em branco
// (espaco) - confirmado checando transparencia pixel a pixel do PNG.
// IMPORTANTE: a fonte so tem maiusculas - converta o texto antes se ele
// vier de uma variavel (std::transform(..., ::toupper)).
// "_" e tratado como espaco em branco, para o caso de escreverem
// "ALGO_ASSIM" esperando um espaco em vez de um caractere sublinhado.
inline void drawText(const RenderContext& ctx, GLuint fontTexture, const std::string& text,
                      float startX, float y, float charWidth, float charHeight)
{
    if (fontTexture == 0)
        return; // EM ABERTO: sem font.png carregado ainda, nao desenha nada

    static const std::string charOrder = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!.? ";
    constexpr int columns = 6;
    constexpr int rows = 7;

    float cursorX = startX;
    for (char c : text)
    {
        char lookupChar = (c == '_') ? ' ' : c;
        size_t index = charOrder.find(lookupChar);
        if (index == std::string::npos)
        {
            cursorX += charWidth * 0.9f; // caractere nao mapeado: so avanca o cursor
            continue;
        }

        drawSpriteCtx(ctx, fontTexture, cursorX, y, charWidth, charHeight,
                     columns, rows, static_cast<int>(index), 1.0f, 1.0f, false);
        cursorX += charWidth * 0.9f; // leve sobreposicao/espacamento entre letras
    }
}

// Mesma coisa que drawText, mas centralizado horizontalmente em centerX
// (util para labels de botao e titulos). centerY e passado direto pro
// drawText, ja que a posicao "y" do sprite representa o CENTRO do quad
// (nao o topo) - subtrair charHeight/2 aqui empurraria o texto pra baixo.
inline void drawTextCentered(const RenderContext& ctx, GLuint fontTexture, const std::string& text,
                              float centerX, float centerY, float charWidth, float charHeight)
{
    if (text.empty())
        return;

    float startX = centerX - charWidth * 0.45f * (static_cast<float>(text.size()) - 1.0f);
    drawText(ctx, fontTexture, text, startX, centerY, charWidth, charHeight);
}

// Desenha o retangulo de fundo do botao e o texto do label, encolhendo a
// fonte automaticamente se o texto for largo demais para caber dentro do
// botao (com uma margem), em vez de deixar o texto vazar pra fora.
inline void drawButtonLabel(const RenderContext& ctx, GLuint fontTexture, const Button& button,
                             float maxCharWidth = 0.05f, float maxCharHeight = 0.065f,
                             float horizontalPaddingRatio = 0.78f)
{
    if (fontTexture == 0 || button.label.empty())
        return;

    float charWidth = maxCharWidth;
    float charHeight = maxCharHeight;

    const float n = static_cast<float>(button.label.size());
    float estimatedWidth = charWidth * (1.0f + 0.9f * std::max(0.0f, n - 1.0f));
    float maxAllowedWidth = button.width * horizontalPaddingRatio;

    if (estimatedWidth > maxAllowedWidth && estimatedWidth > 0.0f)
    {
        float scale = maxAllowedWidth / estimatedWidth;
        charWidth *= scale;
        charHeight *= scale;
    }

    float maxAllowedHeight = button.height * 0.7f;
    if (charHeight > maxAllowedHeight)
    {
        float scale = maxAllowedHeight / charHeight;
        charWidth *= scale;
        charHeight *= scale;
    }

    drawTextCentered(ctx, fontTexture, button.label, button.x, button.y, charWidth, charHeight);
}
