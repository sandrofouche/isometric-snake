#pragma once

#include <cstdint>
#include <random>
#include <vector>

struct Cell {
    int x{};
    int y{};
    friend bool operator==(Cell, Cell) = default;
};

enum class Direction { North, East, South, West };
enum class GameState { Playing, Paused, GameOver, Won };

class SnakeGame {
public:
    explicit SnakeGame(int boardSize = 14, std::uint32_t seed = std::random_device{}());

    void reset();
    void requestDirection(Direction direction);
    void step();
    void togglePause();
    bool shootFood(Cell cell);
    void defeatCupcake();
    void takeDamage();
    void collectCoin();

    [[nodiscard]] int boardSize() const { return boardSize_; }
    [[nodiscard]] int score() const { return score_; }
    [[nodiscard]] int coins() const { return coins_; }
    [[nodiscard]] GameState state() const { return state_; }
    [[nodiscard]] Direction direction() const { return direction_; }
    [[nodiscard]] const std::vector<Cell>& snake() const { return snake_; }
    [[nodiscard]] Cell food() const { return food_; }

private:
    [[nodiscard]] bool isOccupied(Cell cell) const;
    [[nodiscard]] bool isOpposite(Direction a, Direction b) const;
    void placeFood();

    int boardSize_;
    int score_{};
    int coins_{};
    Direction direction_{Direction::East};
    Direction requestedDirection_{Direction::East};
    GameState state_{GameState::Playing};
    std::vector<Cell> snake_;
    Cell food_{};
    std::mt19937 random_;
};
