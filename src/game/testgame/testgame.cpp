#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Button.h"
#include "RenderContext.h"
#include "MenuScreen.h"
#include "GameOverScreen.h"
#include "Enemy.h"

using namespace std;

const GLuint WIDTH = 800;
const GLuint HEIGHT = 600;

// The game world keeps this aspect ratio while the window is resized.
constexpr float VIEW_HALF_WIDTH = 1.20f;
constexpr float VIEW_HALF_HEIGHT = 0.90f;
constexpr float ATTACK_DELAY = 3.0f;

// Intervalo entre gotas (segundos), indexado pela dificuldade selecionada
// no menu (0=facil, 1=medio, 2=dificil). Quanto menor o intervalo, mais
// rapido as gotas aparecem - por isso os valores caem conforme a
// dificuldade sobe. EM ABERTO: numeros de partida, ajustar por playtest.
constexpr float DROP_SPAWN_INTERVALS[3] = { 0.40f, 0.30f, 0.20f };

// Mecanica de FLOOD (linha do Squirtle): cada gota que toca o chao sobe o
// nivel da agua. Tudo indexado pela dificuldade (0=facil, 1=medio, 2=dificil):
// quanto mais dificil, mais a agua sobe por gota, mais alto ela chega e mais
// dano causa enquanto o coracao esta submerso. EM ABERTO: ajustar por playtest.
constexpr float FLOOD_RISE_PER_DROP[3] = { 0.008f, 0.012f, 0.018f };
constexpr float FLOOD_MAX_HEIGHT[3] = { 0.40f, 0.50f, 0.62f };
constexpr float FLOOD_DAMAGE_PER_SECOND[3] = { 0.08f, 0.12f, 0.18f };
constexpr float DROP_SIZE = 0.13f;

// Mecanica de FLASH BANG (linha do Chinchou): intervalo entre piscadas por
// dificuldade (menor = mais frequente). A tela fica branca por FLASH_HOLD
// segundos e depois some em FLASH_FADE segundos. EM ABERTO: ajustar por playtest.
//                                        facil  medio  dificil
constexpr float BEAM_SPAWN_INTERVALS[3] = { 1.8f,  1.2f,  0.7f };
constexpr float BEAM_SPEED  = 1.1f;
constexpr float BEAM_SIZE   = 0.07f;
constexpr float BEAM_HITBOX = 0.07f;constexpr float FLASH_HOLD = 0.25f;
constexpr float FLASH_FADE = 0.90f;

// Mecanica de SHURIKEN (linha do Greninja): cai e quica nas paredes ate
// acabar os quiques. Dano por dificuldade (gota normal = 0.10).
// EM ABERTO: ajustar por playtest.
constexpr float SHURIKEN_SPAWN_INTERVAL = 3.0f;
constexpr float SHURIKEN_DAMAGE[3] = { 0.15f, 0.22f, 0.30f };
constexpr float SHURIKEN_SIZE = 0.14f;
constexpr float SHURIKEN_HITBOX = 0.10f;
constexpr float SHURIKEN_SPEED = 0.85f;
constexpr int SHURIKEN_MAX_BOUNCES = 6;
constexpr float DROPLET_TEXTURE_SIZE = 0.26f;
constexpr float DROPLET_WIDTH = 0.09f;
constexpr float DROPLET_HEIGHT = 0.14f;


const GLchar* colorVertexShaderSource = R"glsl(
    #version 330 core

    layout (location = 0) in vec3 position;

    uniform mat4 projection;
    uniform bool useRectangle;
    uniform vec2 rectanglePosition;
    uniform vec2 rectangleSize;

    void main()
    {
        vec2 finalPosition = useRectangle
            ? position.xy * rectangleSize + rectanglePosition
            : position.xy;

        gl_Position = projection * vec4(finalPosition, position.z, 1.0);
    }
)glsl";

const GLchar* colorFragmentShaderSource = R"glsl(
    #version 330 core

    uniform vec4 inputColor;
    out vec4 color;

    void main()
    {
        color = inputColor;
    }
)glsl";

const GLchar* spriteVertexShaderSource = R"glsl(
    #version 330 core

    layout (location = 0) in vec3 position;
    layout (location = 1) in vec2 texCoord;

    uniform mat4 projection;
    uniform vec2 spritePosition;
    uniform vec2 spriteSize;
    uniform vec4 spriteUv;
    uniform float spriteAngle;

    out vec2 TexCoord;

    void main()
    {
        vec2 local = position.xy * spriteSize;
        float c = cos(spriteAngle);
        float s = sin(spriteAngle);
        local = vec2(c * local.x - s * local.y, s * local.x + c * local.y);
        vec2 finalPosition = local + spritePosition;
        gl_Position = projection * vec4(finalPosition, position.z, 1.0);

        TexCoord = vec2(
            mix(spriteUv.x, spriteUv.z, texCoord.x),
            mix(spriteUv.y, spriteUv.w, texCoord.y)
        );
    }
)glsl";

const GLchar* spriteFragmentShaderSource = R"glsl(
    #version 330 core

    in vec2 TexCoord;

    uniform sampler2D texture1;
    uniform bool useChromaKey;
    uniform vec3 chromaKeyColor;
    uniform float spriteAlpha;   
    uniform float spriteWhiten;

    out vec4 color;

    void main()
    {
        vec4 sampledColor = texture(texture1, TexCoord);

        if (sampledColor.a < 0.05)
            discard;

        if (useChromaKey && distance(sampledColor.rgb, chromaKeyColor) < 0.08)
            discard;

        color = vec4(mix(sampledColor.rgb, vec3(1.0), spriteWhiten),
             sampledColor.a * spriteAlpha);
    }
)glsl";


struct Hitbox
{
    float x;
    float y;
    float width;
    float height;
};

struct Heart
{
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.10f;
    float height = 0.10f;
    float speed = 0.80f;
    float hurtTimer = 0.0f;   
};



struct Box
{
    float left = -0.75f;
    float right = 0.75f;
    float bottom = -0.60f;
    float top = 0.60f;
    float wallThickness = 0.04f;
};

struct AttackDrop
{
    float x;
    float y;
    float speed;
    float age = 0.0f;
};

struct Shuriken
{
    float x;
    float y;
    float vx;
    float vy;
    float age = 0.0f;
    int bounces = 0;
};

struct LanturnBeam
{
    float x, y;
    float vx, vy;
};

vector<LanturnBeam> g_beams;
float g_beamTimer = 0.0f;

bool checkCollision(const Hitbox& a, const Hitbox& b)
{
    return a.x < b.x + b.width &&
           a.x + a.width > b.x &&
           a.y < b.y + b.height &&
           a.y + a.height > b.y;
}

// ---------------------------------------------------------------------
// Utilitarios de shader/geometria/textura (inalterados)
// ---------------------------------------------------------------------

GLuint compileShader(GLenum type, const GLchar* source, const char* name)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        GLchar infoLog[1024];
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        cerr << "Shader compilation failed (" << name << "): " << infoLog << endl;
    }

    return shader;
}

GLuint createProgram(const GLchar* vertexSource, const GLchar* fragmentSource)
{
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource, "vertex");
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource, "fragment");
    GLuint program = glCreateProgram();

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        GLchar infoLog[1024];
        glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
        cerr << "Shader linking failed: " << infoLog << endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

GLuint setupLineGeometry()
{
    const GLfloat vertices[] =
    {
        -0.75f,  0.60f, 0.0f,  0.75f,  0.60f, 0.0f,
         0.75f,  0.60f, 0.0f,  0.75f, -0.60f, 0.0f,
         0.75f, -0.60f, 0.0f, -0.75f, -0.60f, 0.0f,
        -0.75f, -0.60f, 0.0f, -0.75f,  0.60f, 0.0f
    };

    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    return vao;
}

GLuint setupQuadGeometry()
{
    const GLfloat vertices[] =
    {
        -0.5f,  0.5f, 0.0f,  0.0f, 0.0f,
         0.5f,  0.5f, 0.0f,  1.0f, 0.0f,
         0.5f, -0.5f, 0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, 0.0f,  0.0f, 1.0f
    };

    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                          reinterpret_cast<void*>(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    return vao;
}

// Triangulo apontando para a direita, dentro do espaco -0.5..0.5 (mesmo
// espaco local que rectanglePosition/rectangleSize escalam). Usado pelas
// setas do carrossel de inimigos (ver drawArrow em RenderContext.h) -
// espelhar horizontalmente e so passar rectangleSize.x negativo.
GLuint setupTriangleGeometry()
{
    const GLfloat vertices[] =
    {
        -0.35f, -0.40f, 0.0f,
        -0.35f,  0.40f, 0.0f,
         0.45f,  0.00f, 0.0f
    };

    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    return vao;
}

GLuint loadTexture(const char* filename)
{
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* data = stbi_load(filename, &width, &height, &channels, 4);

    if (!data)
    {
        cerr << "Could not load texture: " << filename << endl;
        return 0;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    return texture;
}

// ---------------------------------------------------------------------
// Projecao + conversao de coordenadas de tela (pixel) para mundo
// ---------------------------------------------------------------------

float g_pixelsPerWorldUnit = 1.0f;

void makeProjection(int framebufferWidth, int framebufferHeight, GLfloat* projection)
{
    g_pixelsPerWorldUnit = min(
        framebufferWidth / (2.0f * VIEW_HALF_WIDTH),
        framebufferHeight / (2.0f * VIEW_HALF_HEIGHT)
    );

    const float xScale = 2.0f * g_pixelsPerWorldUnit / framebufferWidth;
    const float yScale = 2.0f * g_pixelsPerWorldUnit / framebufferHeight;

    fill(projection, projection + 16, 0.0f);
    projection[0] = xScale;
    projection[5] = yScale;
    projection[10] = 1.0f;
    projection[15] = 1.0f;
}

// Converte um clique (coordenadas de tela, origem no canto superior
// esquerdo, como o GLFW reporta) para coordenadas de mundo centradas
// (o mesmo sistema usado por heart.x/y, box.left/right, etc.).
void screenToWorld(GLFWwindow* window, double screenX, double screenY, float& worldX, float& worldY)
{
    int fbWidth = 0, fbHeight = 0;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

    int winWidth = 0, winHeight = 0;
    glfwGetWindowSize(window, &winWidth, &winHeight);

    double scaleX = winWidth > 0 ? (double)fbWidth / winWidth : 1.0;
    double scaleY = winHeight > 0 ? (double)fbHeight / winHeight : 1.0;

    double px = screenX * scaleX;
    double py = screenY * scaleY;

    worldX = (float)((px - fbWidth / 2.0) / g_pixelsPerWorldUnit);
    worldY = (float)((fbHeight / 2.0 - py) / g_pixelsPerWorldUnit); // Y invertido: cursor cresce pra baixo
}

// ---------------------------------------------------------------------
// Estado global do jogo (maquina de estados + entidades)
// ---------------------------------------------------------------------

enum class GameState { MENU, PLAYING, GAME_OVER };
GameState g_state = GameState::MENU;

Box g_box;
Heart g_heart;
vector<AttackDrop> g_attackDrops;
mt19937 g_randomGenerator(170);
uniform_real_distribution<float> g_horizontalSpawn(g_box.left + 0.08f, g_box.right - 0.08f);
uniform_real_distribution<float> g_speedDistribution(0.65f, 0.85f);

float g_elapsedTime = 0.0f;
float g_attackSpawnTimer = 0.0f;
bool g_attackStarted = false;
float g_floodHeight = 0.0f;
vector<Shuriken> g_shurikens;
float g_shurikenTimer = 0.0f;
float g_flashClock = 0.0f;
float g_flashRemaining = 0.0f; // tempo restante do flash (hold + fade)
bool g_floodStarted = false;
float g_health = 1.0f;

// Terremoto (mecanica de boss): relogio ate o proximo e tempo restante do tremor.
float g_quakeClock = 0.0f;
bool g_screenFlipped = false;
float g_quakeRemaining = 0.0f;

double g_playStartTime = 0.0;
double g_survivalTime = 0.0;

// Texturas carregadas uma vez em main() e reaproveitadas pelas telas/jogo.
GLuint g_heartTexture = 0;
vector<EnemyDef> g_enemies; // indice = MenuScreen::enemyIndex()
GLuint g_rainTexture = 0;
GLuint g_dropletTexture = 0;
GLuint g_shurikenTexture = 0;

MenuScreen g_menuScreen;
GameOverScreen g_gameOverScreen;

// Ponteiro para os botoes da tela atualmente ativa (nullptr durante o
// gameplay, ja que o jogo usa apenas teclado).

constexpr float HURT_FLASH_DURATION = 0.2f;

void damageHeart(float amount)
{
    g_health = max(0.0f, g_health - amount);
    g_heart.hurtTimer = HURT_FLASH_DURATION;
}

const vector<Button>* g_activeButtons = nullptr;

float floodSurfaceAt(float x, float elapsedTime)
{
    const float floorY = g_box.bottom + g_box.wallThickness;
    const float waveAmplitude = min(0.02f, g_floodHeight * 0.25f);
    return floorY + g_floodHeight +
           sin(x * 8.0f + elapsedTime * 3.0f) * waveAmplitude;
}

void resetGame()
{
    g_heart = Heart{};
    g_attackDrops.clear();
    g_elapsedTime = 0.0f;
    g_attackSpawnTimer = 0.0f;
    g_attackStarted = false;
    g_floodHeight = 0.0f;
    g_floodStarted = false;
    g_shurikens.clear();
    g_shurikenTimer = 0.0f;
    g_beams.clear();
    g_beamTimer = 0.0f;
    g_flashRemaining = 0.0f;
    g_health = 1.0f;
    g_quakeClock = 0.0f;
    g_quakeRemaining = 0.0f;
    g_screenFlipped = false;
    g_playStartTime = glfwGetTime();
}

void startGame()
{
    resetGame();
    g_state = GameState::PLAYING;
    g_activeButtons = nullptr; // sem botoes durante a partida
}

void goToMenu()
{
    g_menuScreen.rebuildButtons();
    g_state = GameState::MENU;
    g_activeButtons = &g_menuScreen.buttons();
}

void endGame()
{
    g_survivalTime = glfwGetTime() - g_playStartTime;
    g_gameOverScreen.setSurvivalTime(g_survivalTime);
    g_gameOverScreen.rebuildButtons();
    g_state = GameState::GAME_OVER;
    g_activeButtons = &g_gameOverScreen.buttons();
}

// ---------------------------------------------------------------------
// Update e render do gameplay em si (mesma logica de antes, so que
// isolada em funcoes para caber na maquina de estados)
// ---------------------------------------------------------------------

void updateGame(GLFWwindow* window, float deltaTime)
{
    g_elapsedTime += deltaTime;
    g_heart.hurtTimer = max(0.0f, g_heart.hurtTimer - deltaTime);

    // Bosses da linha do Squirtle usam gotas + flood em vez da chuva normal.
    const bool floodAttack = g_enemies[g_menuScreen.enemyIndex()].floodAttack;

    // Indice de dificuldade vindo do menu (0=facil, 1=medio, 2=dificil).
    const int difficultyIndex = std::clamp(g_menuScreen.difficulty(), 0, 2);
    const float effectiveSpawnInterval = DROP_SPAWN_INTERVALS[difficultyIndex];

    // EM ABERTO: a velocidade das gotas continua usando um multiplicador
    // aproximado (nao uma tabela fixa como o intervalo de spawn). Se
    // quiserem o mesmo controle explicito por numero aqui, o padrao seria
    // igual ao de cima: um array RAIN_SPEED_MULTIPLIERS[3] indexado por
    // difficultyIndex, no lugar da formula abaixo.
    const float speedMultiplier = 1.0f + 0.35f * (difficultyIndex - 1);

    // Mecanica especial do boss: terremoto periodico (so afeta a camera).
    const EnemyDef& boss = g_enemies[g_menuScreen.enemyIndex()];
    if (boss.quakeInterval > 0.0f)
    {
        g_quakeClock += deltaTime;
        if (g_quakeClock >= boss.quakeInterval)
        {
            g_quakeClock -= boss.quakeInterval;
            g_quakeRemaining = boss.quakeDuration;
            g_screenFlipped = !g_screenFlipped;   
        }
        g_quakeRemaining = max(0.0f, g_quakeRemaining - deltaTime);
    }

    const float oldX = g_heart.x;
    const float oldY = g_heart.y;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        g_heart.y += g_heart.speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        g_heart.y -= g_heart.speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        g_heart.x -= g_heart.speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        g_heart.x += g_heart.speed * deltaTime;

    const Hitbox topWall{g_box.left, g_box.top - g_box.wallThickness,
                         g_box.right - g_box.left, g_box.wallThickness};
    const Hitbox bottomWall{g_box.left, g_box.bottom,
                            g_box.right - g_box.left, g_box.wallThickness};
    const Hitbox leftWall{g_box.left, g_box.bottom,
                          g_box.wallThickness, g_box.top - g_box.bottom};
    const Hitbox rightWall{g_box.right - g_box.wallThickness, g_box.bottom,
                           g_box.wallThickness, g_box.top - g_box.bottom};

    Hitbox heartHitbox{g_heart.x - g_heart.width / 2.0f,
                       g_heart.y - g_heart.height / 2.0f,
                       g_heart.width, g_heart.height};

    if (checkCollision(heartHitbox, topWall) ||
        checkCollision(heartHitbox, bottomWall) ||
        checkCollision(heartHitbox, leftWall) ||
        checkCollision(heartHitbox, rightWall))
    {
        g_heart.x = oldX;
        g_heart.y = oldY;
        heartHitbox.x = g_heart.x - g_heart.width / 2.0f;
        heartHitbox.y = g_heart.y - g_heart.height / 2.0f;
    }

    if (g_elapsedTime >= ATTACK_DELAY)
    {
        if (!g_attackStarted)
        {
            g_attackStarted = true;
            g_attackSpawnTimer = effectiveSpawnInterval;
        }

        g_attackSpawnTimer += deltaTime;
        while (g_attackSpawnTimer >= effectiveSpawnInterval)
        {
            g_attackSpawnTimer -= effectiveSpawnInterval;
            g_attackDrops.push_back({
                g_horizontalSpawn(g_randomGenerator),
                g_box.top + 0.16f,
                g_speedDistribution(g_randomGenerator) * speedMultiplier
            });
        }
    }

    for (auto drop = g_attackDrops.begin(); drop != g_attackDrops.end();)
    {
        drop->y -= drop->speed * deltaTime;
        drop->age += deltaTime;

        const float dropWidth = floodAttack ? DROPLET_WIDTH : DROP_SIZE;
        const float dropHeight = floodAttack ? DROPLET_HEIGHT : DROP_SIZE;
        const Hitbox dropHitbox{drop->x - dropWidth / 2.0f,
                                drop->y - dropHeight / 2.0f,
                                dropWidth, dropHeight};

        // Normal: a chuva ignora as paredes da caixa e so testa o coracao.
        if (checkCollision(heartHitbox, dropHitbox))
        {
            damageHeart(0.10f);
            drop = g_attackDrops.erase(drop);
        }
        else if (floodAttack &&
                 drop->y - DROPLET_HEIGHT / 2.0f <=
                     (g_floodStarted
                         ? floodSurfaceAt(drop->x, g_elapsedTime)
                         : g_box.bottom + g_box.wallThickness))
        {
            // Gota tocou a agua (ou o chao): o nivel da flood sobe.
            g_floodStarted = true;
            g_floodHeight = min(FLOOD_MAX_HEIGHT[difficultyIndex],
                                g_floodHeight + FLOOD_RISE_PER_DROP[difficultyIndex]);
            drop = g_attackDrops.erase(drop);
        }
        else if (!floodAttack && drop->y < g_box.bottom - DROP_SIZE)
        {
            drop = g_attackDrops.erase(drop);
        }
        else
        {
            ++drop;
        }
    }

        // ---- Raio do Lanturn: o flash bang so ocorre ao acertar o coracao ----
    if (boss.flashAttack && g_elapsedTime >= ATTACK_DELAY)
    {
        g_beamTimer += deltaTime;
        while (g_beamTimer >= BEAM_SPAWN_INTERVALS[difficultyIndex])
        {
            g_beamTimer -= BEAM_SPAWN_INTERVALS[difficultyIndex];

            // Mira na posicao do coracao no momento do disparo.
            float dx = g_heart.x - boss.x;
            float dy = g_heart.y - boss.y;
            float len = sqrt(dx * dx + dy * dy);
            if (len < 0.001f) len = 0.001f;

            g_beams.push_back({ boss.x, boss.y,
                                dx / len * BEAM_SPEED, dy / len * BEAM_SPEED });
        }
    }

    for (auto b = g_beams.begin(); b != g_beams.end();)
    {
        b->x += b->vx * deltaTime;
        b->y += b->vy * deltaTime;

        const Hitbox beamHitbox{ b->x - BEAM_HITBOX / 2.0f, b->y - BEAM_HITBOX / 2.0f,
                                 BEAM_HITBOX, BEAM_HITBOX };

        if (checkCollision(heartHitbox, beamHitbox))
        {
            g_flashRemaining = FLASH_HOLD + FLASH_FADE;   // flash so aqui
            b = g_beams.erase(b);
        }
        else if (fabs(b->x) > VIEW_HALF_WIDTH + 0.2f || fabs(b->y) > VIEW_HALF_HEIGHT + 0.2f)
        {
            b = g_beams.erase(b);   // saiu da tela sem acertar
        }
        else
        {
            ++b;
        }
    }
    g_flashRemaining = max(0.0f, g_flashRemaining - deltaTime);

    // ---- Shuriken (linha do Greninja) ----
    if (g_enemies[g_menuScreen.enemyIndex()].shurikenAttack && g_elapsedTime >= ATTACK_DELAY)
    {
        g_shurikenTimer += deltaTime;
        if (g_shurikenTimer >= SHURIKEN_SPAWN_INTERVAL)
        {
            g_shurikenTimer -= SHURIKEN_SPAWN_INTERVAL;
            uniform_real_distribution<float> sideways(-0.55f, 0.55f);
            g_shurikens.push_back({
                g_horizontalSpawn(g_randomGenerator),
                g_box.top + 0.16f,
                sideways(g_randomGenerator),
                -SHURIKEN_SPEED
            });
        }
    }

    {
        const float half = SHURIKEN_SIZE / 2.0f;
        const float minX = g_box.left + g_box.wallThickness + half;
        const float maxX = g_box.right - g_box.wallThickness - half;
        const float floorY = g_box.bottom + g_box.wallThickness + half;

        for (auto sh = g_shurikens.begin(); sh != g_shurikens.end();)
        {
            sh->x += sh->vx * deltaTime;
            sh->y += sh->vy * deltaTime;
            sh->age += deltaTime;

            // Pinball: reflete nas paredes laterais e no chao (so depois de
            // entrar na caixa; antes disso ela ainda esta caindo de fora).
            if (sh->y < g_box.top - half)
            {
                if (sh->x < minX) { sh->x = minX; sh->vx = fabs(sh->vx); ++sh->bounces; }
                if (sh->x > maxX) { sh->x = maxX; sh->vx = -fabs(sh->vx); ++sh->bounces; }
                if (sh->y < floorY) { sh->y = floorY; sh->vy = fabs(sh->vy); ++sh->bounces; }
            }

            const Hitbox shHitbox{sh->x - SHURIKEN_HITBOX / 2.0f,
                                  sh->y - SHURIKEN_HITBOX / 2.0f,
                                  SHURIKEN_HITBOX, SHURIKEN_HITBOX};

            if (checkCollision(heartHitbox, shHitbox))
            {
                damageHeart(SHURIKEN_DAMAGE[difficultyIndex]);
                sh = g_shurikens.erase(sh);
            }
            else if (sh->bounces > SHURIKEN_MAX_BOUNCES)
            {
                sh = g_shurikens.erase(sh);
            }
            else
            {
                // Gravidade leve para o movimento parecer de bolinha de pinball.
                sh->vy -= 0.6f * deltaTime;
                ++sh;
            }
        }
    }

    // Coracao submerso na flood perde vida continuamente.
    if (floodAttack && g_floodStarted &&
        g_heart.y - g_heart.height / 2.0f < floodSurfaceAt(g_heart.x, g_elapsedTime))
    {
        g_health = max(0.0f, g_health - FLOOD_DAMAGE_PER_SECOND[difficultyIndex] * deltaTime);
    }

    if (g_health <= 0.0f)
    {
        endGame();
    }
}

void renderGame(const RenderContext& ctx)
{
    const bool floodAttack = g_enemies[g_menuScreen.enemyIndex()].floodAttack;

    // Health bar: the filled portion stays anchored to the left edge.
    drawRect(ctx, 0.0f, g_box.bottom - 0.12f, g_box.right - g_box.left, 0.07f,
             0.25f, 0.04f, 0.04f);
    if (g_health > 0.0f)
    {
        const float barWidth = (g_box.right - g_box.left) * g_health;
        drawRect(ctx, g_box.left + barWidth / 2.0f, g_box.bottom - 0.12f,
                 barWidth, 0.07f, 0.10f, 0.85f, 0.18f);
    }

    drawBoxOutline(ctx);

    // Inimigo escolhido no menu, com a animacao propria dele (grade,
    // frames validos e velocidade definidos no EnemyDef).
    const EnemyDef& enemy = g_enemies[g_menuScreen.enemyIndex()];
    drawEnemy(ctx, enemy, g_elapsedTime, enemy.x, enemy.y, enemy.width, enemy.height);

    if (g_floodStarted)
    {
        const float interiorLeft = g_box.left + g_box.wallThickness;
        const float interiorRight = g_box.right - g_box.wallThickness;
        const float interiorBottom = g_box.bottom + g_box.wallThickness;
        constexpr int waveSegments = 48;
        const float segmentWidth = (interiorRight - interiorLeft) / waveSegments;

        for (int segment = 0; segment < waveSegments; ++segment)
        {
            const float x = interiorLeft + (segment + 0.5f) * segmentWidth;
            const float surfaceY = max(interiorBottom, floodSurfaceAt(x, g_elapsedTime));
            const float waterHeight = surfaceY - interiorBottom;
            if (waterHeight > 0.0f)
            {
                drawRect(ctx, x, interiorBottom + waterHeight / 2.0f,
                         segmentWidth + 0.002f, waterHeight,
                         0.08f, 0.40f, 0.82f);
            }
        }
    }

    // Gotas/chuva desenhadas depois da caixa e da flood para ficarem visiveis.
    for (const AttackDrop& drop : g_attackDrops)
    {
        if (floodAttack)
        {
            drawSpriteCtx(ctx, g_dropletTexture, drop.x, drop.y,
                          DROPLET_TEXTURE_SIZE, DROPLET_TEXTURE_SIZE,
                          1, 1, 0, 1.0f, 1.0f, false);
        }
        else
        {
            const int rainFrame = static_cast<int>(drop.age / 0.08f) % 8;
            drawSpriteCtx(ctx, g_rainTexture, drop.x, drop.y, DROP_SIZE, DROP_SIZE,
                         4, 2, rainFrame, 1.0f, 1.0f, false);
        }
    }
    for (const LanturnBeam& b : g_beams)
    {
        drawRect(ctx, b.x, b.y, BEAM_SIZE * 1.8f, BEAM_SIZE * 1.8f, 0.2f, 0.6f, 1.0f, 0.35f); // halo
        drawRect(ctx, b.x, b.y, BEAM_SIZE, BEAM_SIZE, 1.0f, 1.0f, 0.6f);                      // nucleo
    }   
    for (const Shuriken& sh : g_shurikens)
    {
        drawSpriteUv(ctx, g_shurikenTexture, sh.x, sh.y, SHURIKEN_SIZE, SHURIKEN_SIZE,
                     0.0f, 0.0f, 1.0f, 1.0f, false,
                     199.0f / 255.0f, 225.0f / 255.0f, 209.0f / 255.0f,
                     sh.age * 12.0f); // gira enquanto quica
    }

const float t = g_heart.hurtTimer / HURT_FLASH_DURATION;   // 1 -> 0 em 0,2 s
drawSpriteUv(ctx, g_heartTexture, g_heart.x, g_heart.y,
             g_heart.width, g_heart.height,
             0.0f, 0.0f, 1.0f, 1.0f, false,
             199.0f / 255.0f, 225.0f / 255.0f, 209.0f / 255.0f,
             0.0f,                 // angle
             1.0f - 0.5f * t,      // alpha: cai até 50% e volta
             t);                   // whiten: branco total e volta ao normal

    // Flash bang por cima de tudo: branco total durante FLASH_HOLD e depois
    // some gradualmente durante FLASH_FADE.
    if (g_flashRemaining > 0.0f)
    {
        const float alpha = min(1.0f, g_flashRemaining / FLASH_FADE);
        drawRect(ctx, 0.0f, 0.0f, 2.4f, 1.8f, 1.0f, 1.0f, 1.0f, alpha);
    }
}

// ---------------------------------------------------------------------
// Callbacks de input
// ---------------------------------------------------------------------

void key_callback(GLFWwindow* window, int key, int, int action, int)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS)
        return;

    if (g_activeButtons == nullptr)
        return;

    double sx = 0.0, sy = 0.0;
    glfwGetCursorPos(window, &sx, &sy);

    float wx = 0.0f, wy = 0.0f;
    screenToWorld(window, sx, sy, wx, wy);

    for (const Button& b : *g_activeButtons)
    {
        if (b.contains(wx, wy))
        {
            b.onClick();
            break;
        }
    }
}

// ---------------------------------------------------------------------
// main
// ---------------------------------------------------------------------

int main()
{
    if (!glfwInit())
    {
        cerr << "Could not initialize GLFW" << endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "OpenGL - PokéRune", nullptr, nullptr);
    if (!window)
    {
        cerr << "Could not create GLFW window" << endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        cerr << "Could not initialize GLAD" << endl;
        glfwTerminate();
        return -1;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLuint colorShader = createProgram(colorVertexShaderSource, colorFragmentShaderSource);
    GLuint spriteShader = createProgram(spriteVertexShaderSource, spriteFragmentShaderSource);
    GLuint lineVao = setupLineGeometry();
    GLuint quadVao = setupQuadGeometry();
    GLuint triangleVao = setupTriangleGeometry();

    g_heartTexture = loadTexture("heart.png");
    g_rainTexture = loadTexture("rain.png");
    g_dropletTexture = loadTexture("droplet.png");
    g_shurikenTexture = loadTexture("shuriken.png");

    // ---- Inimigos (um EnemyDef por boss) ----
    // Todos os PNGs usam fundo 199,225,209 (chroma key padrao do EnemyDef).

    // Chinchou: grade 4x4 na area util de 240x183 px (403x183 no total);
    // so 12 das 16 celulas tem pose.
    EnemyDef chinchou;
    chinchou.name = "CHINCHOU";
    chinchou.texture = loadTexture("chinchou.png");
    chinchou.textureWidth = 403.0f; chinchou.textureHeight = 183.0f;
    chinchou.frames = gridFrames(4, 4, 240.0f, 183.0f, { 0, 1, 2, 4, 8, 9, 10, 11, 12, 13, 14, 15 });
    chinchou.width = 0.30f; chinchou.height = 0.36f;
    chinchou.flashAttack = true; // linha do Chinchou: flash bang
    g_enemies.push_back(chinchou);

    // Squirtle: animacao idle = linha do topo (6 frames de 45x42 px, y=0)
    // na sheet 713x293. O resto da imagem (corpo recortado, shiny, etc.)
    // nao e usado.
    EnemyDef squirtle;
    squirtle.name = "SQUIRTLE";
    squirtle.texture = loadTexture("squirtle.png");
    squirtle.textureWidth = 713.0f; squirtle.textureHeight = 293.0f;
    squirtle.frames = {
        {   6, 0, 45, 42 }, {  51, 0, 45, 42 }, {  95, 0, 45, 42 },
        { 138, 0, 45, 42 }, { 184, 0, 45, 42 }, { 230, 0, 45, 42 }
    };
    squirtle.frameDuration = 0.15f;
    squirtle.floodAttack = true; // linha do Squirtle: gotas + flood
    squirtle.width = 0.365f; squirtle.height = 0.34f; // proporcao 45:42
    g_enemies.push_back(squirtle);

    // Froakie: animacao idle = linha do topo (4 frames de 36x44 px, y=0)
    // na sheet 511x234.
    EnemyDef froakie;
    froakie.name = "FROAKIE";
    froakie.texture = loadTexture("froakie.png");
    froakie.textureWidth = 511.0f; froakie.textureHeight = 234.0f;
    froakie.frames = {
        {   0, 0, 36, 44 }, {  36, 0, 36, 44 }, {  72, 0, 36, 44 }, { 108, 0, 36, 44 }
    };
    froakie.frameDuration = 0.18f;
    froakie.shurikenAttack = true; // linha do Greninja: shuriken
    froakie.width = 0.295f; froakie.height = 0.36f; // proporcao 36:44
    g_enemies.push_back(froakie);

    // Mudkip: primeira linha do sheet (6 frames, ataque Water Gun no ultimo).
    // mudkip.png ja e uma versao LIMPA do sheet (so essa linha, fundo
    // transparente), entao nao usa chroma key. O sprite olha para a direita;
    // como o boss fica a direita da arena, espelhamos (flipX).
    EnemyDef mudkip;
    mudkip.name = "MUDKIP";
    mudkip.texture = loadTexture("mudkip.png");
    mudkip.textureWidth = 221.0f; mudkip.textureHeight = 41.0f;
    mudkip.frames = {
        {   0, 4, 29, 34 }, {  32, 4, 29, 34 }, {  65, 4, 29, 34 },
        {  94, 4, 30, 34 }, { 125, 4, 40, 34 }, { 166, 4, 55, 34 }
    };
    mudkip.frameDuration = 0.15f;
    mudkip.useChromaKey = false;
    mudkip.flipX = true;
    mudkip.width = 0.29f; mudkip.height = 0.34f; // proporcao 29:34 (frame 0)
    mudkip.quakeInterval = 5.0f;   
    mudkip.quakeFlipX = true;
    mudkip.quakeDuration = 0.6f;
    mudkip.quakeAmplitude = 0.03f;
    g_enemies.push_back(mudkip);

    // ---- Bosses de evolucao 2 e 3 ----
    // Cada PNG abaixo e uma tira LIMPA gerada a partir do sheet original
    // (fundo transparente, 1 linha, frames de tamanho uniforme), entao nao
    // usa chroma key. Todos ja olham para a esquerda (nao precisam de flipX).
    // addStrip(nome, arquivo, frames, larguraCelula, alturaCelula, seg/frame, alturaNoMundo)
    auto addStrip = [&](const char* name, const char* file, int n, float cw, float ch,
                        float frameDuration, float worldHeight) -> EnemyDef&
    {
        EnemyDef e;
        e.name = name;
        e.texture = loadTexture(file);
        e.textureWidth = n * cw; e.textureHeight = ch;
        e.frames = gridFrames(n, 1, e.textureWidth, e.textureHeight, {});
        e.frameDuration = frameDuration;
        e.useChromaKey = false;
        setSizeByHeight(e, worldHeight);
        e.x = std::min(0.94f, 1.18f - e.width / 2.0f); // nao sair da tela (borda em 1.20)
        g_enemies.push_back(e);
        return g_enemies.back();
    };

    // Linha do Mudkip: o terremoto fica mais frequente e mais forte a cada
    // evolucao (Mudkip 5s/0.03 -> Marshtomp 4s/0.04 -> Swampert 3s/0.05).
    // EM ABERTO: valores de partida, ajustar por playtest.
    EnemyDef& marshtomp = addStrip("MARSHTOMP", "marshtomp.png", 5, 54, 57, 0.18f, 0.36f); // idx 4
    marshtomp.quakeInterval = 4.0f;
    marshtomp.quakeFlipX = true;
    marshtomp.quakeDuration = 0.7f;
    marshtomp.quakeAmplitude = 0.04f;
    addStrip("WARTORTLE", "wartortle.png", 7, 72, 61, 0.14f, 0.38f).floodAttack = true; // idx 5 (linha do Squirtle)
    // Frogadier: so 2 poses (agachado/em pe) tiradas do sheet da linha do Froakie.
    addStrip("FROGADIER", "frogadier.png", 2, 75, 83, 0.45f, 0.38f).shurikenAttack = true; // idx 6 (linha do Greninja)
    EnemyDef& swampert = addStrip("SWAMPERT", "swampert.png", 13, 76, 60, 0.09f, 0.36f); // idx 7
    swampert.quakeInterval = 3.0f;
    swampert.quakeFlipY = true;
    swampert.quakeDuration = 0.8f;
    swampert.quakeAmplitude = 0.05f;
    addStrip("BLASTOISE", "blastoise.png", 10, 56, 54, 0.12f, 0.40f).floodAttack = true; // idx 8 (linha do Squirtle)
    // Greninja: 1 frame so; o balanco vertical da vida.
    EnemyDef& greninja = addStrip("GRENINJA", "greninja.png", 1, 112, 75, 1.0f, 0.30f); // idx 9
    greninja.bobAmplitude = 0.012f;
    greninja.shurikenAttack = true;
    addStrip("LANTURN", "lanturn.png", 17, 74, 52, 0.10f, 0.32f).flashAttack = true; // idx 10 (linha do Chinchou)

    bool enemiesOk = true;
    for (const EnemyDef& e : g_enemies) enemiesOk = enemiesOk && e.texture != 0;

    if (g_heartTexture == 0 || !enemiesOk || g_rainTexture == 0 || g_dropletTexture == 0 ||
        g_shurikenTexture == 0)
    {
        cerr << "Make sure heart.png, rain.png, droplet.png, shuriken.png, chinchou.png, squirtle.png, froakie.png, mudkip.png and the 7 new boss PNGs are beside the executable." << endl;
        glDeleteVertexArrays(1, &lineVao);
        glDeleteVertexArrays(1, &quadVao);
        glDeleteProgram(colorShader);
        glDeleteProgram(spriteShader);
        glfwTerminate();
        return -1;
    }

    // Fonte bitmap (atlas 6x7: A-Z, 0-9, "!", ".", espaco, "?") usada pelo
    // titulo do menu, tempo de sobrevivencia e labels dos botoes.
    // Precisa estar ao lado do executavel, igual heart.png/chinchou.png/rain.png.
    GLuint fontTexture = loadTexture("font.png");

    const GLint colorProjectionLoc = glGetUniformLocation(colorShader, "projection");
    const GLint useRectangleLoc = glGetUniformLocation(colorShader, "useRectangle");
    const GLint rectanglePositionLoc = glGetUniformLocation(colorShader, "rectanglePosition");
    const GLint rectangleSizeLoc = glGetUniformLocation(colorShader, "rectangleSize");
    const GLint colorLoc = glGetUniformLocation(colorShader, "inputColor");

    const GLint spriteProjectionLoc = glGetUniformLocation(spriteShader, "projection");
    const GLint spritePositionLoc = glGetUniformLocation(spriteShader, "spritePosition");
    const GLint spriteSizeLoc = glGetUniformLocation(spriteShader, "spriteSize");
    const GLint spriteUvLoc = glGetUniformLocation(spriteShader, "spriteUv");
    const GLint spriteAngleLoc = glGetUniformLocation(spriteShader, "spriteAngle");
    const GLint textureLoc = glGetUniformLocation(spriteShader, "texture1");
    const GLint chromaKeyLoc = glGetUniformLocation(spriteShader, "useChromaKey");
    const GLint chromaKeyColorLoc = glGetUniformLocation(spriteShader, "chromaKeyColor");

    RenderContext ctx;
    ctx.colorShader = colorShader;
    ctx.spriteShader = spriteShader;
    ctx.quadVao = quadVao;
    ctx.lineVao = lineVao;
    ctx.triangleVao = triangleVao;
    ctx.colorProjectionLoc = colorProjectionLoc;
    ctx.useRectangleLoc = useRectangleLoc;
    ctx.rectanglePositionLoc = rectanglePositionLoc;
    ctx.rectangleSizeLoc = rectangleSizeLoc;
    ctx.colorLoc = colorLoc;
    ctx.spriteProjectionLoc = spriteProjectionLoc;
    ctx.spritePositionLoc = spritePositionLoc;
    ctx.spriteSizeLoc = spriteSizeLoc;
    ctx.spriteUvLoc = spriteUvLoc;
    ctx.spriteAngleLoc = spriteAngleLoc;
    ctx.textureLoc = textureLoc;
    ctx.chromaKeyLoc = chromaKeyLoc;
    ctx.chromaKeyColorLoc = chromaKeyColorLoc;
    ctx.spriteAlphaLoc  = glGetUniformLocation(spriteShader, "spriteAlpha");
    ctx.spriteWhitenLoc = glGetUniformLocation(spriteShader, "spriteWhiten");
    // ---- Configura as telas ----

    MenuScreen::Assets menuAssets;
    menuAssets.fontTexture = fontTexture;
    menuAssets.enemies = g_enemies;

    // Indices em g_enemies: 0=chinchou, 1=squirtle, 2=froakie, 3=mudkip.
    // FACIL: evolucao 1 de cada linha.
    menuAssets.enemiesByDifficulty[0] = { 3, 1, 0, 2 }; // Mudkip, Squirtle, Chinchou, Froakie
    // MEDIO: Marshtomp, Wartortle, Chinchou (repete: so tem 2 estagios), Frogadier.
    menuAssets.enemiesByDifficulty[1] = { 4, 5, 0, 6 };
    // DIFICIL: Swampert, Blastoise, Lanturn, Greninja.
    menuAssets.enemiesByDifficulty[2] = { 7, 8, 10, 9 };
    g_menuScreen.setup(menuAssets, []() { startGame(); });

    GameOverScreen::Assets gameOverAssets;
    gameOverAssets.fontTexture = fontTexture;
    g_gameOverScreen.setup(gameOverAssets,
        []() { startGame(); },
        []() { goToMenu(); });

    goToMenu(); // estado inicial: MENU, com os botoes do menu ativos

    double previousTime = glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        const double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - previousTime);
        previousTime = currentTime;
        deltaTime = min(deltaTime, 0.05f);

        glfwPollEvents();

        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
        if (framebufferWidth == 0 || framebufferHeight == 0)
            continue;

        GLfloat projection[16];
        makeProjection(framebufferWidth, framebufferHeight, projection);
        ctx.projection = projection;

        glViewport(0, 0, framebufferWidth, framebufferHeight);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        switch (g_state)
        {
            case GameState::MENU:
                g_menuScreen.render(ctx);
                break;

            case GameState::PLAYING:
                updateGame(window, deltaTime);
                {
                    const EnemyDef& boss = g_enemies[g_menuScreen.enemyIndex()];

                    if (g_screenFlipped)
                    {
                        if (boss.quakeFlipX) projection[0] = -projection[0];
                        if (boss.quakeFlipY) projection[5] = -projection[5];
                    }
                    // Tremor de tela: desloca a projecao (translacao em
                    // clip space = offset em mundo * escala da projecao).
                    if (g_quakeRemaining > 0.0f && boss.quakeDuration > 0.0f)
                    {
                        const float t = static_cast<float>(glfwGetTime());
                        const float strength = boss.quakeAmplitude * (g_quakeRemaining / boss.quakeDuration);
                        projection[12] = strength * sin(t * 70.0f) * projection[0];
                        projection[13] = strength * cos(t * 83.0f) * projection[5];
                    }
                    renderGame(ctx);
                }
                break;

            case GameState::GAME_OVER:
                g_gameOverScreen.render(ctx);
                break;
        }

        glfwSwapBuffers(window);
    }

    glDeleteTextures(1, &g_heartTexture);
    for (const EnemyDef& e : g_enemies) glDeleteTextures(1, &e.texture);
    glDeleteTextures(1, &g_rainTexture);
    glDeleteTextures(1, &g_dropletTexture);
    glDeleteTextures(1, &g_shurikenTexture);
    if (fontTexture != 0) glDeleteTextures(1, &fontTexture);
    glDeleteVertexArrays(1, &lineVao);
    glDeleteVertexArrays(1, &quadVao);
    glDeleteVertexArrays(1, &triangleVao);
    glDeleteProgram(colorShader);
    glDeleteProgram(spriteShader);
    glfwTerminate();
    return 0;
}
