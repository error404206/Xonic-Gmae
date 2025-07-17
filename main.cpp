#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <time.h>
#include <string>
#include <fstream>
#include <cmath>
#include <iostream>
#include <vector>
using namespace sf;
using namespace std;

// Constants for Movement Modes
const int MODE_LINEAR = 0;
const int MODE_ZIGZAG = 1;
const int MODE_CIRCLE = 2;

// Global Game Variables
int totalTiles = 0;
const int M = 25;
const int N = 40;
int moveNumber = 0;
int drawCount = 0;
int grid[M][N] = {0};
int trailOwner[M][N] = {0};
int moveCount1 = 0, moveCount2 = 0;
int tileCounts1[100] = {0}, tileCounts2[100] = {0};
int totalTiles1 = 0, totalTiles2 = 0;
int tileCount1 = 0, tileCount2 = 0;
int drawCount1 = 0, drawCount2 = 0;
int ts = 18;
int bonusCount = 0;
int currentThreshold = 10;
int powerUps = 0;
int powerUps2 = 0;
bool enemyStopped = false;
float stopTimer = 0.0f;
bool player1Stopped = false;
bool player2Stopped = false;
float playerStopTimer = 0.0f;
int nextPowerUp = 50;
int nextPowerUp2 = 50;
int totalRawTiles = 0;
int totalRawTiles2 = 0;
int currentThreshold2 = 10;
int bonusCount2 = 0;

// Sound Declarations
sf::SoundBuffer captureBuffer;
sf::Sound captureSound;
sf::SoundBuffer bonusBuffer;
sf::Sound bonusSound;
sf::SoundBuffer powerUpBuffer;
sf::Sound powerUpSound;
sf::SoundBuffer trailBuffer;
sf::Sound trailSound;
bool wasBuilding1 = false;
bool wasBuilding2 = false;

struct ScoreEntry { 
    int score; 
    int time;
};

// Particle structure for spark effects
struct Particle {
    Vector2f position;
    Vector2f velocity;
    float lifetime;
    Color color;

    Particle(float x, float y, float vx, float vy, float life, Color c)
        : position(x, y), velocity(vx, vy), lifetime(life), color(c) {}
};

// Enemy Logic
struct Enemy {
    int x, y;
    int dx, dy;
    int mode;
    int zigDir;
    int zigSteps;
    int zigLength;
    int zigDirVert;
    int zigTick;
    int zigWait;
    float angle;
    float cx, cy;
    float radius;
    float angleSpeed;

    Enemy() {
        x = y = 300;
        dx = 4 - rand() % 8;
        dy = 4 - rand() % 8;
        mode = MODE_LINEAR;
        zigDir = 0;
        zigSteps = 0;
        zigLength = 25;
        zigDirVert = +1;
        zigTick = 0;
        zigWait = 2;
        cx = float(x);
        cy = float(y);
        radius = ts * 5.0f;
        angle = 0.0f;
        angleSpeed = 0.10f;
    }

    void moveLinear() {
        x += dx;
        if (grid[y / ts][x / ts] == 1) { dx = -dx; x += dx; }
        y += dy;
        if (grid[y / ts][x / ts] == 1) { dy = -dy; y += dy; }
    }

    void moveZigZag() {
        if (++zigTick < zigWait) return;
        zigTick = 0;
        int stepX = (zigDir == 0 ? ts : -ts);
        int newX = x + stepX;
        if (newX < 0 || newX >= N * ts || grid[y / ts][newX / ts] == 1) {
            zigDir = 1 - zigDir;
            zigSteps = 0;
        } else {
            x = newX;
            zigSteps++;
        }
        if (zigSteps >= zigLength) {
            zigSteps = 0;
            int newY = y + zigDirVert * ts;
            if (newY < 0 || newY >= M * ts || grid[newY / ts][x / ts] == 1) {
                zigDirVert = -zigDirVert;
                newY = y + zigDirVert * ts;
                if (newY < 0 || newY >= M * ts || grid[newY / ts][x / ts] == 1) {
                    newY = y;
                }
            }
            y = newY;
            zigDir = 1 - zigDir;
        }
        x = clamp(x, 0, N * ts - 1);
        y = clamp(y, 0, M * ts - 1);
    }

    void moveCircle() {
        cx += dx;
        if (grid[int(cy) / ts][int(cx) / ts] == 1) { dx = -dx; cx += dx; }
        cy += dy;
        if (grid[int(cy) / ts][int(cx) / ts] == 1) { dy = -dy; cy += dy; }
        float oldAngle = angle;
        angle += angleSpeed;
        int newX = int(cx + std::cos(angle) * radius);
        int newY = int(cy + std::sin(angle) * radius);
        if (grid[newY / ts][newX / ts] == 1) {
            angle = oldAngle;
            angleSpeed = -angleSpeed;
            newX = int(cx + std::cos(angle) * radius);
            newY = int(cy + std::sin(angle) * radius);
        }
        x = std::clamp(newX, 0, N * ts - 1);
        y = std::clamp(newY, 0, M * ts - 1);
    }

    void move() {
        if (mode == MODE_LINEAR) moveLinear();
        else if (mode == MODE_ZIGZAG) moveZigZag();
        else if (mode == MODE_CIRCLE) moveCircle();
    }
};

// Flood-Fill Capture Function
void drop(int y, int x, int player) {
    if (y < 0 || y >= M || x < 0 || x >= N || grid[y][x] != 0 || trailOwner[y][x] != 0) return;
    grid[y][x] = -1; // Temporarily mark as visited
    trailOwner[y][x] = player; // Assign ownership to the player
    drop(y - 1, x, player);
    drop(y + 1, x, player);
    drop(y, x - 1, player);
    drop(y, x + 1, player);
}

// Animation Components
struct MenuAnimation {
    RectangleShape sky;
    std::vector<VertexArray> gridLines;
    std::vector<RectangleShape> stars;
    std::vector<Particle> particles;
    float elapsedTime;

    MenuAnimation(float width, float height) : elapsedTime(0.0f) {
        // Sky background
        sky.setSize(Vector2f(width, height));
        sky.setFillColor(Color(10, 10, 30));

        // Grid floor
        float gridSpacing = 30.0f;
        for (float x = 0; x < width; x += gridSpacing) {
            VertexArray line(Lines, 2);
            line[0].position = Vector2f(x, height / 2);
            line[1].position = Vector2f(x, height);
            line[0].color = Color(0, 255, 255, 100);
            line[1].color = Color(0, 255, 255, 0);
            gridLines.push_back(line);
        }
        for (float y = height / 2; y < height; y += gridSpacing) {
            VertexArray line(Lines, 2);
            line[0].position = Vector2f(0, y);
            line[1].position = Vector2f(width, y);
            line[0].color = Color(0, 255, 255, 100);
            line[1].color = Color(0, 255, 255, 0);
            gridLines.push_back(line);
        }

        // Stars
        for (int i = 0; i < 30; ++i) {
            RectangleShape star(Vector2f(2, 2));
            star.setPosition(rand() % int(width), rand() % int(height / 2));
            star.setFillColor(Color::White);
            stars.push_back(star);
        }
    }

    void update(float deltaTime, float width, float height, float alpha) {
        elapsedTime += deltaTime;

        // Add particles randomly
        if (rand() % 10 < 2) {
            float px = rand() % int(width);
            float py = rand() % int(height);
            float vx = (rand() % 100 - 50) / 25.0f;
            float vy = (rand() % 100 - 50) / 25.0f;
            Color pColor = (rand() % 2 == 0) ? Color(255, 0, 255) : Color(0, 255, 255);
            particles.emplace_back(px, py, vx, vy, 1.0f, pColor);
        }

        // Update particles
        for (auto it = particles.begin(); it != particles.end();) {
            it->lifetime -= 0.016f;
            it->position += it->velocity;
            if (it->lifetime <= 0 || it->position.x < 0 || it->position.x > width || 
                it->position.y < 0 || it->position.y > height) {
                it = particles.erase(it);
            } else {
                it->color.a = static_cast<Uint8>(it->lifetime * alpha);
                ++it;
            }
        }
    }

    void draw(RenderWindow& window, float alpha) {
        window.draw(sky);
        for (const auto& star : stars) {
            Color c = star.getFillColor();
            c.a = static_cast<Uint8>(alpha);
            RectangleShape s = star;
            s.setFillColor(c);
            window.draw(s);
        }
        for (const auto& line : gridLines) {
            VertexArray l = line;
            for (int i = 0; i < 2; ++i) {
                l[i].color.a = static_cast<Uint8>(l[i].color.a * alpha / 255);
            }
            window.draw(l);
        }
        for (const auto& p : particles) {
            CircleShape particle(3);
            particle.setPosition(p.position);
            particle.setFillColor(p.color);
            window.draw(particle);
        }
    }
};

// Transition Screen Function
void showTransitionScreen(RenderWindow& window, Font& font) {
    Text welcomeText;
    welcomeText.setFont(font);
    welcomeText.setString("Welcome to Xonic");
    welcomeText.setCharacterSize(50);
    welcomeText.setFillColor(Color::Magenta);
    welcomeText.setStyle(Text::Bold);
    FloatRect welcomeBounds = welcomeText.getLocalBounds();
    welcomeText.setOrigin(welcomeBounds.width / 2, welcomeBounds.height / 2);
    welcomeText.setPosition(N * ts / 2, M * ts / 2 - 50);

    Text createdByText;
    createdByText.setFont(font);
    createdByText.setString("made by W & A");
    createdByText.setCharacterSize(20);
    createdByText.setFillColor(Color::Cyan);
    FloatRect createdBounds = createdByText.getLocalBounds();
    createdByText.setOrigin(createdBounds.width, createdBounds.height);
    createdByText.setPosition(N * ts - 10, M * ts - 10);

    MenuAnimation anim(N * ts, M * ts);
    Clock transitionClock;
    float transitionDuration = 6.0f;
    float textZoomDuration = 1.5f;
    float createdByFadeStart = 2.0f;
    float fadeOutStart = 4.5f;

    while (window.isOpen() && transitionClock.getElapsedTime().asSeconds() < transitionDuration) {
        Event e;
        while (window.pollEvent(e)) {
            if (e.type == Event::Closed) window.close();
            if (e.type == Event::KeyPressed && (e.key.code == Keyboard::Enter || e.key.code == Keyboard::Space)) return;
        }

        float elapsed = transitionClock.getElapsedTime().asSeconds();
        float alpha = 255;
        float scale = 1.0f;

        if (elapsed < textZoomDuration) {
            scale = 0.5f + (elapsed / textZoomDuration) * 0.5f;
            alpha = (elapsed / textZoomDuration) * 255;
        } else if (elapsed >= fadeOutStart) {
            alpha = (transitionDuration - elapsed) / (transitionDuration - fadeOutStart) * 255;
        }

        welcomeText.setScale(scale, scale);
        welcomeText.setFillColor(Color(255, 0, 255, static_cast<Uint8>(alpha)));

        float createdByAlpha = 0;
        if (elapsed >= createdByFadeStart && elapsed < fadeOutStart) {
            createdByAlpha = ((elapsed - createdByFadeStart) / (fadeOutStart - createdByFadeStart)) * 255;
        } else if (elapsed >= fadeOutStart) {
            createdByAlpha = (transitionDuration - elapsed) / (transitionDuration - fadeOutStart) * 255;
        }
        createdByText.setFillColor(Color(0, 255, 255, static_cast<Uint8>(createdByAlpha)));

        anim.update(1.0f / 60.0f, N * ts, M * ts, alpha / 255.0f);

        window.clear();
        anim.draw(window, alpha);
        window.draw(welcomeText);
        window.draw(createdByText);
        window.display();
    }
}

void resetGame(int& x, int& y, int& dx, int& dy, bool& building,
    int& x2, int& y2, int& dx2, int& dy2, bool& building2,
    float& spawnTimer, bool& patternAssigned, Clock& patternClock,
    Clock& gameTimer, Enemy a[], int& enemyCount, bool continuousMode,
    int& powerUps, int& powerUps2, int& currentThreshold2, int& bonusCount2,
    int& nextPowerUp2, int& totalTiles2, int& totalRawTiles2, int tileCounts2[], 
    sf::Sound& captureSound, sf::Sound& trailSound, sf::Sound& bonusSound, sf::Sound& powerUpSound, 
    bool& wasBuilding1, bool& wasBuilding2)
{
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            grid[i][j] = (i == 0 || j == 0 || i == M - 1 || j == N - 1) ? 1 : 0;
            trailOwner[i][j] = 0;
        }
    }
    
    captureSound.stop();
    trailSound.stop();
    bonusSound.stop();
    powerUpSound.stop();
    wasBuilding1 = false;
    wasBuilding2 = false;

    x = 10;   y = 0;
    dx = 0;   dy = 0;
    building = false;
    drawCount1 = 0;
    moveCount1 = 0;
    tileCount1 = 0;
    totalTiles1 = 0;
    totalRawTiles = 0;
    for (int i = 0; i < 100; ++i) tileCounts1[i] = 0;
    bonusCount = 0;
    currentThreshold = 10;
    powerUps = 0;
    nextPowerUp = 50;
    player1Stopped = false;

    x2 = N - 1;   y2 = M - 1;
    dx2 = 0;      dy2 = 0;
    building2 = false;
    drawCount2 = 0;
    moveCount2 = 0;
    tileCount2 = 0;
    totalTiles2 = 0;
    totalRawTiles2 = 0;
    for (int i = 0; i < 100; ++i) tileCounts2[i] = 0;
    bonusCount2 = 0;
    currentThreshold2 = 10;
    powerUps2 = 0;
    nextPowerUp2 = 50;
    player2Stopped = false;

    totalTiles = 0;
    moveNumber = 0;
    drawCount = 0;
    enemyStopped = false;
    stopTimer = 0.0f;
    playerStopTimer = 0.0f;
    spawnTimer = 0.0f;
    patternAssigned = false;
    patternClock.restart();
    gameTimer.restart();

    enemyCount = continuousMode ? 2 : enemyCount;
    for (int i = 0; i < enemyCount; ++i) {
        a[i] = Enemy();
    }
}

int main() {
    srand(time(0));

    // Load sound buffers
    if (!trailBuffer.loadFromFile("capture.wav")) {
        cout << "Error loading trail.wav" << endl;
        return -1;
    }
    trailSound.setBuffer(trailBuffer);
    trailSound.setLoop(true);

    if (!captureBuffer.loadFromFile("capture.wav")) {
        cout << "Error loading capture.wav" << endl;
        return -1;
    }
    captureSound.setBuffer(captureBuffer);

    if (!bonusBuffer.loadFromFile("bonus.WAV")) {
        cout << "Error loading bonus.WAV" << endl;
        return -1;
    }
    bonusSound.setBuffer(bonusBuffer);

    if (!powerUpBuffer.loadFromFile("powerup.WAV")) {
        cout << "Error loading powerup.WAV" << endl;
        return -1;
    }
    powerUpSound.setBuffer(powerUpBuffer);

    // Initialize window
    RenderWindow window(VideoMode(N * ts, M * ts), "Xonix Game!");
    window.setFramerateLimit(60);

    // Load font
    Font font;
    if (!font.loadFromFile("arial.ttf")) {
        cout << "Error loading arial.ttf" << endl;
        return -1;
    }

    // Load textures
    Texture t1, t2, t3;
    if (!t1.loadFromFile("tiles.png") || !t2.loadFromFile("gameover.png") || !t3.loadFromFile("enemy.png")) {
        cout << "Error loading texture files" << endl;
        return -1;
    }

    Sprite sTile(t1), sGameover(t2), sEnemy(t3);
    sGameover.setPosition(20 , 20);
    sEnemy.setOrigin(20, 20);

    Texture gameBackgroundTexture, powerUpBackgroundTexture;
    if (!gameBackgroundTexture.loadFromFile("gameBackground.jpg") || !powerUpBackgroundTexture.loadFromFile("powerUpBackground.jpeg")) {
        cout << "Error loading background textures" << endl;
        return -1;
    }

    Sprite gameBackground;
    gameBackground.setTexture(gameBackgroundTexture);
    float winW = float(N * ts);
    float winH = float(M * ts);
    float bgTexW = float(gameBackgroundTexture.getSize().x);
    float bgTexH = float(gameBackgroundTexture.getSize().y);
    gameBackground.setScale(winW / bgTexW, winH / bgTexH);
    gameBackground.setPosition(0, 0);

    // Show transition screen
    showTransitionScreen(window, font);

    // Game variables
    Clock clock, patternClock, gameTimer;
    float deltaTime = 0.0f;
    float speedTimer = 0.0f, spawnTimer = 0.0f, patternTime = 0.0f;
    const float speedInterval = 20.0f, spawnInterval = 20.0f, patternStart = 30.0f;
    bool patternAssigned = false;

    const string playerModeItems[] = {"Single Player", "Two Players", "Exit"};
    const int playerModeCount = 3;
    int playerModeIndex = 0;
    bool twoPlayers = false;

    const string mainItems[] = {"Start Game", "Select Level", "Scoreboard", "Exit"};
    const int mainCount = 4;
    int mainIndex = 0;

    const string levelItems[] = {"Easy (2 enemies)", "Medium (4 enemies)", "Hard (6 enemies)", "Continuous Mode"};
    const int levelCount = 4;
    int levelIndex = 0;

    int enemyCount = 4;
    bool continuousMode = false;

    Enemy a[10];
    bool gameRunning = true;
    bool building = false;
    int x = 0, y = 0, dx = 0

, dy = 0;
    float timer = 0, delay = 0.07f;
    int x2 = N - 1, y2 = M - 1, dx2 = 0, dy2 = 0;
    bool building2 = false;

    // Initialize grid
    for (int i = 0; i < M; i++)
        for (int j = 0; j < N; j++)
            if (i == 0 || j == 0 || i == M - 1 || j == N - 1)
                grid[i][j] = 1;

    resetGame(x, y, dx, dy, building,
              x2, y2, dx2, dy2, building2,
              spawnTimer, patternAssigned, patternClock,
              gameTimer, a, enemyCount, continuousMode,
              powerUps, powerUps2, currentThreshold2, bonusCount2,
              nextPowerUp2, totalTiles2, totalRawTiles2, tileCounts2, 
              captureSound, trailSound, bonusSound, powerUpSound, wasBuilding1, wasBuilding2);

    // Menu animation
    MenuAnimation menuAnim(N * ts, M * ts);
    Clock menuClock;

    // Player mode selection
    bool playerModeSelected = false;
    while (window.isOpen() && !playerModeSelected) {
        float delta = clock.restart().asSeconds();
        menuAnim.update(delta, N * ts, M * ts, 1.0f);

        Event e;
        while (window.pollEvent(e)) {
            if (e.type == Event::Closed) return 0;
            if (e.type == Event::KeyPressed) {
                if (e.key.code == Keyboard::Up)
                    playerModeIndex = (playerModeIndex + playerModeCount - 1) % playerModeCount;
                if (e.key.code == Keyboard::Down)
                    playerModeIndex = (playerModeIndex + 1) % playerModeCount;
                if (e.key.code == Keyboard::Enter) {
                    switch (playerModeIndex) {
                    case 0: twoPlayers = false; playerModeSelected = true; break;
                    case 1: twoPlayers = true; playerModeSelected = true; break;
                    case 2: return 0;
                    }
                }
            }
        }

        window.clear();
        menuAnim.draw(window, 255);

        Text titleText;
        titleText.setFont(font);
        titleText.setString("Xonix Game");
        titleText.setCharacterSize(50);
        titleText.setFillColor(Color::Magenta);
        titleText.setStyle(Text::Bold);
        FloatRect titleBounds = titleText.getLocalBounds();
        titleText.setOrigin(titleBounds.width / 2, titleBounds.height / 2);
        titleText.setPosition(N * ts / 2, M * ts / 4);
        float titleScale = 1.0f + 0.1f * sin(menuClock.getElapsedTime().asSeconds());
        titleText.setScale(titleScale, titleScale);
        window.draw(titleText);

        Text menuText;
        menuText.setFont(font);
        menuText.setCharacterSize(30);
        for (int i = 0; i < playerModeCount; ++i) {
            menuText.setString((i == playerModeIndex ? "> " : "  ") + playerModeItems[i]);
            float alpha = (i == playerModeIndex) ? 255 : 150;
            menuText.setFillColor(Color(0, 255, 255, static_cast<Uint8>(alpha)));
            menuText.setPosition(250, 230 + i * 45);
            window.draw(menuText);
        }

        window.display();
    }

    // Main menu
    bool startGameSelected = false;
    menuClock.restart();
    while (window.isOpen() && !startGameSelected) {
        float delta = clock.restart().asSeconds();
        menuAnim.update(delta, N * ts, M * ts, 1.0f);

        Event e;
        while (window.pollEvent(e)) {
            if (e.type == Event::Closed) return 0;
            if (e.type == Event::KeyPressed) {
                if (e.key.code == Keyboard::Up)
                    mainIndex = (mainIndex + mainCount - 1) % mainCount;
                if (e.key.code == Keyboard::Down)
                    mainIndex = (mainIndex + 1) % mainCount;
                if (e.key.code == Keyboard::Enter) {
                    switch (mainIndex) {
                    case 0: startGameSelected = true; break;
                    case 1: {
                        bool levelDone = false;
                        menuClock.restart();
                        while (window.isOpen() && !levelDone) {
                            float deltaLevel = clock.restart().asSeconds();
                            menuAnim.update(deltaLevel, N * ts, M * ts, 1.0f);

                            Event ev;
                            while (window.pollEvent(ev)) {
                                if (ev.type == Event::Closed) return 0;
                                if (ev.type == Event::KeyPressed) {
                                    if (ev.key.code == Keyboard::Up)
                                        levelIndex = (levelIndex + levelCount - 1) % levelCount;
                                    if (ev.key.code == Keyboard::Down)
                                        levelIndex = (levelIndex + 1) % levelCount;
                                    if (ev.key.code == Keyboard::Enter) {
                                        switch (levelIndex) {
                                        case 0: enemyCount = 2; continuousMode = false; break;
                                        case 1: enemyCount = 4; continuousMode = false; break;
                                        case 2: enemyCount = 6; continuousMode = false; break;
                                        case 3: enemyCount = 2; continuousMode = true; break;
                                        }
                                        levelDone = true;
                                    }
                                    if (ev.key.code == Keyboard::Escape)
                                        levelDone = true;
                                }
                            }

                            window.clear();
                            menuAnim.draw(window, 255);

                            Text titleText;
                            titleText.setFont(font);
                            titleText.setString("Select Level");
                            titleText.setCharacterSize(50);
                            titleText.setFillColor(Color::Magenta);
                            titleText.setStyle(Text::Bold);
                            FloatRect titleBounds = titleText.getLocalBounds();
                            titleText.setOrigin(titleBounds.width / 2, titleBounds.height / 2);
                            titleText.setPosition(N * ts / 2, M * ts / 4);
                            float titleScale = 1.0f + 0.1f * sin(menuClock.getElapsedTime().asSeconds());
                            titleText.setScale(titleScale, titleScale);
                            window.draw(titleText);

                            Text menuText;
                            menuText.setFont(font);
                            menuText.setCharacterSize(30);
                            for (int i = 0; i < levelCount; ++i) {
                                menuText.setString((i == levelIndex ? "> " : "  ") + levelItems[i]);
                                float alpha = (i == levelIndex) ? 255 : 150;
                                menuText.setFillColor(Color(0, 255, 255, static_cast<Uint8>(alpha)));
                                menuText.setPosition(220, 230 + i * 45);
                                window.draw(menuText);
                            }

                            Text escText;
                            escText.setFont(font);
                            escText.setString("Press ESC to return");
                            escText.setCharacterSize(20);
                            escText.setFillColor(Color::Cyan);
                            escText.setPosition(220, M * ts - 50);
                            window.draw(escText);

                            window.display();
                        }
                        menuClock.restart();
                        break;
                    }
                    case 2: {
                        ScoreEntry topScores[10];
                        int topCount = 0;

                        ifstream in("scores.txt");
                        int score, time;
                        while (in >> score >> time && topCount < 5) {
                            topScores[topCount].score = score;
                            topScores[topCount].time = time;
                            topCount++;
                        }
                        in.close();

                        bool showScores = true;
                        menuClock.restart();
                        while (window.isOpen() && showScores) {
                            float deltaScore = clock.restart().asSeconds();
                            menuAnim.update(deltaScore, N * ts, M * ts, 1.0f);

                            Event ev;
                            while (window.pollEvent(ev)) {
                                if (ev.type == Event::Closed) return 0;
                                if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::Escape)
                                    showScores = false;
                            }

                            window.clear();
                            menuAnim.draw(window, 255);

                            Text titleText;
                            titleText.setFont(font);
                            titleText.setString("Top Scores");
                            titleText.setCharacterSize(50);
                            titleText.setFillColor(Color::Magenta);
                            titleText.setStyle(Text::Bold);
                            FloatRect titleBounds = titleText.getLocalBounds();
                            titleText.setOrigin(titleBounds.width / 2, titleBounds.height / 2);
                            titleText.setPosition(N * ts / 2, M * ts / 4);
                            float titleScale = 1.0f + 0.1f * sin(menuClock.getElapsedTime().asSeconds());
                            titleText.setScale(titleScale, titleScale);
                            window.draw(titleText);

                            Text menuText;
                            menuText.setFont(font);
                            menuText.setCharacterSize(30);
                            for (int i = 0; i < topCount; ++i) {
                                menuText.setString(to_string(i + 1) + ". " + 
                                                  to_string(topScores[i].score) + 
                                                  " (" + to_string(topScores[i].time) + "s)");
                                menuText.setFillColor(Color::Cyan);
                                menuText.setPosition(220, 230 + i * 45);
                                window.draw(menuText);
                            }

                            Text escText;
                            escText.setFont(font);
                            escText.setString("Press ESC to return");
                            escText.setCharacterSize(20);
                            escText.setFillColor(Color::Cyan);
                            escText.setPosition(220, M * ts - 50);
                            window.draw(escText);

                            window.display();
                        }
                        menuClock.restart();
                        break;
                    }
                    case 3: return 0;
                    }
                }
            }
        }

        window.clear();
        menuAnim.draw(window, 255);

        Text titleText;
        titleText.setFont(font);
        titleText.setString("Main Menu");
        titleText.setCharacterSize(50);
        titleText.setFillColor(Color::Magenta);
        titleText.setStyle(Text::Bold);
        FloatRect titleBounds = titleText.getLocalBounds();
        titleText.setOrigin(titleBounds.width / 2, titleBounds.height / 2);
        titleText.setPosition(N * ts / 2, M * ts / 4);
        float titleScale = 1.0f + 0.1f * sin(menuClock.getElapsedTime().asSeconds());
        titleText.setScale(titleScale, titleScale);
        window.draw(titleText);

        Text menuText;
        menuText.setFont(font);
        menuText.setCharacterSize(30);
        for (int i = 0; i < mainCount; ++i) {
            string label = mainItems[i];
            if (i == 0) {
                label = continuousMode ?
                    "Start Game (Continuous Mode)" :
                    "Start Game (" + to_string(enemyCount) + " enemies)";
            }
            menuText.setString((i == mainIndex ? "> " : "  ") + label);
            float alpha = (i == mainIndex) ? 255 : 150;
            menuText.setFillColor(Color(0, 255, 255, static_cast<Uint8>(alpha)));
            menuText.setPosition(220, 230 + i * 45);
            window.draw(menuText);
        }

        Text player1Stats;
        player1Stats.setFont(font);
        player1Stats.setCharacterSize(20);
        player1Stats.setFillColor(Color::Green);
        player1Stats.setPosition(10, M * ts - 60);
        player1Stats.setString(
            "P1 - Moves: " + to_string(moveCount1) +
            "\nTiles: " + to_string(tileCount1)
        );
        window.draw(player1Stats);

        if (twoPlayers) {
            Text player2Stats;
            player2Stats.setFont(font);
            player2Stats.setCharacterSize(20);
            player2Stats.setFillColor(Color::Green);
            player2Stats.setPosition(N * ts - 200, M * ts - 60);
            player2Stats.setString(
                "P2 - Moves: " + to_string(moveCount2) +
                "\nTiles: " + to_string(tileCount2)
            );
            window.draw(player2Stats);
        }

        window.display();
    }

    // Game loop
    while (window.isOpen()) {
        float elapsed = clock.getElapsedTime().asSeconds();
        clock.restart();
        timer += elapsed;
        patternTime = patternClock.getElapsedTime().asSeconds();

        speedTimer += elapsed;
        if (speedTimer >= speedInterval) {
            speedTimer -= speedInterval;
            for (int i = 0; i < enemyCount; i++) {
                a[i].dx += (a[i].dx > 0 ? 1 : -1);
                a[i].dy += (a[i].dy > 0 ? 1 : -1);
            }
        }

        if (patternTime >= patternStart && !patternAssigned) {
            for (int i = 0; i < enemyCount; i++) {
                if (i < enemyCount / 4) a[i].mode = MODE_ZIGZAG;
                else if (i < enemyCount / 2) a[i].mode = MODE_CIRCLE;
            }
            patternAssigned = true;
        }

        if (continuousMode) {
            spawnTimer += elapsed;
            if (spawnTimer >= spawnInterval) {
                spawnTimer -= spawnInterval;
                int canAdd = min(2, 10 - enemyCount);
                for (int i = 0; i < canAdd; ++i)
                    a[enemyCount + i] = Enemy();
                enemyCount += canAdd;
            }
        }

        Event e;
        while (window.pollEvent(e)) {
            if (e.type == Event::Closed) window.close();
            if (e.type == Event::KeyPressed) {
                if (e.key.code == Keyboard::Escape) {
                    for (int i = 1; i < M - 1; i++)
                        for (int j = 1; j < N - 1; j++)
                            grid[i][j] = 0;
                    x = 10; y = 0;
                    gameRunning = true;
                    building = false;
                    moveNumber = drawCount = totalTiles = 0;
                    player1Stopped = false;
                    player2Stopped = false;
                }
                if (e.key.code == Keyboard::X && powerUps > 0) {
                    powerUps--;
                    enemyStopped = true;
                    stopTimer = 3.0f;
                    powerUpSound.play();
                    if (twoPlayers) {
                        player2Stopped = true;
                        playerStopTimer = 3.0f;
                    }
                }
                if (twoPlayers && e.key.code == Keyboard::P && powerUps2 > 0) {
                    powerUps2--;
                    enemyStopped = true;
                    stopTimer = 3.0f;
                    powerUpSound.play();
                    player1Stopped = true;
                    playerStopTimer = 3.0f;
                }
            }
        }

        if (gameRunning) {
            if (!player1Stopped) {
                if (Keyboard::isKeyPressed(Keyboard::Left))  { dx = -1; dy = 0; }
                if (Keyboard::isKeyPressed(Keyboard::Right)) { dx = 1; dy = 0; }
                if (Keyboard::isKeyPressed(Keyboard::Up))    { dx = 0; dy = -1; }
                if (Keyboard::isKeyPressed(Keyboard::Down))  { dx = 0; dy = 1; }
            }
            if (twoPlayers && !player2Stopped) {
                if (Keyboard::isKeyPressed(Keyboard::A))  { dx2 = -1; dy2 = 0; }
                if (Keyboard::isKeyPressed(Keyboard::D))  { dx2 = 1; dy2 = 0; }
                if (Keyboard::isKeyPressed(Keyboard::W))  { dx2 = 0; dy2 = -1; }
                if (Keyboard::isKeyPressed(Keyboard::S))  { dx2 = 0; dy2 = 1; }
            }
        }

        if (timer > delay) {
            if (gameRunning) {
                if (!player1Stopped) {
                    int newX = x + dx;
                    int newY = y + dy;
                    newX = clamp(newX, 0, N - 1);
                    newY = clamp(newY, 0, M - 1);

                    int cell = grid[newY][newX];
                    int owner = trailOwner[newY][newX];
                    if (twoPlayers && cell == 2 && owner == 2) {
                        player1Stopped = true;
                        dx = dy = 0;
                    } else {
                        x = newX;
                        y = newY;

                        if (cell == 2 && owner == 1) {
                            gameRunning = false;
                        } else if (cell == 0) {
                            building = true;
                            grid[y][x] = 2;
                            trailOwner[y][x] = 1;
                            drawCount1++;
                            if (!wasBuilding1) {
                                trailSound.stop();
                                trailSound.play();
                                wasBuilding1 = true;
                            }
                        } else if (cell == 1 && building) {
                            building = false;
                            wasBuilding1 = false;
                            trailSound.stop();
                            // Start flood-fill from each enemy's position
                            for (int i = 0; i < enemyCount; ++i)
                                drop(a[i].y / ts, a[i].x / ts, 0); // Mark areas with enemies as neutral

                            // Convert non-marked areas to player's territory
                            int captureCount = 0;
                            for (int i = 1; i < M - 1; ++i) {
                                for (int j = 1; j < N - 1; ++j) {
                                    if (grid[i][j] == 0) { // Areas not reached by flood-fill
                                        grid[i][j] = 1; // Convert to player's territory
                                        trailOwner[i][j] = 1;
                                        captureCount++;
                                    } else if (grid[i][j] == -1) {
                                        grid[i][j] = 0; // Reset marked areas
                                        trailOwner[i][j] = 0;
                                    }
                                }
                            }

                            // Clear the trail
                            for (int i = 0; i < M; ++i) {
                                for (int j = 0; j < N; ++j) {
                                    if (grid[i][j] == 2 && trailOwner[i][j] == 1) {
                                        grid[i][j] = 1;
                                        trailOwner[i][j] = 1;
                                    }
                                }
                            }

                            int drawnAndFilled1 = drawCount1 + captureCount;
                            tileCount1 += drawnAndFilled1;
                            totalRawTiles += drawnAndFilled1;

                            int multiplier = 1;
                            if (drawnAndFilled1 > currentThreshold) {
                                ++bonusCount;
                                bonusSound.play();
                                if (bonusCount == 3) {
                                    currentThreshold = 5;
                                }
                            }
                            if (bonusCount >= 5 && drawnAndFilled1 > currentThreshold) {
                                multiplier = 4;
                            } else if (drawnAndFilled1 > currentThreshold) {
                                multiplier = 2;
                            }

                            int pointsThisMove = drawnAndFilled1 * multiplier;
                            tileCounts1[moveCount1] = pointsThisMove;
                            totalTiles1 += pointsThisMove;
                            moveCount1++;
                            captureSound.stop();
                            captureSound.play();

                            while (totalTiles1 >= nextPowerUp) {
                                ++powerUps;
                                if (nextPowerUp < 50) nextPowerUp = 50;
                                else if (nextPowerUp == 50) nextPowerUp = 70;
                                else if (nextPowerUp == 70) nextPowerUp = 100;
                                else if (nextPowerUp == 100) nextPowerUp = 130;
                                else nextPowerUp += 30;
                            }

                            ++moveNumber;
                            drawCount1 = 0;
                        }
                    }
                }

                if (twoPlayers && !player2Stopped) {
                    int newX2 = x2 + dx2;
                    int newY2 = y2 + dy2;
                    newX2 = clamp(newX2, 0, N - 1);
                    newY2 = clamp(newY2, 0, M - 1);

                    int cell2 = grid[newY2][newX2];
                    int owner2 = trailOwner[newY2][newX2];

                    if (cell2 == 2 && owner2 == 1) {
                        player2Stopped = true;
                        dx2 = dy2 = 0;
                    } else {
                        x2 = newX2;
                        y2 = newY2;

                        if (cell2 == 2 && owner2 == 2) {
                            gameRunning = false;
                        } else if (cell2 == 0) {
                            building2 = true;
                            grid[y2][x2] = 2;
                            trailOwner[y2][x2] = 2;
                            drawCount2++;
                            if (!wasBuilding2) {
                                trailSound.stop();
                                trailSound.play();
                                wasBuilding2 = true;
                            }
                        } else if (cell2 == 1 && building2) {
                            building2 = false;
                            wasBuilding2 = false;
                            trailSound.stop();
                            // Start flood-fill from each enemy's position
                            for (int i = 0; i < enemyCount; i++)
                                drop(a[i].y / ts, a[i].x / ts, 0); // Mark areas with enemies as neutral

                            // Convert non-marked areas to player's territory
                            int captureCount2 = 0;
                            for (int i = 1; i < M - 1; i++) {
                                for (int j = 1; j < N - 1; j++) {
                                    if (grid[i][j] == 0) { // Areas not reached by flood-fill
                                        grid[i][j] = 1; // Convert to player's territory
                                        trailOwner[i][j] = 2;
                                        captureCount2++;
                                    } else if (grid[i][j] == -1) {
                                        grid[i][j] = 0; // Reset marked areas
                                        trailOwner[i][j] = 0;
                                    }
                                }
                            }

                            // Clear the trail
                            for (int i = 0; i < M; i++) {
                                for (int j = 0; j < N; j++) {
                                    if (grid[i][j] == 2 && trailOwner[i][j] == 2) {
                                        grid[i][j] = 1;
                                        trailOwner[i][j] = 2;
                                    }
                                }
                            }

                            int drawnFilled2 = drawCount2 + captureCount2;
                            tileCount2 += drawnFilled2;
                            totalRawTiles2 += drawnFilled2;

                            int multiplier2 = 1;
                            if (drawnFilled2 > currentThreshold2) {
                                ++bonusCount2;
                                bonusSound.play();
                                if (bonusCount2 == 3) {
                                    currentThreshold2 = 5;
                                }
                            }
                            if (bonusCount2 >= 5 && drawnFilled2 > currentThreshold2) {
                                multiplier2 = 4;
                            } else if (drawnFilled2 > currentThreshold2) {
                                multiplier2 = 2;
                            }

                            int pointsThisMove2 = drawnFilled2 * multiplier2;
                            tileCounts2[moveCount2] = pointsThisMove2;
                            totalTiles2 += pointsThisMove2;
                            moveCount2++;
                            captureSound.stop();
                            captureSound.play();

                            while (totalTiles2 >= nextPowerUp2) {
                                ++powerUps2;
                                if (nextPowerUp2 < 50) nextPowerUp2 = 50;
                                else if (nextPowerUp2 == 50) nextPowerUp2 = 70;
                                else if (nextPowerUp2 == 70) nextPowerUp2 = 100;
                                else if (nextPowerUp2 == 100) nextPowerUp2 = 130;
                                else nextPowerUp2 += 30;
                            }

                            drawCount2 = 0;
                        }
                    }
                }
            }
            timer = 0;
        }

        if (enemyStopped) {
            stopTimer -= elapsed;
            if (stopTimer <= 0) enemyStopped = false;
        }

        if (playerStopTimer > 0) {
            playerStopTimer -= elapsed;
            if (playerStopTimer <= 0) {
                player1Stopped = false;
                player2Stopped = false;
            }
        }

        if (gameRunning && !enemyStopped) {
            for (int i = 0; i < enemyCount; i++)
                a[i].move();
        }

        for (int i = 0; i < enemyCount; i++) {
            int cx = a[i].x / ts, cy = a[i].y / ts;
            if (gameRunning && cx >= 0 && cx < N && cy >= 0 && cy < M && grid[cy][cx] == 2)
                gameRunning = false;
        }

        if (!gameRunning) {
            trailSound.stop();
        }

        window.clear();

        // Draw game background
        if (enemyStopped) {
            gameBackground.setTexture(powerUpBackgroundTexture);
        } else {
            gameBackground.setTexture(gameBackgroundTexture);
        }
        gameBackground.setScale(winW / bgTexW, winH / bgTexH);
        window.draw(gameBackground);

        for (int i = 0; i < M; i++) {
            for (int j = 0; j < N; j++) {
                int v = grid[i][j];
                if (v == 0) continue;
                sTile.setTextureRect(IntRect((v == 1 ? 0 : 54), 0, ts, ts));
                sTile.setPosition(j * ts, i * ts);
                window.draw(sTile);
            }
        }

        Text timeDisplay;
        timeDisplay.setFont(font);
        timeDisplay.setCharacterSize(25);
        timeDisplay.setFillColor(Color::Red);
        timeDisplay.setPosition(585, 20);
        float elapsedSeconds = gameTimer.getElapsedTime().asSeconds();
        int minutes = static_cast<int>(elapsedSeconds) / 60;
        int seconds = static_cast<int>(elapsedSeconds) % 60;
        timeDisplay.setString(
            "Time: " + 
            to_string(minutes) + ":" + 
            (seconds < 10 ? "0" : "") + to_string(seconds)
        );
        window.draw(timeDisplay);

        Text stopStatus;
        stopStatus.setFont(font);
        stopStatus.setCharacterSize(20);
        stopStatus.setFillColor(Color::Red);
        stopStatus.setPosition(15, 80);
        stopStatus.setString(enemyStopped ? "Enemies Stopped" : "");
        window.draw(stopStatus);

        sTile.setTextureRect(IntRect(36, 0, ts, ts));
        sTile.setPosition(x * ts, y * ts);
        window.draw(sTile);

        if (twoPlayers) {
            sTile.setTextureRect(IntRect(72, 0, ts, ts));
            sTile.setPosition(x2 * ts, y2 * ts);
            window.draw(sTile);
        }

        if (gameRunning) sEnemy.rotate(10);
        for (int i = 0; i < enemyCount; i++) {
            sEnemy.setPosition(a[i].x, a[i].y);
            window.draw(sEnemy);
        }

        Text p1FinalStats;
        p1FinalStats.setFont(font);
        p1FinalStats.setCharacterSize(20);
        p1FinalStats.setFillColor(Color::Yellow);
        p1FinalStats.setPosition(15, 20);
        p1FinalStats.setString(
            "Player 1 - Score: " + to_string(totalTiles1) +
            "  Moves: " + to_string(moveCount1) +
            "  Tiles: " + to_string(tileCount1) +
            "  Power: " + to_string(powerUps) +
            (player1Stopped ? " STOPPED" : "")
        );
        window.draw(p1FinalStats);

        if (twoPlayers) {
            Text p2FinalStats;
            p2FinalStats.setFont(font);
            p2FinalStats.setCharacterSize(20);
            p2FinalStats.setFillColor(Color::Yellow);
            p2FinalStats.setPosition(15, 40);
            p2FinalStats.setString(
                "Player 2 - Score: " + to_string(totalTiles2) +
                "  Moves: " + to_string(moveCount2) +
                "  Tiles: " + to_string(tileCount2) +
                "  Power: " + to_string(powerUps2) +
                (player2Stopped ? " STOPPED" : "")
            );
            window.draw(p2FinalStats);
        }

        if (!gameRunning) {
            float totalGameTime = gameTimer.getElapsedTime().asSeconds();
            int finalScore1 = 0, finalScore2 = 0;
            for (int i = 0; i < moveCount1; i++) finalScore1 += tileCounts1[i];
            for (int i = 0; i < moveCount2; i++) finalScore2 += tileCounts2[i];

            ScoreEntry topScores[6];
            int topCount = 0;

            ifstream in("scores.txt");
            int score, time;
            while (in >> score >> time && topCount < 5) {
                topScores[topCount].score = score;
                topScores[topCount].time = time;
                topCount++;
            }
            in.close();

            bool isHigh = (topCount < 5 || finalScore1 > topScores[topCount - 1].score);

            if (isHigh) {
                topScores[topCount].score = finalScore1;
                topScores[topCount].time = static_cast<int>(gameTimer.getElapsedTime().asSeconds());
                topCount++;

                for (int i = 0; i < topCount; ++i) {
                    for (int j = i + 1; j < topCount; ++j) {
                        if (topScores[j].score > topScores[i].score) {
                            ScoreEntry tmp = topScores[i];
                            topScores[i] = topScores[j];
                            topScores[j] = tmp;
                        }
                    }
                }

                if (topCount > 5) topCount = 5;

                ofstream out("scores.txt");
                for (int i = 0; i < topCount; ++i) {
                    out << topScores[i].score << " " << topScores[i].time << "\n";
                }
                out.close();
            }

            const string endItems[] = {"Restart", "Main Menu", "Exit Game"};
            const int endCount = 3;
            int endIndex = 0;
            bool inEndMenu = true;
            MenuAnimation endAnim(N * ts, M * ts);
            menuClock.restart();

            while (window.isOpen() && inEndMenu) {
                float delta = clock.restart().asSeconds();
                endAnim.update(delta, N * ts, M * ts, 1.0f);

                Event ev;
                while (window.pollEvent(ev)) {
                    if (ev.type == Event::Closed) return 0;
                    if (ev.type == Event::KeyPressed) {
                        if (ev.key.code == Keyboard::Up)
                            endIndex = (endIndex + endCount - 1) % endCount;
                        if (ev.key.code == Keyboard::Down)
                            endIndex = (endIndex + 1) % endCount;
                        if (ev.key.code == Keyboard::Enter) {
                            switch (endIndex) {
                            case 0:
                                resetGame(x, y, dx, dy, building,
                                          x2, y2, dx2, dy2, building2,
                                          spawnTimer, patternAssigned, patternClock,
                                          gameTimer, a, enemyCount, continuousMode,
                                          powerUps, powerUps2, currentThreshold2, bonusCount2,
                                          nextPowerUp2, totalTiles2, totalRawTiles2, tileCounts2,
                                          captureSound, trailSound, bonusSound, powerUpSound,
                                          wasBuilding1, wasBuilding2);
                                gameRunning = true;
                                player1Stopped = false;
                                player2Stopped = false;
                                inEndMenu = false;
                                break;
                            case 1: return main();
                            case 2: return 0;
                            }
                        }
                    }
                }

                window.clear();
                endAnim.draw(window, 255);

                Text titleText;
                titleText.setFont(font);
                titleText.setString("Game Over!");
                titleText.setCharacterSize(50);
                titleText.setFillColor(Color::Magenta);
                titleText.setStyle(Text::Bold);
                FloatRect titleBounds = titleText.getLocalBounds();
                titleText.setOrigin(titleBounds.width / 2, titleBounds.height / 2);
                titleText.setPosition(N * ts / 2, M * ts / 4);
                float titleScale = 1.0f + 0.1f * sin(menuClock.getElapsedTime().asSeconds());
                titleText.setScale(titleScale, titleScale);
                window.draw(titleText);

                if (twoPlayers) {
                    Text winnerText;
                    winnerText.setFont(font);
                    winnerText.setCharacterSize(30);
                    winnerText.setFillColor(Color::Cyan);
                    FloatRect winnerBounds = winnerText.getLocalBounds();
                    winnerText.setOrigin(winnerBounds.width / 2, winnerBounds.height / 2);
                    winnerText.setPosition(N * ts / 2, M * ts / 4 + 50);
                    if (finalScore1 > finalScore2) {
                        winnerText.setString("Player 1 Wins!");
                    } else if (finalScore2 > finalScore1) {
                        winnerText.setString("Player 2 Wins!");
                    } else {
                        winnerText.setString("It's a Tie!");
                    }
                    window.draw(winnerText);
                }

                Text p1EndText;
                p1EndText.setFont(font);
                p1EndText.setCharacterSize(20);
                p1EndText.setFillColor(Color::Cyan);
                p1EndText.setPosition(150, 190);
                p1EndText.setString(
                    "P1 - Moves: " + to_string(moveCount1) +
                    "  Tiles: " + to_string(tileCount1) +
                    "  Score: " + to_string(finalScore1)
                );
                window.draw(p1EndText);

                if (twoPlayers) {
                    Text p2EndText;
                    p2EndText.setFont(font);
                    p2EndText.setCharacterSize(20);
                    p2EndText.setFillColor(Color::Cyan);
                    p2EndText.setPosition(150, 220);
                    p2EndText.setString(
                        "P2 - Moves: " + to_string(moveCount2) +
                        "  Tiles: " + to_string(tileCount2) +
                        "  Score: " + to_string(finalScore2)
                    );
                    window.draw(p2EndText);
                }

                Text powerUpText;
                powerUpText.setFont(font);
                powerUpText.setCharacterSize(20);
                powerUpText.setFillColor(Color::Cyan);
                powerUpText.setPosition(50, 120);
                powerUpText.setString("P1 Power-Ups: " + to_string(powerUps));
                window.draw(powerUpText);

                if (twoPlayers) {
                    Text powerUpText2;
                    powerUpText2.setFont(font);
                    powerUpText2.setCharacterSize(20);
                    powerUpText2.setFillColor(Color::Cyan);
                    powerUpText2.setPosition(50, 150);
                    powerUpText2.setString("P2 Power-Ups: " + to_string(powerUps2));
                    window.draw(powerUpText2);
                }

                Text timeDisplay;
                timeDisplay.setFont(font);
                timeDisplay.setCharacterSize(20);
                timeDisplay.setFillColor(Color::Cyan);
                timeDisplay.setPosition(50, 90);
                timeDisplay.setString("Time: " + to_string(int(totalGameTime)) + " seconds");
                window.draw(timeDisplay);

                if (!twoPlayers) {
                    Text scoreDisplay;
                    scoreDisplay.setFont(font);
                    scoreDisplay.setCharacterSize(30);
                    scoreDisplay.setFillColor(Color::Green);
                    scoreDisplay.setPosition(N * ts / 2, M * ts / 4 + 50);
                    scoreDisplay.setString(isHigh ? "New High Score!" : "");
                    FloatRect scoreBounds = scoreDisplay.getLocalBounds();
                    scoreDisplay.setOrigin(scoreBounds.width / 2, scoreBounds.height / 2);
                    window.draw(scoreDisplay);
                }

                Text menuText;
                menuText.setFont(font);
                menuText.setCharacterSize(30);
                for (int i = 0; i < endCount; i++) {
                    menuText.setString((i == endIndex ? "> " : "  ") + endItems[i]);
                    float menuAlpha = (i == endIndex) ? 255 : 150;
                    menuText.setFillColor(Color(0, 255, 255, static_cast<Uint8>(menuAlpha)));
                    menuText.setPosition(150, 250 + i * 50);
                    window.draw(menuText);
                }

                window.display();
            }
        }

        window.display();
    }

    return 0;
}
