#include "GameM.h"

#include "Button.hpp"
#include "Audio.hpp"
#include "main.h"


static SCRN screen;

GameInitInfo *getGameInitInfoPointer() {
    static GameInitInfo game_init_info{screen};
    return &game_init_info;
}
GameState *getGameStatePointer() {
    static GameState game_state{screen, BUTTON(up), BUTTON(down), BUTTON(left), BUTTON(right), BUTTON(a), BUTTON(b)};
    return &game_state;
}

GameInitInfo *initGameInitInfo(GameInitInfo *game_init_info) {
    game_init_info->screen.init();
    return game_init_info;
}

GameState *initGameState(GameState *game_state) {
    return game_state;
}

void updateGameState(GameState *game_state) {}

void updateAudio(GameState *game_state){
    AudioPlayer::getInstance().update(game_state);
}