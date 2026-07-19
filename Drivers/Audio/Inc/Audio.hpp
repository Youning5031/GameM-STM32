#pragma once

#include "GameM.h"

#include <cstdint>
#include <limits>
#include <span>


struct Pitch {
    const std::uint16_t arr;
    const std::uint16_t ccr;
    consteval Pitch(std::uint16_t arr, std::uint16_t ccr) : arr(arr), ccr(ccr) {}
};

constexpr inline Pitch O{std::numeric_limits<std::uint16_t>::max(), 0};
constexpr inline Pitch C4{34399, 17200};
constexpr inline Pitch C4_Sharp{32469, 16235};
constexpr inline Pitch D4{30647, 15324};
constexpr inline Pitch D4_Sharp{28926, 14464};
constexpr inline Pitch E4{27302, 13652};
constexpr inline Pitch F4{25770, 12886};
constexpr inline Pitch F4_Sharp{24324, 12163};
constexpr inline Pitch G4{22958, 11480};
constexpr inline Pitch G4_Sharp{21670, 10836};
constexpr inline Pitch A4{20454, 10228};
constexpr inline Pitch A4_Sharp{19306, 9654};
constexpr inline Pitch B4{18222, 9112};
constexpr inline Pitch C5{17199, 8600};
constexpr inline Pitch C5_Sharp{16234, 8118};
constexpr inline Pitch D5{15323, 7662};
constexpr inline Pitch D5_Sharp{14463, 7232};
constexpr inline Pitch E5{13651, 6826};
constexpr inline Pitch F5{12884, 6443};
constexpr inline Pitch F5_Sharp{12161, 6081};
constexpr inline Pitch G5{11479, 5740};
constexpr inline Pitch G5_Sharp{10834, 5418};
constexpr inline Pitch A5{10226, 5114};
constexpr inline Pitch A5_Sharp{9652, 4827};
constexpr inline Pitch B5{9110, 4556};

struct Note {
    const Pitch pitch;
    const std::uint32_t duration;
    const std::uint32_t gate;

    consteval Note(Pitch pitch, std::uint32_t meter_duration)
      : pitch(pitch), duration(static_cast<std::uint32_t>(meter_duration * 0.8)), gate(meter_duration - duration) {}
};

enum class AudioState { IDLE, PLAYING, PAUSED };

class AudioPlayer {
    friend bool initAudio(GameInitInfo *);
    friend void updateAudio(GameState *);

public:
    using Audio = std::span<const Note>;

private:
    Audio audio;
    using NoteIterator = decltype(audio.begin());
    AudioState state = AudioState::IDLE;
    AudioState requset_to = AudioState::IDLE;
    NoteIterator note_it = audio.begin();
    const Note *current_note=nullptr;
    std::int64_t current_note_start = 0;

    AudioPlayer() =default;

    bool init(GameInitInfo *game_init_info);
    void update(GameState *game_state);
    void playAudio();
    void stopAudio();

    void transitionToIdle();
    void transitionToPauesd();
    void transitionToPlaying();
    void transitionToLoading();
    void doPlay();

public:
    AudioPlayer(const AudioPlayer &) = delete;
    AudioPlayer &operator=(const AudioPlayer &) = delete;
    AudioPlayer(AudioPlayer &&) = delete;
    AudioPlayer &operator=(AudioPlayer &&) = delete;
    static AudioPlayer &getInstance() {
        static AudioPlayer instance;
        return instance;
    }

    void pause() {
        requset_to = AudioState::PAUSED;
    }
    void stop() {
        requset_to = AudioState::IDLE;
    }
    void play() {
        requset_to = AudioState::PLAYING;
    }
    void play(const Audio &_audio) {
        requset_to = AudioState::PLAYING;
        audio = _audio;
        note_it = audio.begin();
        current_note=&*(note_it++);
    }

    void restart() {
        note_it = audio.begin();
        current_note=nullptr;
    };
};