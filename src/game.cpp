#include "game.hpp"

#include <algorithm>
#include <stdexcept>

SnakeGame::SnakeGame(int boardSize, std::uint32_t seed)
    : boardSize_(boardSize), random_(seed) {
    if (boardSize < 6) throw std::invalid_argument("Board size must be at least 6");
    reset();
}

void SnakeGame::reset() {
    const int middle = boardSize_ / 2;
    snake_ = {{middle, middle}, {middle - 1, middle}, {middle - 2, middle}};
    score_ = 0;
    coins_ = 0;
    direction_ = Direction::East;
    requestedDirection_ = Direction::East;
    state_ = GameState::Playing;
    placeFood();
}

bool SnakeGame::isOpposite(Direction a, Direction b) const {
    return (a == Direction::North && b == Direction::South) ||
           (a == Direction::South && b == Direction::North) ||
           (a == Direction::East && b == Direction::West) ||
           (a == Direction::West && b == Direction::East);
}

void SnakeGame::requestDirection(Direction direction) {
    if (!isOpposite(direction, direction_)) requestedDirection_ = direction;
}

void SnakeGame::togglePause() {
    if (state_ == GameState::Playing) state_ = GameState::Paused;
    else if (state_ == GameState::Paused) state_ = GameState::Playing;
}

bool SnakeGame::shootFood(Cell cell) {
    if (state_ != GameState::Playing || cell != food_) return false;
    score_ += 5;
    placeFood();
    return true;
}

void SnakeGame::defeatCupcake() {
    if (state_ == GameState::Playing) score_ += 15;
}

void SnakeGame::takeDamage() {
    if (state_ != GameState::Playing) return;
    if (snake_.size() > 1) snake_.pop_back();
    else state_ = GameState::GameOver;
}

void SnakeGame::collectCoin() {
    if (state_ == GameState::Playing) ++coins_;
}

bool SnakeGame::isOccupied(Cell cell) const {
    return std::find(snake_.begin(), snake_.end(), cell) != snake_.end();
}

void SnakeGame::placeFood() {
    std::vector<Cell> empty;
    empty.reserve(static_cast<std::size_t>(boardSize_ * boardSize_ - snake_.size()));
    for (int y = 0; y < boardSize_; ++y) {
        for (int x = 0; x < boardSize_; ++x) {
            if (!isOccupied({x, y})) empty.push_back({x, y});
        }
    }
    if (empty.empty()) {
        state_ = GameState::Won;
        return;
    }
    std::uniform_int_distribution<std::size_t> pick(0, empty.size() - 1);
    food_ = empty[pick(random_)];
}

void SnakeGame::step() {
    if (state_ != GameState::Playing) return;
    direction_ = requestedDirection_;
    Cell next = snake_.front();
    switch (direction_) {
        case Direction::North: --next.y; break;
        case Direction::East:  ++next.x; break;
        case Direction::South: ++next.y; break;
        case Direction::West:  --next.x; break;
    }

    if (next.x < 0 || next.y < 0 || next.x >= boardSize_ || next.y >= boardSize_) {
        state_ = GameState::GameOver;
        return;
    }

    const bool grows = next == food_;
    const auto collisionEnd = grows ? snake_.end() : std::prev(snake_.end());
    if (std::find(snake_.begin(), collisionEnd, next) != collisionEnd) {
        state_ = GameState::GameOver;
        return;
    }

    snake_.insert(snake_.begin(), next);
    if (grows) {
        score_ += 10;
        placeFood();
    } else {
        snake_.pop_back();
    }
}
