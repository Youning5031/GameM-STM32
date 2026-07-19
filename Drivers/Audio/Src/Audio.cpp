#include "Audio.hpp"

#include "extime.h"
#include "GameM.h"
#include "main.h"
#include "stm32f1xx_hal_tim.h"
#include "tim.h"

#include <cstdint>

void AudioPlayer::update(GameState *game_state) {
    if (audio.empty()) return;
    if (requset_to != state) switch (requset_to) {
        case AudioState::IDLE: transitionToIdle(); break;
        case AudioState::PAUSED: transitionToPauesd(); break;
        case AudioState::PLAYING: transitionToPlaying(); break;
        }
    if (AudioState::PLAYING != state) return;
    doPlay();
}

void AudioPlayer::doPlay() {
    std::int64_t elapsed = timeMicros() - current_note_start;

    if (elapsed < current_note->duration) {
        // 播放音高
        __HAL_TIM_SET_COMPARE(&audio_tim, AUDIO_CHANNEL, current_note->pitch.ccr);
    } else if (elapsed <current_note->duration+ current_note->gate) {  // 间隔
        __HAL_TIM_SET_COMPARE(&audio_tim, AUDIO_CHANNEL, 0);
    } else  {  // 下一个
        current_note = &*(note_it++);
        if(note_it==audio.end()) note_it = audio.begin();
        __HAL_TIM_SET_COMPARE(&audio_tim, AUDIO_CHANNEL, current_note->pitch.ccr);
        __HAL_TIM_SET_AUTORELOAD(&audio_tim, current_note->pitch.arr);
        current_note_start = timeMicros();
    }
    // if (time_diff < 8000) { // 渐强
    // } else if (time_diff < note_it->duration - 8000) { // 正常播放
    // } else if (time_diff < note_it->duration) { //减弱
    // } else if (time_diff < note_it->duration - +note_it->gate) { // 间隔
    // } else { // 下一个音符
    // }
}

void AudioPlayer::transitionToIdle() {
    audio = Audio{};
    note_it = audio.begin();
    current_note=nullptr;
    HAL_TIM_PWM_Stop(&audio_tim, AUDIO_CHANNEL);
    state = AudioState::IDLE;
}
void AudioPlayer::transitionToPauesd() {
    if (state != AudioState::PLAYING) return;
    (void)__HAL_TIM_SET_COMPARE(&audio_tim, AUDIO_CHANNEL, 0);
    state = AudioState::PAUSED;
}
void AudioPlayer::transitionToPlaying() {
    if (state == AudioState::IDLE) {
        HAL_TIM_PWM_Start(&audio_tim, AUDIO_CHANNEL);
        (void)__HAL_TIM_SET_COMPARE(&audio_tim, AUDIO_CHANNEL, 0);
    }
    state = AudioState::PLAYING;
}
