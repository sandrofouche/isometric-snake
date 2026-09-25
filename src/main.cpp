#include "game.hpp"
#include "raylib.h"
#include "rlgl.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <numbers>
#include <random>
#include <string_view>
#include <vector>

namespace {
constexpr int screenWidth = 1100;
constexpr int screenHeight = 760;

struct Bullet {
    Vector3 position;
    Vector3 direction;
    float life;
};

struct Fly {
    Vector3 position;
    float wingPhase;
};

struct Coin {
    Vector3 position;
    float age;
};

enum class Screen { Title, Introduction, Playing };
enum class WavePhase { Survival, Grace, Shop };

Sound makeTone(float startFrequency, float endFrequency, float duration,
               float volume = 0.35F) {
    constexpr unsigned int sampleRate = 44100;
    const unsigned int frameCount = static_cast<unsigned int>(sampleRate * duration);
    std::vector<std::int16_t> samples(frameCount);
    float phase = 0.0F;
    for (unsigned int i = 0; i < frameCount; ++i) {
        const float progress = static_cast<float>(i) / static_cast<float>(frameCount);
        const float frequency = startFrequency + (endFrequency - startFrequency) * progress;
        phase += 2.0F * std::numbers::pi_v<float> * frequency / sampleRate;
        const float envelope = (1.0F - progress) * (1.0F - progress);
        samples[i] = static_cast<std::int16_t>(std::sin(phase) * envelope * volume * 32767.0F);
    }
    Wave wave{};
    wave.frameCount = frameCount;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = samples.data();
    return LoadSoundFromWave(wave);
}

Vector3 worldPosition(Cell cell, int boardSize, float height = 0.0F) {
    const float offset = (static_cast<float>(boardSize) - 1.0F) * 0.5F;
    return {static_cast<float>(cell.x) - offset, height,
            static_cast<float>(cell.y) - offset};
}

Vector3 directionVector(Direction direction) {
    switch (direction) {
        case Direction::North: return {0.0F, 0.0F, -1.0F};
        case Direction::East:  return {1.0F, 0.0F, 0.0F};
        case Direction::South: return {0.0F, 0.0F, 1.0F};
        case Direction::West:  return {-1.0F, 0.0F, 0.0F};
    }
    return {};
}

void drawBoard(const SnakeGame& game) {
    for (int y = 0; y < game.boardSize(); ++y) {
        for (int x = 0; x < game.boardSize(); ++x) {
            const Color tile = (x + y) % 2 == 0
                ? Color{45, 67, 81, 255} : Color{52, 77, 91, 255};
            const Vector3 p = worldPosition({x, y}, game.boardSize(), -0.18F);
            DrawCube(p, 0.96F, 0.30F, 0.96F, tile);
            DrawCubeWires(p, 0.96F, 0.30F, 0.96F, Color{25, 40, 49, 255});
        }
    }
}

void drawBlessing(Cell cell, int boardSize, float time) {
    const float bob = std::sin(time * 3.0F) * 0.08F;
    const Vector3 base = worldPosition(cell, boardSize, 0.52F + bob);
    DrawSphereEx(base, 0.34F, 16, 24, Color{255, 210, 72, 255});
    DrawSphereEx(base, 0.23F, 12, 18, Color{255, 246, 176, 255});
    for (int i = 0; i < 3; ++i) {
        const float angle = time * 2.0F + static_cast<float>(i) * 2.094F;
        DrawSphereEx({base.x + std::cos(angle) * 0.48F, base.y + 0.06F,
                      base.z + std::sin(angle) * 0.48F},
                     0.065F, 6, 8, Color{255, 228, 110, 255});
    }
}

void drawCupcake(Cell cell, int boardSize) {
    const Vector3 p = worldPosition(cell, boardSize, 0.12F);
    DrawCylinder({p.x, p.y + 0.23F, p.z}, 0.31F, 0.40F, 0.45F, 12,
                 Color{126, 67, 50, 255});
    DrawSphereEx({p.x, p.y + 0.58F, p.z}, 0.39F, 12, 18,
                 Color{196, 77, 133, 255});
    DrawSphereEx({p.x, p.y + 0.86F, p.z}, 0.20F, 10, 14,
                 Color{230, 104, 161, 255});
    DrawSphereEx({p.x, p.y + 1.04F, p.z}, 0.095F, 8, 10,
                 Color{190, 32, 52, 255});
    DrawSphereEx({p.x - 0.15F, p.y + 0.64F, p.z - 0.29F}, 0.065F, 6, 8,
                 Color{34, 14, 27, 255});
    DrawSphereEx({p.x + 0.15F, p.y + 0.64F, p.z - 0.29F}, 0.065F, 6, 8,
                 Color{34, 14, 27, 255});
}

Fly spawnFly(int boardSize, std::mt19937& random) {
    const float edge = static_cast<float>(boardSize) * 0.5F - 0.35F;
    std::uniform_real_distribution<float> along(-edge, edge);
    std::uniform_int_distribution<int> side(0, 3);
    Vector3 position{along(random), 0.72F, along(random)};
    switch (side(random)) {
        case 0: position.x = -edge; break;
        case 1: position.x = edge; break;
        case 2: position.z = -edge; break;
        default: position.z = edge; break;
    }
    return {position, along(random)};
}

void drawFly(const Fly& fly, float time) {
    const float wing = std::sin(time * 18.0F + fly.wingPhase) * 0.12F;
    DrawSphereEx(fly.position, 0.23F, 8, 12, Color{35, 30, 42, 255});
    DrawSphereEx({fly.position.x, fly.position.y, fly.position.z + 0.24F},
                 0.17F, 8, 10, Color{60, 52, 67, 255});
    DrawSphereEx({fly.position.x - 0.23F, fly.position.y + 0.10F + wing,
                  fly.position.z}, 0.18F, 6, 8, Fade(SKYBLUE, 0.72F));
    DrawSphereEx({fly.position.x + 0.23F, fly.position.y + 0.10F - wing,
                  fly.position.z}, 0.18F, 6, 8, Fade(SKYBLUE, 0.72F));
    DrawSphereEx({fly.position.x - 0.09F, fly.position.y + 0.07F,
                  fly.position.z - 0.19F}, 0.055F, 6, 8, RED);
    DrawSphereEx({fly.position.x + 0.09F, fly.position.y + 0.07F,
                  fly.position.z - 0.19F}, 0.055F, 6, 8, RED);
}

void drawCoin(const Coin& coin, float time) {
    const float bob = std::sin(time * 4.0F + coin.age) * 0.08F;
    const Vector3 p = {coin.position.x, coin.position.y + bob, coin.position.z};
    DrawCylinder(p, 0.23F, 0.23F, 0.09F, 16, Color{244, 187, 53, 255});
    DrawCylinderWires(p, 0.23F, 0.23F, 0.09F, 16, Color{255, 235, 133, 255});
}

bool occupiedBySnake(const SnakeGame& game, Cell cell) {
    return std::find(game.snake().begin(), game.snake().end(), cell) != game.snake().end();
}

Cell randomOpenCell(const SnakeGame& game, const std::vector<Cell>& cupcakes,
                    std::mt19937& random) {
    std::uniform_int_distribution<int> coordinate(0, game.boardSize() - 1);
    for (;;) {
        const Cell candidate{coordinate(random), coordinate(random)};
        if (candidate != game.food() && !occupiedBySnake(game, candidate) &&
            std::find(cupcakes.begin(), cupcakes.end(), candidate) == cupcakes.end())
            return candidate;
    }
}

void refillCupcakes(const SnakeGame& game, std::vector<Cell>& cupcakes,
                    std::mt19937& random) {
    while (cupcakes.size() < 3) cupcakes.push_back(randomOpenCell(game, cupcakes, random));
}

void drawGun(Vector3 head, Direction heading, float muzzleFlash) {
    const Vector3 forward = directionVector(heading);
    const Vector3 side = {forward.z, 0.0F, -forward.x};
    const Vector3 mount = {head.x, head.y + 0.28F, head.z};
    DrawCubeV(mount, {0.42F, 0.18F, 0.42F}, Color{57, 68, 78, 255});

    for (float sign : {-1.0F, 1.0F}) {
        const Vector3 barrelStart = {
            mount.x + side.x * sign * 0.15F + forward.x * 0.08F,
            mount.y + 0.02F,
            mount.z + side.z * sign * 0.15F + forward.z * 0.08F
        };
        const Vector3 barrelEnd = {
            barrelStart.x + forward.x * 0.52F,
            barrelStart.y,
            barrelStart.z + forward.z * 0.52F
        };
        DrawCylinderEx(barrelStart, barrelEnd, 0.065F, 0.045F, 8,
                       Color{92, 111, 124, 255});
        if (muzzleFlash > 0.0F) {
            const Vector3 flash = {barrelEnd.x + forward.x * 0.10F, barrelEnd.y,
                                   barrelEnd.z + forward.z * 0.10F};
            DrawSphereEx(flash, 0.11F + muzzleFlash * 0.8F, 6, 8,
                         Color{255, 211, 76, 255});
        }
    }
}

void drawSnake(const SnakeGame& game, float muzzleFlash) {
    const auto& snake = game.snake();
    for (std::size_t i = 1; i < snake.size(); ++i) {
        const Vector3 from = worldPosition(snake[i - 1], game.boardSize(), 0.46F);
        const Vector3 to = worldPosition(snake[i], game.boardSize(), 0.46F);
        // Only join adjacent pieces. A sharp turn still looks continuous while
        // avoiding a diagonal bridge across the inside of the corner.
        const int distance = std::abs(snake[i - 1].x - snake[i].x) +
                             std::abs(snake[i - 1].y - snake[i].y);
        if (distance == 1)
            DrawCylinderEx(from, to, 0.28F, 0.28F, 12, Color{45, 164, 96, 255});
    }
    for (std::size_t i = snake.size(); i-- > 0;) {
        const bool head = i == 0;
        const float radius = head ? 0.43F
                                  : std::max(0.26F, 0.36F - static_cast<float>(i) * 0.006F);
        const Vector3 p = worldPosition(snake[i], game.boardSize(), radius + 0.12F);
        const Color body = head ? Color{116, 232, 132, 255}
                                : Color{54, 190, 113, 255};
        DrawSphereEx(p, radius, 12, 20, body);
        DrawSphereEx({p.x - 0.10F, p.y + 0.13F, p.z - 0.10F}, radius * 0.42F,
                     8, 12, Fade(RAYWHITE, 0.12F));

        if (head) {
            const Vector3 forward = directionVector(game.direction());
            const Vector3 side = {forward.z, 0.0F, -forward.x};
            for (float sign : {-1.0F, 1.0F}) {
                const Vector3 eye = {
                    p.x + forward.x * 0.34F + side.x * sign * 0.18F,
                    p.y,
                    p.z + forward.z * 0.34F + side.z * sign * 0.18F
                };
                const Vector3 raisedEye = {eye.x, eye.y + 0.17F, eye.z};
                DrawSphereEx(raisedEye, 0.105F, 8, 12, RAYWHITE);
                DrawSphereEx({raisedEye.x + forward.x * 0.078F, raisedEye.y,
                              raisedEye.z + forward.z * 0.078F},
                             0.052F, 8, 10, Color{20, 27, 31, 255});
            }
            drawGun(p, game.direction(), muzzleFlash);
        }
    }
}

void drawBullets(const std::vector<Bullet>& bullets) {
    for (const Bullet& bullet : bullets) {
        const Vector3 tail = {bullet.position.x - bullet.direction.x * 0.28F,
                              bullet.position.y,
                              bullet.position.z - bullet.direction.z * 0.28F};
        DrawCylinderEx(tail, bullet.position, 0.055F, 0.035F, 6,
                       Color{255, 184, 58, 255});
        DrawSphereEx(bullet.position, 0.09F, 6, 8, Color{255, 238, 126, 255});
    }
}

void centeredText(const char* text, int y, int size, Color color) {
    DrawText(text, (screenWidth - MeasureText(text, size)) / 2, y, size, color);
}

Direction turnLeft(Direction direction) {
    switch (direction) {
        case Direction::North: return Direction::West;
        case Direction::West:  return Direction::South;
        case Direction::South: return Direction::East;
        case Direction::East:  return Direction::North;
    }
    return direction;
}

Direction turnRight(Direction direction) {
    switch (direction) {
        case Direction::North: return Direction::East;
        case Direction::East:  return Direction::South;
        case Direction::South: return Direction::West;
        case Direction::West:  return Direction::North;
    }
    return direction;
}

// Steering is relative to the snake instead of the board or camera. Forward
// needs no input because the snake moves continuously; reverse remains illegal.
void readMovementInput(SnakeGame& game) {
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        game.requestDirection(turnLeft(game.direction()));
    } else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        game.requestDirection(turnRight(game.direction()));
    }
}
} // namespace

int main(int argc, char** argv) {
    const bool captureShop = argc == 3 && std::string_view(argv[1]) == "--screenshot-shop";
    const bool captureFrame = argc == 3 &&
        (std::string_view(argv[1]) == "--screenshot" || captureShop);
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, "Isometric Snake");
    InitAudioDevice();
    SetTargetFPS(60);

    SnakeGame game(18);
    std::vector<Bullet> bullets;
    std::vector<Cell> cupcakes;
    std::vector<Fly> flies;
    std::vector<Coin> droppedCoins;
    std::mt19937 random(std::random_device{}());
    refillCupcakes(game, cupcakes, random);
    Screen screen = captureFrame ? Screen::Playing : Screen::Title;
    float fireCooldown = 0.0F;
    float muzzleFlash = 0.0F;
    float damageCooldown = 0.0F;
    float flySpawnTimer = 1.2F;
    int stage = 1;
    WavePhase wavePhase = WavePhase::Survival;
    float phaseTime = 30.0F;
    int fireRateLevel = 0;
    bool nextWaveInvincibility = false;
    float invincibilityTime = 0.0F;
    if (captureShop) {
        wavePhase = WavePhase::Shop;
        for (int i = 0; i < 14; ++i) game.collectCoin();
    }
    const Sound fireSound = makeTone(760.0F, 260.0F, 0.09F, 0.24F);
    const Sound hitSound = makeTone(180.0F, 520.0F, 0.14F, 0.32F);
    const Sound eatSound = makeTone(420.0F, 880.0F, 0.18F, 0.28F);
    const Sound crashSound = makeTone(160.0F, 55.0F, 0.38F, 0.38F);
    const Sound cupcakeSound = makeTone(240.0F, 95.0F, 0.22F, 0.34F);
    const Sound introSound = makeTone(120.0F, 420.0F, 1.15F, 0.18F);
    const Sound flySound = makeTone(520.0F, 110.0F, 0.12F, 0.28F);
    const Sound damageSound = makeTone(95.0F, 45.0F, 0.24F, 0.38F);
    const Sound coinSound = makeTone(720.0F, 1080.0F, 0.12F, 0.22F);
    const Sound phaseSound = makeTone(300.0F, 680.0F, 0.32F, 0.22F);
    Camera3D camera{};
    camera.position = {13.5F, 16.0F, 13.5F};
    camera.target = {0.0F, 0.0F, 0.0F};
    camera.up = {0.0F, 1.0F, 0.0F};
    camera.fovy = 24.5F;
    camera.projection = CAMERA_ORTHOGRAPHIC;

    double lastStep = GetTime();
    bool done = false;
    int captureDelay = captureFrame ? 90 : 0;
    while (!WindowShouldClose() && !done) {
        const float delta = GetFrameTime();
        if (screen == Screen::Title && IsKeyPressed(KEY_ENTER)) {
            screen = Screen::Introduction;
            PlaySound(introSound);
        } else if (screen == Screen::Introduction &&
                   (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))) {
            screen = Screen::Playing;
            lastStep = GetTime();
        }

        if (screen == Screen::Playing) {
            readMovementInput(game);
            if (IsKeyPressed(KEY_P) && wavePhase != WavePhase::Shop) game.togglePause();
            if (IsKeyPressed(KEY_R)) {
                game.reset();
                bullets.clear();
                cupcakes.clear();
                flies.clear();
                droppedCoins.clear();
                refillCupcakes(game, cupcakes, random);
                stage = 1;
                wavePhase = WavePhase::Survival;
                phaseTime = 30.0F;
                flySpawnTimer = 1.2F;
                fireRateLevel = 0;
                nextWaveInvincibility = false;
                invincibilityTime = 0.0F;
                lastStep = GetTime();
            }

            if (wavePhase == WavePhase::Shop) {
                if (IsKeyPressed(KEY_ONE) && game.spendCoins(5)) {
                    game.addTailSegment();
                    PlaySound(coinSound);
                }
                if (IsKeyPressed(KEY_TWO) && fireRateLevel < 3 && game.spendCoins(6)) {
                    ++fireRateLevel;
                    PlaySound(coinSound);
                }
                if (IsKeyPressed(KEY_THREE) && !nextWaveInvincibility &&
                    game.spendCoins(8)) {
                    nextWaveInvincibility = true;
                    PlaySound(coinSound);
                }
                if (IsKeyPressed(KEY_ENTER)) {
                    ++stage;
                    wavePhase = WavePhase::Survival;
                    phaseTime = 30.0F + static_cast<float>(stage - 1) * 5.0F;
                    flySpawnTimer = 0.8F;
                    if (nextWaveInvincibility) {
                        invincibilityTime = 8.0F;
                        nextWaveInvincibility = false;
                    }
                    PlaySound(phaseSound);
                }
            }
        }

        fireCooldown = std::max(0.0F, fireCooldown - delta);
        muzzleFlash = std::max(0.0F, muzzleFlash - delta);
        damageCooldown = std::max(0.0F, damageCooldown - delta);
        invincibilityTime = std::max(0.0F, invincibilityTime - delta);
        const float fireInterval = std::max(0.08F, 0.18F - fireRateLevel * 0.025F);
        if (screen == Screen::Playing && wavePhase != WavePhase::Shop && IsKeyDown(KEY_SPACE) &&
            fireCooldown <= 0.0F && game.state() == GameState::Playing) {
            const Vector3 direction = directionVector(game.direction());
            Vector3 origin = worldPosition(game.snake().front(), game.boardSize(), 0.72F);
            origin.x += direction.x * 0.58F;
            origin.z += direction.z * 0.58F;
            bullets.push_back({origin, direction, 2.0F});
            fireCooldown = fireInterval;
            muzzleFlash = 0.075F;
            PlaySound(fireSound);
        }
        if (screen == Screen::Playing && game.state() == GameState::Playing) {
            if (wavePhase != WavePhase::Shop) phaseTime -= delta;
            if (wavePhase == WavePhase::Survival) {
                flySpawnTimer -= delta;
                const float spawnInterval = std::max(1.05F, 2.30F - stage * 0.08F);
                if (flySpawnTimer <= 0.0F && flies.size() < 16) {
                    flies.push_back(spawnFly(game.boardSize(), random));
                    flySpawnTimer = spawnInterval;
                }
                if (phaseTime <= 0.0F) {
                    wavePhase = WavePhase::Grace;
                    phaseTime = 10.0F;
                    flies.clear();
                    PlaySound(phaseSound);
                }
            } else if (wavePhase == WavePhase::Grace && phaseTime <= 0.0F) {
                wavePhase = WavePhase::Shop;
                bullets.clear();
                PlaySound(phaseSound);
            }

            const Vector3 head = worldPosition(game.snake().front(), game.boardSize(), 0.58F);
            if (wavePhase == WavePhase::Survival) {
                const float flySpeed = 1.75F + static_cast<float>(stage - 1) * 0.12F;
                for (Fly& fly : flies) {
                    const float dx = head.x - fly.position.x;
                    const float dz = head.z - fly.position.z;
                    const float length = std::sqrt(dx * dx + dz * dz);
                    if (length > 0.001F) {
                        fly.position.x += dx / length * flySpeed * delta;
                        fly.position.z += dz / length * flySpeed * delta;
                    }
                }
                for (Fly& fly : flies) {
                    const float dx = head.x - fly.position.x;
                    const float dz = head.z - fly.position.z;
                    if (dx * dx + dz * dz < 0.30F) {
                        fly.position.y = -100.0F;
                        if (damageCooldown <= 0.0F && invincibilityTime <= 0.0F) {
                            game.takeDamage();
                            damageCooldown = 0.75F;
                            PlaySound(damageSound);
                        }
                    }
                }
                std::erase_if(flies, [](const Fly& fly) { return fly.position.y < 0.0F; });
            }

            for (Coin& coin : droppedCoins) {
                coin.age += delta;
                const float dx = head.x - coin.position.x;
                const float dz = head.z - coin.position.z;
                if (dx * dx + dz * dz < 0.32F) {
                    coin.position.y = -100.0F;
                    game.collectCoin();
                    PlaySound(coinSound);
                }
            }
            std::erase_if(droppedCoins,
                          [](const Coin& coin) { return coin.position.y < 0.0F; });

            for (Bullet& bullet : bullets) {
                bullet.position.x += bullet.direction.x * 18.0F * delta;
                bullet.position.z += bullet.direction.z * 18.0F * delta;
                bullet.life -= delta;

                const float boardEdge = static_cast<float>(game.boardSize()) * 0.5F;
                if (std::abs(bullet.position.x) >= boardEdge ||
                    std::abs(bullet.position.z) >= boardEdge) {
                    bullet.life = 0.0F;
                    continue;
                }

                for (Fly& fly : flies) {
                    const float fx = bullet.position.x - fly.position.x;
                    const float fy = bullet.position.y - fly.position.y;
                    const float fz = bullet.position.z - fly.position.z;
                    if (fx * fx + fy * fy + fz * fz < 0.20F) {
                        droppedCoins.push_back({{fly.position.x, 0.28F, fly.position.z}, 0.0F});
                        fly.position.y = -100.0F;
                        bullet.life = 0.0F;
                        PlaySound(flySound);
                        break;
                    }
                }
                if (bullet.life <= 0.0F) continue;

                const Vector3 apple = worldPosition(game.food(), game.boardSize(), 0.46F);
                const float dx = bullet.position.x - apple.x;
                const float dy = bullet.position.y - apple.y;
                const float dz = bullet.position.z - apple.z;
                if (dx * dx + dy * dy + dz * dz < 0.24F) {
                    if (game.shootFood(game.food())) {
                        bullet.life = 0.0F;
                        PlaySound(hitSound);
                    }
                }

                for (std::size_t i = 0; i < cupcakes.size() && bullet.life > 0.0F; ++i) {
                    const Vector3 enemy = worldPosition(cupcakes[i], game.boardSize(), 0.58F);
                    const float ex = bullet.position.x - enemy.x;
                    const float ey = bullet.position.y - enemy.y;
                    const float ez = bullet.position.z - enemy.z;
                    if (ex * ex + ey * ey + ez * ez < 0.30F) {
                        cupcakes.erase(cupcakes.begin() + static_cast<std::ptrdiff_t>(i));
                        game.defeatCupcake();
                        bullet.life = 0.0F;
                        PlaySound(cupcakeSound);
                        refillCupcakes(game, cupcakes, random);
                    }
                }
            }
            std::erase_if(flies, [](const Fly& fly) { return fly.position.y < 0.0F; });
            std::erase_if(bullets, [](const Bullet& bullet) { return bullet.life <= 0.0F; });
        }

        const double interval = std::max(0.075, 0.19 - game.score() * 0.0015);
        if (screen == Screen::Playing && wavePhase != WavePhase::Shop &&
            GetTime() - lastStep >= interval) {
            const int previousScore = game.score();
            const GameState previousState = game.state();
            game.step();
            if (game.score() > previousScore) PlaySound(eatSound);
            if (previousState == GameState::Playing && game.state() == GameState::GameOver)
                PlaySound(crashSound);
            lastStep = GetTime();
        }

        BeginDrawing();
        ClearBackground({14, 20, 30, 255});
        BeginMode3D(camera);
        drawBoard(game);
        drawBlessing(game.food(), game.boardSize(), static_cast<float>(GetTime()));
        for (Cell cupcake : cupcakes) drawCupcake(cupcake, game.boardSize());
        for (const Coin& coin : droppedCoins) drawCoin(coin, static_cast<float>(GetTime()));
        for (const Fly& fly : flies) drawFly(fly, static_cast<float>(GetTime()));
        drawBullets(bullets);
        drawSnake(game, muzzleFlash);
        EndMode3D();

        if (screen == Screen::Title) {
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(Color{8, 10, 18, 255}, 0.82F));
            centeredText("BALTHAZAR", 205, 72, Color{255, 210, 72, 255});
            centeredText("THE BECOMING OF A GOD", 290, 30, Color{196, 77, 133, 255});
            centeredText("A divine serpent awakens hungry.", 380, 24, RAYWHITE);
            centeredText("Press ENTER", 475, 24, Color{123, 231, 196, 255});
        } else if (screen == Screen::Introduction) {
            DrawRectangle(90, 105, screenWidth - 180, screenHeight - 210,
                          Fade(Color{8, 10, 18, 255}, 0.93F));
            centeredText("CHILD OF WADJET", 145, 36, Color{255, 210, 72, 255});
            centeredText("Balthazar once devoured every Blessing he possessed", 235, 22, RAYWHITE);
            centeredText("to defeat the Ravenous Cupcake and save the world.", 270, 22, RAYWHITE);
            centeredText("After millennia of slumber, he has awakened powerless...", 340, 22, RAYWHITE);
            centeredText("AND EXTREMELY HUNGRY.", 385, 30, Color{229, 75, 92, 255});
            centeredText("Devour Blessings. Destroy Cupcakes. Ascend.", 460, 24,
                         Color{123, 231, 196, 255});
            centeredText("Survive the swarm and collect what the flies leave behind.", 495, 20,
                         Color{196, 204, 216, 255});
            centeredText("Press ENTER to begin", 555, 20, Color{160, 174, 194, 255});
        } else {
            centeredText("BALTHAZAR: THE BECOMING OF A GOD", 24, 32,
                         Color{255, 210, 72, 255});
            centeredText("Left/A + Right/D steer   |   Space fires   |   P pauses   |   R restarts",
                         64, 18, {160, 174, 194, 255});
            DrawText(TextFormat("DIVINE POWER  %04i", game.score()), 42,
                     screenHeight - 66, 24, RAYWHITE);
            DrawText(TextFormat("COINS  %03i", game.coins()), 315,
                     screenHeight - 64, 22, Color{255, 210, 72, 255});
            const char* phaseLabel = wavePhase == WavePhase::Survival ? "SURVIVE" :
                                     wavePhase == WavePhase::Grace ? "GRACE" : "SHOP";
            const Color phaseColor = wavePhase == WavePhase::Survival
                ? Color{229, 75, 92, 255} : Color{123, 231, 196, 255};
            const int shownTime = wavePhase == WavePhase::Shop ? 0 :
                static_cast<int>(std::ceil(std::max(0.0F, phaseTime)));
            DrawText(TextFormat("STAGE %02i   %s  %02i", stage, phaseLabel, shownTime),
                     470, screenHeight - 64, 21, phaseColor);
            const float ascension = std::min(1.0F, static_cast<float>(game.score()) / 200.0F);
            DrawText("ASCENSION", 745, screenHeight - 67, 18, Color{255, 210, 72, 255});
            DrawRectangle(865, screenHeight - 65, 190, 18, Color{32, 42, 53, 255});
            DrawRectangle(865, screenHeight - 65, static_cast<int>(190.0F * ascension), 18,
                          Color{196, 77, 133, 255});
            if (invincibilityTime > 0.0F)
                centeredText(TextFormat("DIVINE SHIELD  %.1f", invincibilityTime), 96, 20,
                             Color{123, 231, 196, 255});
        }
        if (screen == Screen::Playing && wavePhase == WavePhase::Shop) {
            DrawRectangle(165, 150, 770, 450, Fade(Color{8, 10, 18, 255}, 0.95F));
            centeredText("GRACE MARKET", 180, 42, Color{255, 210, 72, 255});
            centeredText(TextFormat("COINS AVAILABLE: %i", game.coins()), 238, 22, RAYWHITE);
            DrawText("1   Add one tail segment", 260, 305, 25, RAYWHITE);
            DrawText("5 coins", 745, 305, 25, Color{255, 210, 72, 255});
            DrawText("2   Faster firing", 260, 365, 25, RAYWHITE);
            DrawText(TextFormat("6 coins   Level %i/3", fireRateLevel), 670, 365, 23,
                     fireRateLevel >= 3 ? GRAY : Color{255, 210, 72, 255});
            DrawText("3   Divine shield for next wave", 260, 425, 25, RAYWHITE);
            DrawText(nextWaveInvincibility ? "PURCHASED" : "8 coins", 745, 425, 23,
                     nextWaveInvincibility ? Color{123, 231, 196, 255}
                                              : Color{255, 210, 72, 255});
            centeredText("Press ENTER to begin the next wave", 520, 21,
                         Color{123, 231, 196, 255});
        }
        if (screen == Screen::Playing && game.state() == GameState::Paused)
            centeredText("PAUSED", 345, 48, RAYWHITE);
        if (screen == Screen::Playing && game.state() == GameState::GameOver) {
            DrawRectangle(0, 310, screenWidth, 120, Fade(BLACK, 0.72F));
            centeredText("GAME OVER", 325, 48, {255, 102, 108, 255});
            centeredText("Press R to try again", 383, 22, RAYWHITE);
        }
        if (screen == Screen::Playing && game.state() == GameState::Won)
            centeredText("YOU FILLED THE BOARD!", 345, 40, {255, 221, 87, 255});
        if (captureFrame && --captureDelay <= 0) {
            rlDrawRenderBatchActive();
            TakeScreenshot(argv[2]);
            done = true;
        }
        EndDrawing();
    }

    UnloadSound(fireSound);
    UnloadSound(hitSound);
    UnloadSound(eatSound);
    UnloadSound(crashSound);
    UnloadSound(cupcakeSound);
    UnloadSound(introSound);
    UnloadSound(flySound);
    UnloadSound(damageSound);
    UnloadSound(coinSound);
    UnloadSound(phaseSound);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
