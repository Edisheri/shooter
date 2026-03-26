#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <cmath>
#include <algorithm>

// Константы игры
const unsigned int WINDOW_WIDTH = 800;
const unsigned int WINDOW_HEIGHT = 600;
const float PLAYER_SPEED = 200.f;
const float BULLET_SPEED = 500.f;
const float ENEMY_SPEED = 100.f;
const float ENEMY_SPAWN_INTERVAL = 1.0f;

// Класс игрока
class Player {
public:
    sf::CircleShape shape;
    float speed;
    int score;
    bool isAlive;

    Player() {
        shape.setRadius(15.f);
        shape.setFillColor(sf::Color::Green);
        shape.setOrigin(15.f, 15.f);
        shape.setPosition(WINDOW_WIDTH / 2.f, WINDOW_HEIGHT / 2.f);
        speed = PLAYER_SPEED;
        score = 0;
        isAlive = true;
    }

    void update(float dt, const sf::Vector2i& mousePos) {
        sf::Vector2f movement(0.f, 0.f);

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
            movement.y -= 1.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
            movement.y += 1.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
            movement.x -= 1.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
            movement.x += 1.f;

        if (movement.x != 0.f || movement.y != 0.f) {
            float length = std::sqrt(movement.x * movement.x + movement.y * movement.y);
            movement /= length;
        }

        shape.move(movement * speed * dt);

        if (shape.getPosition().x < 0) shape.setPosition(0.f, shape.getPosition().y);
        if (shape.getPosition().x > WINDOW_WIDTH) shape.setPosition((float)WINDOW_WIDTH, shape.getPosition().y);
        if (shape.getPosition().y < 0) shape.setPosition(shape.getPosition().x, 0.f);
        if (shape.getPosition().y > WINDOW_HEIGHT) shape.setPosition(shape.getPosition().x, (float)WINDOW_HEIGHT);

        float angle = std::atan2(mousePos.y - shape.getPosition().y, mousePos.x - shape.getPosition().x);
        shape.setRotation(angle * 180.f / 3.14159f);
    }
};

// Класс пули
class Bullet {
public:
    sf::CircleShape shape;
    sf::Vector2f velocity;
    bool active;

    Bullet() : active(false) {
        shape.setRadius(5.f);
        shape.setFillColor(sf::Color::Yellow);
        shape.setOrigin(5.f, 5.f);
    }

    void fire(sf::Vector2f startPos, sf::Vector2f direction) {
        shape.setPosition(startPos);
        velocity = direction * BULLET_SPEED;
        active = true;
    }

    void update(float dt) {
        if (active) {
            shape.move(velocity * dt);
            if (shape.getPosition().x < -10 || shape.getPosition().x > WINDOW_WIDTH + 10 ||
                shape.getPosition().y < -10 || shape.getPosition().y > WINDOW_HEIGHT + 10) {
                active = false;
            }
        }
    }

    sf::FloatRect getBounds() const {
        return shape.getGlobalBounds();
    }
};

// Класс врага
class Enemy {
public:
    sf::RectangleShape shape;
    float speed;
    bool active;

    Enemy() : active(false) {
        shape.setSize(sf::Vector2f(30.f, 30.f));
        shape.setFillColor(sf::Color::Red);
        shape.setOrigin(15.f, 15.f);
        speed = ENEMY_SPEED;
    }

    void spawn() {
        int side = rand() % 4;
        switch(side) {
            case 0:
                shape.setPosition(sf::Vector2f((float)(rand() % WINDOW_WIDTH), -30.f));
                break;
            case 1:
                shape.setPosition(sf::Vector2f((float)(rand() % WINDOW_WIDTH), WINDOW_HEIGHT + 30.f));
                break;
            case 2:
                shape.setPosition(sf::Vector2f(-30.f, (float)(rand() % WINDOW_HEIGHT)));
                break;
            case 3:
                shape.setPosition(sf::Vector2f(WINDOW_WIDTH + 30.f, (float)(rand() % WINDOW_HEIGHT)));
                break;
        }
        active = true;
        speed = ENEMY_SPEED + (rand() % 50);
    }

    void update(float dt, sf::Vector2f targetPos) {
        if (active) {
            sf::Vector2f direction = targetPos - shape.getPosition();
            float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
            if (length > 0) {
                direction /= length;
                shape.move(direction * speed * dt);
            }
        }
    }

    sf::FloatRect getBounds() const {
        return shape.getGlobalBounds();
    }
};

int main() {
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "SFML Shooter");
    window.setFramerateLimit(60);

    srand(static_cast<unsigned>(time(nullptr)));

    sf::Font font;
    std::string fontPath = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
    
    if (!font.loadFromFile(fontPath)) {
        std::cerr << "Warning: Could not load font. Text will not be displayed." << std::endl;
    }

    sf::Text scoreText;
    scoreText.setFont(font);
    scoreText.setString("Score: 0");
    scoreText.setCharacterSize(30);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition(10.f, 10.f);

    Player player;
    std::vector<Bullet> bullets;
    std::vector<Enemy> enemies;

    sf::Clock clock;
    sf::Clock enemySpawnClock;
    float enemySpawnTimer = 0.f;
    bool gameOver = false;

    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            
            if (event.type == sf::Event::MouseButtonPressed && !gameOver) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    Bullet bullet;
                    sf::Vector2f playerPos = player.shape.getPosition();
                    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                    
                    sf::Vector2f direction((float)(mousePos.x - playerPos.x), (float)(mousePos.y - playerPos.y));
                    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                    if (length > 0) direction /= length;
                    
                    bullet.fire(playerPos, direction);
                    bullets.push_back(bullet);
                }
            }
            
            if (event.type == sf::Event::KeyPressed && gameOver) {
                if (event.key.code == sf::Keyboard::Space) {
                    player = Player();
                    bullets.clear();
                    enemies.clear();
                    scoreText.setString("Score: 0");
                    gameOver = false;
                }
            }
        }

        if (!gameOver) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);
            player.update(dt, mousePos);

            for (auto& bullet : bullets) {
                bullet.update(dt);
            }
            bullets.erase(std::remove_if(bullets.begin(), bullets.end(), 
                [](const Bullet& b) { return !b.active; }), bullets.end());

            enemySpawnTimer += dt;
            if (enemySpawnTimer >= ENEMY_SPAWN_INTERVAL) {
                Enemy enemy;
                enemy.spawn();
                enemies.push_back(enemy);
                enemySpawnTimer = 0.f;
            }

            for (auto& enemy : enemies) {
                enemy.update(dt, player.shape.getPosition());
            }

            for (auto& bullet : bullets) {
                if (!bullet.active) continue;
                for (auto& enemy : enemies) {
                    if (!enemy.active) continue;
                    if (bullet.getBounds().intersects(enemy.getBounds())) {
                        bullet.active = false;
                        enemy.active = false;
                        player.score += 10;
                        scoreText.setString("Score: " + std::to_string(player.score));
                        break;
                    }
                }
            }

            enemies.erase(std::remove_if(enemies.begin(), enemies.end(), 
                [](const Enemy& e) { return !e.active; }), enemies.end());

            for (const auto& enemy : enemies) {
                if (player.shape.getGlobalBounds().intersects(enemy.getBounds())) {
                    gameOver = true;
                    break;
                }
            }
        }

        window.clear(sf::Color::Black);
        
        if (!gameOver) {
            window.draw(player.shape);
        }
        
        for (const auto& bullet : bullets) {
            window.draw(bullet.shape);
        }
        
        for (const auto& enemy : enemies) {
            window.draw(enemy.shape);
        }
        
        window.draw(scoreText);

        if (gameOver) {
            sf::Text gameOverText;
            gameOverText.setFont(font);
            gameOverText.setString("GAME OVER!\nPress SPACE to restart");
            gameOverText.setCharacterSize(40);
            gameOverText.setFillColor(sf::Color::Red);
            gameOverText.setPosition(WINDOW_WIDTH / 2.f - 150.f, WINDOW_HEIGHT / 2.f - 50.f);
            window.draw(gameOverText);
        }

        window.display();
    }

    return 0;
}
