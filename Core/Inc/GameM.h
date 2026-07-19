#pragma once

#ifdef __cplusplus
#  include "Button.hpp"
#  include "Screen.hpp"
#  include "ST7735S.hpp"


using DT = DisplayTraits<RGB565, 128, 160, 1024>;
inline DT::BufferType buffer;
using SCRN = Screen<ST7735S<DT, buffer>>;

struct GameInitInfo {
    SCRN &screen;

    uint32_t fps;
    uint32_t audio_bpm;
    bool init_statues;
};

using ButtonState = bool;

struct GameState {
    SCRN &screen;
    Button &up;
    Button &down;
    Button &left;
    Button &right;
    Button &a;
    Button &b;
};

#else
typedef struct {
    void *screen;

    uint32_t fps;
    uint32_t audio_bpm;
    bool init_statues;
} GameInitInfo;
typedef struct GameState GameState;
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    GameInitInfo *(*init)(GameInitInfo *);
    GameState *(*update)(GameState *);
    GameState *(*shutdown)(GameState *);
} Game;

__attribute__((weak)) Game *crateGame();
__attribute__((weak)) void destroyGame(Game *game);

GameInitInfo *getGameInitInfoPointer();
GameState *getGameStatePointer();

GameInitInfo *initGameInitInfo(GameInitInfo *);
GameState *initGameState(GameState *);
void updateGameState(GameState *gameState);
bool initAudio(GameInitInfo *);
void updateAudio(GameState *);

#ifdef __cplusplus
}
#endif
