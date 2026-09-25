#include "game.hpp"
#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <numbers>
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

void drawApple(Cell cell, int boardSize) {
    const Vector3 base = worldPosition(cell, boardSize, 0.46F);
    DrawSphereEx(base, 0.36F, 16, 24, Color{229, 55, 69, 255});
    DrawSphereEx({base.x - 0.12F, base.y + 0.08F, base.z}, 0.27F, 12, 20,
                 Color{247, 75, 82, 255});
    DrawCylinderEx({base.x, base.y + 0.27F, base.z},
                   {base.x + 0.03F, base.y + 0.62F, base.z},
                   0.045F, 0.025F, 8, Color{91, 56, 35, 255});
    DrawSphereEx({base.x + 0.16F, base.y + 0.55F, base.z}, 0.12F, 6, 10,
                 Color{80, 190, 91, 255});
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
    const bool captureFrame = argc == 3 && std::string_view(argv[1]) == "--screenshot";
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, "Isometric Snake");
    InitAudioDevice();
    SetTargetFPS(60);

    SnakeGame game(18);
    std::vector<Bullet> bullets;
    float fireCooldown = 0.0F;
    float muzzleFlash = 0.0F;
    const Sound fireSound = makeTone(760.0F, 260.0F, 0.09F, 0.24F);
    const Sound hitSound = makeTone(180.0F, 520.0F, 0.14F, 0.32F);
    const Sound eatSound = makeTone(420.0F, 880.0F, 0.18F, 0.28F);
    const Sound crashSound = makeTone(160.0F, 55.0F, 0.38F, 0.38F);
    Camera3D camera{};
    camera.position = {13.5F, 16.0F, 13.5F};
    camera.target = {0.0F, 0.0F, 0.0F};
    camera.up = {0.0F, 1.0F, 0.0F};
    camera.fovy = 24.5F;
    camera.projection = CAMERA_ORTHOGRAPHIC;

    double lastStep = GetTime();
    bool done = false;
    while (!WindowShouldClose() && !done) {
        const float delta = GetFrameTime();
        readMovementInput(game);
        if (IsKeyPressed(KEY_P)) game.togglePause();
        if (IsKeyPressed(KEY_R)) {
            game.reset();
            bullets.clear();
            lastStep = GetTime();
        }

        fireCooldown = std::max(0.0F, fireCooldown - delta);
        muzzleFlash = std::max(0.0F, muzzleFlash - delta);
        if (IsKeyDown(KEY_SPACE) && fireCooldown <= 0.0F && game.state() == GameState::Playing) {
            const Vector3 direction = directionVector(game.direction());
            Vector3 origin = worldPosition(game.snake().front(), game.boardSize(), 0.72F);
            origin.x += direction.x * 0.58F;
            origin.z += direction.z * 0.58F;
            bullets.push_back({origin, direction, 2.0F});
            fireCooldown = 0.18F;
            muzzleFlash = 0.075F;
            PlaySound(fireSound);
        }
        if (game.state() == GameState::Playing) {
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
            }
            std::erase_if(bullets, [](const Bullet& bullet) { return bullet.life <= 0.0F; });
        }

        const double interval = std::max(0.075, 0.19 - game.score() * 0.0015);
        if (GetTime() - lastStep >= interval) {
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
        drawApple(game.food(), game.boardSize());
        drawBullets(bullets);
        drawSnake(game, muzzleFlash);
        EndMode3D();

        centeredText("ISOMETRIC SNAKE", 24, 36, {123, 231, 196, 255});
        centeredText("Left/A + Right/D steer   •   Space fires   •   P pauses   •   R restarts",
                     68, 18, {160, 174, 194, 255});
        DrawText(TextFormat("SCORE  %04i", game.score()), 42, screenHeight - 64, 28, RAYWHITE);
        if (game.state() == GameState::Paused) centeredText("PAUSED", 345, 48, RAYWHITE);
        if (game.state() == GameState::GameOver) {
            DrawRectangle(0, 310, screenWidth, 120, Fade(BLACK, 0.72F));
            centeredText("GAME OVER", 325, 48, {255, 102, 108, 255});
            centeredText("Press R to try again", 383, 22, RAYWHITE);
        }
        if (game.state() == GameState::Won)
            centeredText("YOU FILLED THE BOARD!", 345, 40, {255, 221, 87, 255});
        if (captureFrame) {
            TakeScreenshot(argv[2]);
            done = true;
        }
        EndDrawing();
    }

    UnloadSound(fireSound);
    UnloadSound(hitSound);
    UnloadSound(eatSound);
    UnloadSound(crashSound);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
