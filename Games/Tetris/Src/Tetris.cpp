#include "Tetris.hpp"

#include "Audio.hpp"
#include "Colors.hpp"
#include "font.hpp"
#include "Music.hpp"
#include "Point.hpp"
#include "Screen.hpp"

#include <algorithm>


extern "C" Game *crateGame() {
    static Game game
        = {[](GameInitInfo *initInfo) -> GameInitInfo * { return Tetris::getInstance().init(initInfo); },
           [](GameState *gameState) -> GameState * {
               Tetris::getInstance().update(gameState);
               return gameState;
           },
           [](GameState *gameState) -> GameState * {
               Tetris::getInstance().shutdown();
               return gameState;
           }};
    return &game;
}

extern "C" void destroyGame(Game *game) {
    // 静态对象，无需释放
    (void)game;
}

// ------------------------------------------------------------
// 单例
// ------------------------------------------------------------
Tetris &Tetris::getInstance() {
    static Tetris instance;
    return instance;
}

// ------------------------------------------------------------
// 初始化 / 重置
// ------------------------------------------------------------
GameInitInfo *Tetris::init(GameInitInfo *initInfo) {
    initInfo->screen.clear(BLACK);
    initInfo->fps = 60;
    initInfo->audio_bpm = bpm;
    AudioPlayer::getInstance().play(Korobeiniki);
    rng.seed(0x12345678);  // 固定种子，可改用 HAL_GetTick()
    reset(initInfo->screen);
    initInfo->init_statues = true;
    return initInfo;
}

void Tetris::reset(SCRN &screen) {
    // 清空游戏板
    for (int r = 0; r < GROUND_SIZE.y; ++r) {
        for (int c = 0; c < GROUND_SIZE.x; ++c) {
            fixedGrid[r][c] = BLACK;
            displayGrid[r][c] = BLACK;
        }
    }
    for (int r = 0; r < PREVIEW_SIZE; ++r)
        for (int c = 0; c < PREVIEW_SIZE; ++c) previewDisplay[r][c] = BLACK;

    fillBag();
    bagIndex = 0;
    generateBlock(next);
    spawnBlock();

    canHold = true;
    score = 0;
    level = 1;
    linesCleared = 0;
    dropCounter = 0;
    dropInterval = 60;
    gameOver = false;
    hold = Block();       // type = NONE
    screen.clear(BLACK);  // 清除屏幕所有旧内容
    drawBorder(screen);   // 绘制边界
    render(screen);       // 重新绘制游戏内容
}

void Tetris::shutdown() {
    // 无需清理
}

// ------------------------------------------------------------
// 方块生成
// ------------------------------------------------------------
void Tetris::fillBag() {
    std::shuffle(bag.begin(), bag.end(), rng);
}

void Tetris::generateBlock(Block &block) {
    if (bagIndex >= 7) {
        fillBag();
        bagIndex = 0;
    }
    Block::Type type = bag[bagIndex++];
    block = Block(type, Point(0, 0));
    block.state = Block::State::ROT_0;
}

void Tetris::spawnBlock() {
    current = next;
    current.pos = Point(3, 0);  // 起始列
    generateBlock(next);
    if (checkCollision(current)) gameOver = true;
    canHold = true;
    dropCounter = 0;
}

// ------------------------------------------------------------
// 碰撞检测
// ------------------------------------------------------------
bool Tetris::checkCollision(const Block &block) const {
    for (const Point &p : block) {
        int x = p.x;
        int y = p.y;
        if (x < 0 || x >= GROUND_SIZE.x || y < 0 || y >= GROUND_SIZE.y) return true;
        if (fixedGrid[y][x] != BLACK) return true;
    }
    return false;
}

// ------------------------------------------------------------
// 移动 / 旋转
// ------------------------------------------------------------
void Tetris::moveLeft() {
    Block tmp = current;
    tmp.moveLeft();
    if (!checkCollision(tmp)) current = tmp;
}

void Tetris::moveRight() {
    Block tmp = current;
    tmp.moveRight();
    if (!checkCollision(tmp)) current = tmp;
}

void Tetris::moveDown() {
    Block tmp = current;
    tmp.moveDown();
    if (!checkCollision(tmp)) {
        current = tmp;
        dropCounter = 0;
    } else {
        lockBlock();
    }
}

void Tetris::rotateLeft() {
    Block tmp = current;
    tmp.rotateLeft();
    if (!checkCollision(tmp)) current = tmp;
}

void Tetris::rotateRight() {
    Block tmp = current;
    tmp.rotateRight();
    if (!checkCollision(tmp)) current = tmp;
}

void Tetris::hardDrop() {
    while (true) {
        Block tmp = current;
        tmp.moveDown();
        if (checkCollision(tmp)) break;
        current = tmp;
    }
    lockBlock();
}

void Tetris::holdBlock() {
    if (!canHold || gameOver) return;
    if (hold.type == Block::Type::NONE) {
        hold = current;
        current = next;
        generateBlock(next);
    } else {
        std::swap(current, hold);
    }
    current.pos = Point(3, 0);
    if (checkCollision(current)) gameOver = true;
    canHold = false;
    dropCounter = 0;
}

// ------------------------------------------------------------
// 固定当前块 & 消行
// ------------------------------------------------------------
void Tetris::lockBlock() {
    RGB565 color = COLORS[static_cast<int>(current.type)];
    for (const Point &p : current) {
        int x = p.x;
        int y = p.y;
        if (x >= 0 && x < GROUND_SIZE.x && y >= 0 && y < GROUND_SIZE.y) fixedGrid[y][x] = color;
    }
    clearLines();
    spawnBlock();
}

void Tetris::clearLines() {
    int cleared = 0;
    for (int row = GROUND_SIZE.y - 1; row >= 0;) {
        bool full = true;
        for (int col = 0; col < GROUND_SIZE.x; ++col) {
            if (fixedGrid[row][col] == BLACK) {
                full = false;
                break;
            }
        }
        if (full) {
            // 上移
            for (int r = row; r > 0; --r)
                for (int c = 0; c < GROUND_SIZE.x; ++c) fixedGrid[r][c] = fixedGrid[r - 1][c];
            for (int c = 0; c < GROUND_SIZE.x; ++c) fixedGrid[0][c] = BLACK;
            ++cleared;
            // 继续检查同一行
        } else {
            --row;
        }
    }

    if (cleared > 0) {
        linesCleared += cleared;
        score += cleared * 100;
        level = 1 + linesCleared / 10;
        dropInterval = 60 - (level - 1) * 5;
        if (dropInterval < 10) dropInterval = 10;
    }
}

// ------------------------------------------------------------
// 增量绘制
// ------------------------------------------------------------
RGB565 Tetris::getGridColor(int row, int col) const {
    if (fixedGrid[row][col] != BLACK) return fixedGrid[row][col];
    for (const Point &p : current) {
        if (p.x == col && p.y == row) return COLORS[static_cast<int>(current.type)];
    }
    return BLACK;
}

void Tetris::drawCell(SCRN &screen, Point pos, RGB565 color) {
    Point p = GAME_OFFSET + pos * CELL_SIZE;
    screen.drawRect(p, CELL_SIZE - Point{1, 1}, color);
}

void Tetris::drawPreview(SCRN &screen) {
    for (Point::pType r = 0; r < PREVIEW_SIZE; r++) {
        for (Point::pType c = 0; c < PREVIEW_SIZE; c++) {
            RGB565 color = BLACK;
            for (const Point &p : next) {
                if (p.x == c && p.y == r) {
                    color = COLORS[static_cast<int>(next.type)];
                    break;
                }
            }
            if (color != previewCache[r][c]) {
                auto p = PREVIEW_OFFSET + Point{c, r} * PREVIEW_CELL_SIZE;
                screen.drawRect(p, Point{PREVIEW_CELL_SIZE - 1, PREVIEW_CELL_SIZE - 1}, color);
                previewCache[r][c] = color;
            }
        }
    }
}

void Tetris::render(SCRN &screen) {
    // 1. 游戏板增量绘制
    for (Point::pType row = 0; row < GROUND_SIZE.y; row++) {
        for (Point::pType col = 0; col < GROUND_SIZE.x; col++) {
            RGB565 color = getGridColor(row, col);
            if (color != displayGrid[row][col]) {
                drawCell(screen, Point{col, row}, color);
                displayGrid[row][col] = color;
            }
        }
    }

    // 2. 预览（简单重绘）
    drawPreview(screen);

    // 3. 游戏结束信息
    if (gameOver) {
        screen.drawString(Point{20, 80}, " GAME OVER ", WHITE, RED);
        screen.drawString(Point{20, 94}, "  PRESS A  ", WHITE, RED);
        screen.drawRect(Point{20, 108}, Point{3, 15}, RED);
        screen.drawString(Point{24, 108}, "TO RESTART", WHITE, RED);
        screen.drawRect(Point{24 + 10 * FONT_WIDTH, 108}, Point{3, 15}, RED);
    }

    screen.flush();
}

// ------------------------------------------------------------
// 主更新循环
// ------------------------------------------------------------
void Tetris::update(GameState *gameState) {
    // 读取当前按键状态
    bool upNow = gameState->up;
    bool downNow = gameState->down;
    bool leftNow = gameState->left;
    bool rightNow = gameState->right;
    bool aNow = gameState->a;

    // 计算上升沿（边沿）
    bool upEdge = upNow && !lastUp;
    bool downEdge = downNow && !lastDown;
    bool leftEdge = leftNow && !lastLeft;
    bool rightEdge = rightNow && !lastRight;
    bool aEdge = aNow && !lastA;

    // 更新历史状态
    lastUp = upNow;
    lastDown = downNow;
    lastLeft = leftNow;
    lastRight = rightNow;
    lastA = aNow;

    if (gameOver) {
        if (aEdge) {
            reset(gameState->screen);
        } else {
            render(gameState->screen);
        }
        return;
    }

    // 组合键：A + 方向
    if (gameState->a) {
        if (leftEdge) {
            rotateLeft();
        } else if (rightEdge) {
            rotateRight();
        } else if (downEdge) {
            hardDrop();
        }
    }

    // 单独按键（A 未按下时）
    if (leftEdge && !gameState->a) {
        moveLeft();
    }
    if (rightEdge && !gameState->a) {
        moveRight();
    }
    if (downEdge && !gameState->a) {
        moveDown();
    }
    if (upEdge) {
        holdBlock();
    }

    // 下落计时
    dropCounter++;
    if (dropCounter >= dropInterval) {
        dropCounter = 0;
        moveDown();  // 自动下落
    }

    render(gameState->screen);
}

void Tetris::drawBorder(SCRN &screen) {
    const RGB565 borderColor = RGB565(0x8410);  // 灰色
    auto top_left = GAME_OFFSET - Point{2, 0};
    auto bottom_right = GAME_OFFSET + GROUND_SIZE * CELL_SIZE - Point{0, 1};
    screen.drawRect(top_left, Point{1, bottom_right.y}, borderColor);
    screen.drawRect(bottom_right, Point{1, static_cast<Point::pType>(-bottom_right.y)}, borderColor);
}
