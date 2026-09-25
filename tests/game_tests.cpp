#include "game.hpp"

#include <cassert>
#include <iostream>

int main() {
    SnakeGame game(10, 42);
    assert(game.snake().size() == 3);
    assert(game.score() == 0);

    const Cell firstFood = game.food();
    assert(!game.shootFood({-1, -1}));
    assert(game.score() == 0);
    assert(game.shootFood(firstFood));
    assert(game.score() == 5);
    game.defeatCupcake();
    assert(game.score() == 20);

    game.reset();
    assert(game.score() == 0);
    assert(game.coins() == 0);

    game.collectCoin();
    game.collectCoin();
    assert(game.coins() == 2);
    game.takeDamage();
    assert(game.snake().size() == 2);
    game.takeDamage();
    assert(game.snake().size() == 1);
    game.takeDamage();
    assert(game.state() == GameState::GameOver);
    game.reset();

    const Cell start = game.snake().front();
    game.step();
    assert((game.snake().front() == Cell{start.x + 1, start.y}));

    game.requestDirection(Direction::West); // Reversal must be ignored.
    game.step();
    assert(game.direction() == Direction::East);

    game.requestDirection(Direction::North);
    game.step();
    assert(game.direction() == Direction::North);

    game.togglePause();
    const Cell pausedAt = game.snake().front();
    game.step();
    assert(game.snake().front() == pausedAt);
    game.togglePause();

    while (game.state() == GameState::Playing) game.step();
    assert(game.state() == GameState::GameOver);
    game.reset();
    assert(game.state() == GameState::Playing);
    assert(game.score() == 0);

    std::cout << "All snake core tests passed.\n";
}
