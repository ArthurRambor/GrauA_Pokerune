#pragma once

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "RenderContext.h"

// Um frame da animacao: retangulo EM PIXELS dentro do PNG original
// (x,y = canto superior esquerdo). As sprite sheets dos bosses nao sao
// grades regulares, entao cada frame e descrito pelo seu proprio retangulo.
struct FrameRect
{
    float x, y, w, h;
};

// Descreve UM inimigo/boss. Cada boss novo = um EnemyDef novo.
struct EnemyDef
{
    std::string name;
    GLuint texture = 0;

    // Tamanho do PNG em pixels (necessario para converter pixels -> UV).
    float textureWidth = 1.0f;
    float textureHeight = 1.0f;

    // Frames da animacao, em ordem de reproducao.
    std::vector<FrameRect> frames;
    float frameDuration = 0.12f; // segundos por frame

    // Cor de fundo a remover (chroma key). Os 3 PNGs usam 199,225,209.
    bool useChromaKey = true;
    float keyR = 199.0f / 255.0f;
    float keyG = 225.0f / 255.0f;
    float keyB = 209.0f / 255.0f;

    // Espelha o sprite horizontalmente (PNGs que olham para a direita, mas o
    // boss fica a direita da arena e deveria olhar para a esquerda).
    bool flipX = false;

    // Mecanica especial: terremoto periodico (tela treme). 0 = desligado.
    float quakeInterval = 0.0f;   // segundos entre terremotos
    float quakeDuration = 0.6f;   // duracao do tremor
    float quakeAmplitude = 0.03f; // deslocamento maximo em unidades de mundo

    // Posicao/tamanho do boss na arena (mundo).
    float x = 0.94f;
    float y = 0.10f;
    float width = 0.30f;
    float height = 0.36f;
};

// Monta frames a partir de uma grade (so o chinchou tem grade regular).
// regionPxW/H = area util da imagem em pixels; validFrames vazio = todos.
inline std::vector<FrameRect> gridFrames(int columns, int rows, float regionPxW,
                                         float regionPxH, const std::vector<int>& validFrames)
{
    std::vector<FrameRect> out;
    const float cw = regionPxW / columns;
    const float ch = regionPxH / rows;
    auto add = [&](int i) { out.push_back({ (i % columns) * cw, (i / columns) * ch, cw, ch }); };
    if (validFrames.empty())
        for (int i = 0; i < columns * rows; ++i) add(i);
    else
        for (int i : validFrames) add(i);
    return out;
}

// Desenha o inimigo no instante "time" (segundos). time=0 -> primeiro frame
// (usado como preview estatico no menu). width/height = tamanho do PRIMEIRO
// frame; frames de tamanho diferente (ex.: o jato dagua do Mudkip) mantem a
// mesma escala em pixels e crescem a partir do lado do corpo (esquerda, ou
// direita se flipX), entao o corpo nao "pula" entre frames.
inline void drawEnemy(const RenderContext& ctx, const EnemyDef& e, float time,
                      float x, float y, float width, float height)
{
    if (e.frames.empty())
        return;

    const int n = static_cast<int>(e.frames.size());
    const FrameRect& f = e.frames[static_cast<int>(time / e.frameDuration) % n];

    const float scaleX = width / e.frames[0].w;
    const float scaleY = height / e.frames[0].h;
    const float w = f.w * scaleX;
    const float h = f.h * scaleY;
    const float cx = e.flipX ? (x + width / 2.0f - w / 2.0f)
                             : (x - width / 2.0f + w / 2.0f);

    float u0 = f.x / e.textureWidth;
    float u1 = (f.x + f.w) / e.textureWidth;
    if (e.flipX)
        std::swap(u0, u1);

    drawSpriteUv(ctx, e.texture, cx, y, w, h,
                 u0, f.y / e.textureHeight,
                 u1, (f.y + f.h) / e.textureHeight,
                 e.useChromaKey, e.keyR, e.keyG, e.keyB);
}
