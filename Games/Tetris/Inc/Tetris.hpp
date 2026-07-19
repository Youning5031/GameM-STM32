#pragma once

#include "Block.hpp"
#include "Colors.hpp"
#include "GameM.h"
#include "Point.hpp"
#include "Xorshift.hpp"

class Tetris {
public:
    static Tetris &getInstance();

    GameInitInfo *init(GameInitInfo *initInfo);
    void update(GameState *gameState);
    void shutdown();

private:
    Tetris() = default;
    ~Tetris() = default;
    Tetris(const Tetris &) = delete;
    Tetris &operator=(const Tetris &) = delete;

    // 游戏板尺寸
    static constexpr Point GROUND_SIZE{10, 20};
    static constexpr Point CELL_SIZE{8, 8};     // 像素
    static constexpr Point GAME_OFFSET{24, 0};  // 居中
    static constexpr Point PREVIEW_OFFSET{110, 10};
    static constexpr Point::pType PREVIEW_CELL_SIZE = 4;
    static constexpr Point::pType PREVIEW_SIZE = 4;

    // 方块颜色
    static inline constexpr RGB565 COLORS[7] = {
        CYAN,    // I  青色
        YELLOW,  // O  黄色
        PURPLE,  // T  紫色
        GREEN,   // S  绿色
        RED,     // Z  红色
        BLUE,    // J  蓝色
        ORANGE   // L  橙色
    };

    Xorshift rng{GOLDEN_RATIO};

    RGB565 fixedGrid[GROUND_SIZE.y][GROUND_SIZE.x];     // 已固定的方块
    RGB565 displayGrid[GROUND_SIZE.y][GROUND_SIZE.x];   // 上次绘制的颜色（用于增量）
    RGB565 previewDisplay[PREVIEW_SIZE][PREVIEW_SIZE];  // 预览缓存
    RGB565 previewCache[PREVIEW_SIZE][PREVIEW_SIZE];    // 预览增量缓存

    std::array<Block::Type, 7> bag{Block::Type::I, Block::Type::O, Block::Type::T, Block::Type::S,
                                   Block::Type::Z, Block::Type::J, Block::Type::L};
    int bagIndex;

    Block current;
    Block next;
    Block hold;  // type == Type::NONE 表示空

    bool canHold = true;
    int score = 0;
    int level = 1;
    int linesCleared = 0;
    int dropCounter = 0;
    int dropInterval = 60;  // 帧数
    bool gameOver = false;

    bool lastUp = false;
    bool lastDown = false;
    bool lastLeft = false;
    bool lastRight = false;
    bool lastA = false;

    // 辅助函数
    void fillBag();
    void reset(SCRN &screen);
    void generateBlock(Block &block);
    void spawnBlock();
    bool checkCollision(const Block &block) const;
    void lockBlock();
    void clearLines();
    void moveLeft();
    void moveRight();
    void moveDown();
    void rotateLeft();
    void rotateRight();
    void hardDrop();
    void holdBlock();
    void drawBorder(SCRN &screen);  // 绘制边界

    RGB565 getGridColor(int row, int col) const;
    void drawCell(SCRN &screen, Point pos, RGB565 color);
    void drawPreview(SCRN &screen);
    void render(SCRN &screen);
};