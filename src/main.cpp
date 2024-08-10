#include <SFML/Graphics.hpp>
#include <deque>
#include <iostream>
#include <optional>
#include <random>
#include <unordered_set>

enum class Direction : int { Left, Up, Right, Down };
enum class GameStatus : int { InProgress, Win, Loss };

std::optional<Direction> get_move() {
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
    return Direction::Left;
  } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
    return Direction::Up;
  } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
    return Direction::Right;
  } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
    return Direction::Down;
  }
  return std::nullopt;
}

namespace std {
template <> struct hash<sf::Vector2i> {
  std::size_t operator()(const sf::Vector2i &obj) const {
    // Simple hash function based on the value
    return std::hash<int>()(obj.x) + std::hash<int>()(obj.y);
  }
};
} // namespace std

// class Snake

void drawCells(sf::RenderWindow &window, std::vector<sf::Vector2i> cells,
               sf::Color color, float blockSize) {
  for (sf::Vector2i cell : cells) {
    sf::Vector2f objFloat(static_cast<float>(cell.x),
                          static_cast<float>(cell.y));

    sf::RectangleShape shape({blockSize, blockSize});
    shape.setPosition(objFloat);
    shape.setFillColor(color);
    window.draw(shape);
  }
}

struct RandomGen {
  RandomGen(int from, int to)
      : rng_(std::random_device{}()), uniform_(from, to) {}
  int generate() { return uniform_(rng_); }
  std::mt19937 rng_;
  std::uniform_int_distribution<std::mt19937::result_type> uniform_;
};

using Apple = sf::Vector2i;

class Snake {
public:
  Snake() = default;
  Snake(sf::Vector2i start)
      : direction_(Direction::Right), head_(start),
        tail_(start - sf::Vector2i{1, 0}) {}

  void turn(std::optional<Direction> maybeNewDirection) {
    if (!maybeNewDirection)
      return;
    auto direction = maybeNewDirection.value();
    if (bannedTransitions_[direction_] != direction) {
      direction_ = direction;
    }
  }
  void move(Direction maybeNewDirection) {
    turn(maybeNewDirection);
    move();
  }

  void move() {
    sf::Vector2i prevHead = head_;

    // move the head
    switch (direction_) {
    case Direction::Left:
      head_ += {-1, 0};
      break;
    case Direction::Right:
      head_ += {1, 0};
      break;
    case Direction::Up:
      head_ += {0, -1};
      break;
    case Direction::Down:
      head_ += {0, 1};
      break;
    }
    body_.push_front(prevHead);
    tail_ = body_.back();
    body_.pop_back();
  }

  void growTail() {
    if (tail_ != back()) {
      body_.push_back(tail_);
    }
  }

  sf::Vector2i back() {
    if (body_.empty()) {
      return head_;
    }
    return body_.back();
  }

  bool can_eat(Apple apple) const { return head_ == apple; }

  std::vector<sf::Vector2i> getGridBody() const {
    std::vector<sf::Vector2i> gridBody{head_};
    gridBody.insert(gridBody.end(), body_.begin(), body_.end());
    return gridBody;
  }

  sf::Vector2i head() const { return head_; }

  bool isSelfCollision() {
    for (const auto &bodyCell : body_) {
      if (bodyCell == head_) {
        return true;
      }
    }
    return false;
  }

  int length() const { return body_.size() + 1; }

private:
  Direction direction_;
  sf::Vector2i head_;
  sf::Vector2i tail_;
  std::unordered_map<Direction, Direction> bannedTransitions_{
      {Direction::Right, Direction::Left},
      {Direction::Left, Direction::Right},
      {Direction::Up, Direction::Down},
      {Direction::Down, Direction::Up}};
  std::deque<sf::Vector2i> body_;
};

struct Grid {
public:
  Grid(int width, int height) : width_(width), height_(height) {
    snake_ = Snake({0, 0});
    apple_ = spawnApple();
  }

  sf::Vector2i spawnApple() {
    std::vector<sf::Vector2i> snakeBody = snake_.getGridBody();
    std::unordered_set<sf::Vector2i> snakeSetBody(snakeBody.begin(),
                                                  snakeBody.end());
    std::vector<sf::Vector2i> available;
    available.reserve(width_ * height_ - snakeSetBody.size());
    for (int i = 0; i < width_ * height_; i++) {
      int y = i / width_;
      int x = i % width_;
      if (snakeSetBody.find({x, y}) == snakeSetBody.end()) {
        available.push_back({x, y});
      }
    }
    RandomGen gen(0, available.size() - 1);
    int num = gen.generate();
    return available[num];
  }

  void playMove(std::optional<Direction> move) {
    if (!move.has_value()) {
      snake_.move();
    } else {
      snake_.move(move.value());
    }
    auto snakeBody = snake_.getGridBody();
    auto snakeHead = snake_.head();
    // Check collision with wall or itself
    if (!(0 <= snakeHead.x && snakeHead.x < width_ && 0 <= snakeHead.y &&
          snakeHead.y < height_)) {
      std::cout << "RAN INTO A WALL";
      gameStatus_ = GameStatus::Loss;
      return;
    }
    if (snake_.isSelfCollision()) {
      std::cout << "RAN INTO MYSELF";
      gameStatus_ = GameStatus::Loss;
      return;
    }
    bool eaten = snake_.can_eat(apple_);
    if (eaten) {
      snake_.growTail();
    }
    if (snake_.length() == width_ * height_) {
      gameStatus_ = GameStatus::Win;
      return;
    }
    if (eaten) {
      apple_ = spawnApple();
    }
  }

  int width_;
  int height_;
  Snake snake_;
  Apple apple_;
  GameStatus gameStatus_{GameStatus::InProgress};
};

struct GridDrawer {
  GridDrawer(unsigned int modeWidth, unsigned int modeHeight, const char *title,
             int cellSize)
      : window_({modeWidth, modeHeight}, title), cellSize_(cellSize) {
    window_.setFramerateLimit(144);
  }
  void draw(const Grid &grid) {
    std::vector<sf::Vector2i> gridSnake = grid.snake_.getGridBody();
    for (auto &cell : gridSnake) {
      cell *= cellSize_;
    }
    sf::Vector2i gridApple = grid.apple_;
    gridApple *= cellSize_;
    drawCells(window_, gridSnake, sf::Color::Green, cellSize_);
    if (grid.gameStatus_ == GameStatus::InProgress) {
      drawCells(window_, {gridApple}, sf::Color::Red, cellSize_);
    }
  }

  sf::RenderWindow window_;
  int cellSize_;
};

int main() {

  int height = 2;
  int width = 5;
  int renderWidth = 300;
  int renderHeight = 300;
  auto small = std::min(renderWidth, renderHeight);
  auto blockSize = small / std::max(height, width);

  GridDrawer gridDrawer(blockSize * width, blockSize * height, "C++ Snake Game",
                        blockSize);
  auto &window = gridDrawer.window_;
  Grid grid(width, height);
  sf::Clock clock;
  const sf::Time delay = sf::seconds(0.7f);
  // Snake snake(window, blockSize);

  while (window.isOpen() && grid.gameStatus_ == GameStatus::InProgress) {
    for (auto event = sf::Event{}; window.pollEvent(event);) {
      switch (event.type) {
      case sf::Event::Closed:
        window.close();
      default:
        break;
      }
    }

    grid.snake_.turn(get_move());
    sf::Time elapsed = clock.getElapsedTime();
    if (elapsed < delay) {
      continue;
    }
    grid.playMove(get_move());
    window.clear(sf::Color::Black);
    gridDrawer.draw(grid);
    window.display();
    clock.restart();
  }
  std::cout << static_cast<int>(grid.gameStatus_);
}
