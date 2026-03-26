#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>

// Константы игры
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const float PLAYER_SPEED = 200.0f;
const float BULLET_SPEED = 500.0f;
const float ENEMY_SPEED = 80.0f;
const float FIRE_RATE = 0.15f; // секунды между выстрелами

// Класс игрока
class Player {
public:
    sf::RectangleShape shape;
    float fireTimer;
    
    Player() : fireTimer(0) {
        shape.setSize(sf::Vector2f(40, 40));
        shape.setFillColor(sf::Color::Green);
        shape.setPosition(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f);
        shape.setOrigin(20, 20);
    }
    
    void update(float dt) {
        sf::Vector2f movement(0, 0);
        
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || 
            sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) movement.y -= 1;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || 
            sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) movement.y += 1;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || 
            sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) movement.x -= 1;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || 
            sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) movement.x += 1;
        
        // Нормализация диагонального движения
        if (movement.x != 0 || movement.y != 0) {
            float length = std::sqrt(movement.x * movement.x + movement.y * movement.y);
            movement.x /= length;
            movement.y /= length;
        }
        
        shape.move(movement * PLAYER_SPEED * dt);
        
        // Ограничение пределами экрана
        sf::Vector2f pos = shape.getPosition();
        pos.x = std::max(20.0f, std::min(pos.x, (float)WINDOW_WIDTH - 20));
        pos.y = std::max(20.0f, std::min(pos.y, (float)WINDOW_HEIGHT - 20));
        shape.setPosition(pos);
        
        // Таймер стрельбы
        if (fireTimer > 0) fireTimer -= dt;
    }
    
    bool canFire() const {
        return fireTimer <= 0;
    }
    
    void fire() {
        fireTimer = FIRE_RATE;
    }
    
    sf::Vector2f getPosition() const {
        return shape.getPosition();
    }
};

// Класс пули
class Bullet {
public:
    sf::CircleShape shape;
    sf::Vector2f velocity;
    bool active;
    
    Bullet() : active(false) {
        shape.setRadius(5);
        shape.setFillColor(sf::Color::Yellow);
        shape.setOrigin(5, 5);
    }
    
    void spawn(sf::Vector2f position, sf::Vector2f direction) {
        shape.setPosition(position);
        velocity = direction * BULLET_SPEED;
        active = true;
    }
    
    void update(float dt) {
        if (!active) return;
        
        shape.move(velocity * dt);
        
        // Деактивация если за пределами экрана
        sf::Vector2f pos = shape.getPosition();
        if (pos.x < 0 || pos.x > WINDOW_WIDTH || pos.y < 0 || pos.y > WINDOW_HEIGHT) {
            active = false;
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
    bool active;
    
    Enemy() : active(false) {
        shape.setSize(sf::Vector2f(30, 30));
        shape.setFillColor(sf::Color::Red);
        shape.setOrigin(15, 15);
    }
    
    void spawn() {
        // Случайная позиция по краям экрана
        int side = rand() % 4;
        switch(side) {
            case 0: // верх
                shape.setPosition(rand() % WINDOW_WIDTH, -30);
                break;
            case 1: // низ
                shape.setPosition(rand() % WINDOW_WIDTH, WINDOW_HEIGHT + 30);
                break;
            case 2: // лево
                shape.setPosition(-30, rand() % WINDOW_HEIGHT);
                break;
            case 3: // право
                shape.setPosition(WINDOW_WIDTH + 30, rand() % WINDOW_HEIGHT);
                break;
        }
        active = true;
    }
    
    void update(float dt, sf::Vector2f playerPos) {
        if (!active) return;
        
        sf::Vector2f dir = playerPos - shape.getPosition();
        float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (length > 0) {
            dir.x /= length;
            dir.y /= length;
        }
        
        shape.move(dir * ENEMY_SPEED * dt);
    }
    
    sf::FloatRect getBounds() const {
        return shape.getGlobalBounds();
    }
};

int main() {
    srand(static_cast<unsigned>(time(nullptr)));
    
    // Создание окна
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), 
                           "Basic Shooter - WASD to move, Mouse to aim and shoot");
    window.setFramerateLimit(60);
    
    // Инициализация объектов
    Player player;
    std::vector<Bullet> bullets;
    std::vector<Enemy> enemies;
    
    // Предварительное создание пуль и врагов
    for (int i = 0; i < 10; i++) {
        bullets.push_back(Bullet());
    }
    for (int i = 0; i < 5; i++) {
        enemies.push_back(Enemy());
    }
    
    // Спавн первого врага
    enemies[0].spawn();
    int activeEnemies = 1;
    
    int score = 0;
    sf::Font font;
    
    // Попытка загрузить шрифт (если не получится, просто не будем отображать счет)
    // Используем системный шрифт или дефолтный
    if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
        // Шрифт не загрузился, но игра продолжит работу
    }
    
    sf::Text scoreText;
    scoreText.setFont(font);
    scoreText.setCharacterSize(24);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition(10, 10);
    
    sf::Clock clock;
    sf::Clock enemySpawnClock;
    float enemySpawnInterval = 2.0f; // секунды
    
    // Игровой цикл
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        
        // Обработка событий
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            
            // Стрельба по клику мыши
            if (event.type == sf::Event::MouseButtonPressed && 
                event.mouseButton.button == sf::Mouse::Left) {
                if (player.canFire()) {
                    player.fire();
                    
                    // Находим свободную пулю
                    for (auto& bullet : bullets) {
                        if (!bullet.active) {
                            sf::Vector2f playerPos = player.getPosition();
                            sf::Vector2f mousePos = window.mapPixelToCoords(
                                sf::Mouse::getPosition(window));
                            
                            sf::Vector2f dir = mousePos - playerPos;
                            float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
                            if (length > 0) {
                                dir.x /= length;
                                dir.y /= length;
                            }
                            
                            bullet.spawn(playerPos, dir);
                            break;
                        }
                    }
                }
            }
        }
        
        // Обновление игрока
        player.update(dt);
        
        // Обновление пуль
        for (auto& bullet : bullets) {
            bullet.update(dt);
        }
        
        // Спавн врагов
        if (enemySpawnClock.getElapsedTime().asSeconds() >= enemySpawnInterval) {
            enemySpawnClock.restart();
            
            // Находим свободного врага
            for (auto& enemy : enemies) {
                if (!enemy.active) {
                    enemy.spawn();
                    activeEnemies++;
                    break;
                }
            }
            
            // Усложнение со временем
            if (enemySpawnInterval > 0.5f) {
                enemySpawnInterval -= 0.05f;
            }
        }
        
        // Обновление врагов
        for (auto& enemy : enemies) {
            if (enemy.active) {
                enemy.update(dt, player.getPosition());
            }
        }
        
        // Проверка столкновений
        for (auto& bullet : bullets) {
            if (!bullet.active) continue;
            
            for (auto& enemy : enemies) {
                if (!enemy.active) continue;
                
                if (bullet.getBounds().intersects(enemy.getBounds())) {
                    bullet.active = false;
                    enemy.active = false;
                    activeEnemies--;
                    score += 10;
                    scoreText.setString("Score: " + std::to_string(score));
                    break;
                }
            }
        }
        
        // Проверка столкновения игрока с врагом
        for (auto& enemy : enemies) {
            if (!enemy.active) continue;
            
            if (player.shape.getGlobalBounds().intersects(enemy.getBounds())) {
                // Game Over - перезапуск
                score = 0;
                scoreText.setString("Score: 0");
                enemySpawnInterval = 2.0f;
                
                // Деактивируем всех врагов
                for (auto& e : enemies) {
                    e.active = false;
                }
                activeEnemies = 0;
                
                // Спавним первого врага
                enemies[0].spawn();
                activeEnemies = 1;
                
                // Сброс позиции игрока
                player.shape.setPosition(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f);
                break;
            }
        }
        
        // Отрисовка
        window.clear(sf::Color::Black);
        
        window.draw(player.shape);
        
        for (auto& bullet : bullets) {
            if (bullet.active) {
                window.draw(bullet.shape);
            }
        }
        
        for (auto& enemy : enemies) {
            if (enemy.active) {
                window.draw(enemy.shape);
            }
        }
        
        window.draw(scoreText);
        
        window.display();
    }
    
    return 0;
}
