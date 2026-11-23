#pragma once
#include <SDL3/SDL.h>
#include <atomic>
#include <mutex>
#include <cstdint>

enum class SfxType {
    Laser,
    EnemyLaser,
    LockPing,
    MissileLaunch,
    Explosion,
    BigExplosion,
    RingGate,
    BarrelRoll,
    Deflect,
    ShieldHit,
    HullHit,
    Upgrade,
    WarningBeep,
    HitMarker,
    WaterSplash,
    Scrape
};

class AudioEngine {
public:
    bool init();
    void shutdown();

    void playSfx(SfxType type, float volume = 0.7f, float pitchParam = 1.0f);
    void setFlightParams(float speedNorm, float boostAmt, bool inCombat, bool bossActive);
    void toggleMusic();
    void toggleMute();
    bool isMusicEnabled() const { return musicEnabled.load(std::memory_order_relaxed); }
    bool isMuted() const { return muted.load(std::memory_order_relaxed); }

private:
    struct Voice {
        bool active = false;
        uint32_t gen = 0;
        SfxType type = SfxType::Laser;
        float t = 0.0f;
        float dur = 0.2f;
        float vol = 0.5f;
        float pitch = 1.0f;
        float phase = 0.0f;
        float lp = 0.0f;
        uint32_t rng = 1;
    };

    static void SDLCALL audioCallback(void* userdata, SDL_AudioStream* stream, int additional, int total);
    void mixFrames(float* outStereo, int numFrames);

    SDL_AudioStream* stream = nullptr;
    std::mutex voiceMutex;
    static constexpr int MAX_VOICES = 24;
    Voice voices[MAX_VOICES];
    uint32_t nextVoiceGen = 1;

    std::atomic<float> engineSpeed{0.0f};
    std::atomic<float> engineBoost{0.0f};
    std::atomic<bool> combatMode{false};
    std::atomic<bool> bossMode{false};
    std::atomic<bool> musicEnabled{true};
    std::atomic<bool> muted{false};

    // Audio thread persistent synth state
    double musicTime = 0.0;
    float engPhase = 0.0f;
    float engNoiseLP = 0.0f;
    float bassLP = 0.0f;
    float padLP = 0.0f;
    float arpPhase = 0.0f;
    float bassPhase = 0.0f;
    float padPhases[6] = {};
    uint32_t audioRng = 0xCAFEBABEu;
    float mixBuf[2048] = {};
};
