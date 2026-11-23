#include "audio.hpp"
#include "math3d.hpp"
#include <cmath>
#include <algorithm>
#include <cstring>

static constexpr int SAMPLE_RATE = 44100;

static inline float fastNoise(uint32_t& s) {
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return (float(s & 0xFFFFu) / 32767.5f) - 1.0f;
}

static inline float midiToHz(float midi) {
    return 440.0f * std::pow(2.0f, (midi - 69.0f) / 12.0f);
}

bool AudioEngine::init() {
    SDL_AudioSpec spec{};
    spec.format = SDL_AUDIO_F32;
    spec.channels = 2;
    spec.freq = SAMPLE_RATE;

    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, audioCallback, this);
    if (!stream) {
        // Gracefully run silent if no audio hardware is present
        return false;
    }
    SDL_ResumeAudioStreamDevice(stream);
    return true;
}

void AudioEngine::shutdown() {
    if (stream) {
        SDL_DestroyAudioStream(stream);
        stream = nullptr;
    }
}

void AudioEngine::setFlightParams(float speedNorm, float boostAmt, bool inCombat, bool bossActive) {
    engineSpeed.store(clampf(speedNorm, 0.0f, 1.5f), std::memory_order_relaxed);
    engineBoost.store(clampf(boostAmt, 0.0f, 1.0f), std::memory_order_relaxed);
    combatMode.store(inCombat, std::memory_order_relaxed);
    bossMode.store(bossActive, std::memory_order_relaxed);
}

void AudioEngine::toggleMusic() {
    musicEnabled.store(!musicEnabled.load(std::memory_order_relaxed), std::memory_order_relaxed);
}

void AudioEngine::toggleMute() {
    muted.store(!muted.load(std::memory_order_relaxed), std::memory_order_relaxed);
}

void AudioEngine::playSfx(SfxType type, float volume, float pitchParam) {
    if (!stream || muted.load(std::memory_order_relaxed)) return;
    std::lock_guard<std::mutex> lock(voiceMutex);
    // Find inactive slot or oldest voice
    int slot = 0;
    float maxT = -1.0f;
    for (int i = 0; i < MAX_VOICES; ++i) {
        if (!voices[i].active) { slot = i; break; }
        if (voices[i].t > maxT) { maxT = voices[i].t; slot = i; }
    }
    Voice& v = voices[slot];
    v.active = true;
    v.gen = nextVoiceGen++;
    v.type = type;
    v.t = 0.0f;
    v.vol = volume;
    v.pitch = pitchParam;
    v.phase = 0.0f;
    v.lp = 0.0f;
    v.rng = 0x9E3779B9u ^ uint32_t(slot * 7919 + int(musicTime * 1000.0));
    switch (type) {
        case SfxType::Laser:         v.dur = 0.11f; break;
        case SfxType::EnemyLaser:    v.dur = 0.13f; break;
        case SfxType::LockPing:      v.dur = 0.14f; break;
        case SfxType::MissileLaunch: v.dur = 0.34f; break;
        case SfxType::Explosion:     v.dur = 0.55f; break;
        case SfxType::BigExplosion:  v.dur = 1.35f; break;
        case SfxType::RingGate:      v.dur = 0.65f; break;
        case SfxType::BarrelRoll:    v.dur = 0.28f; break;
        case SfxType::Deflect:       v.dur = 0.24f; break;
        case SfxType::ShieldHit:     v.dur = 0.18f; break;
        case SfxType::HullHit:       v.dur = 0.26f; break;
        case SfxType::Upgrade:       v.dur = 0.52f; break;
        case SfxType::WarningBeep:   v.dur = 0.12f; break;
        case SfxType::HitMarker:     v.dur = 0.055f; break;
        case SfxType::WaterSplash:   v.dur = 0.68f; break;
        case SfxType::Scrape:        v.dur = 0.36f; break;
    }
}

void SDLCALL AudioEngine::audioCallback(void* userdata, SDL_AudioStream* s, int additional, int /*total*/) {
    auto* self = static_cast<AudioEngine*>(userdata);
    int bytesRemaining = additional;
    constexpr int maxChunkFrames = 1024;
    while (bytesRemaining > 0) {
        int frames = std::min(maxChunkFrames, bytesRemaining / int(sizeof(float) * 2));
        if (frames <= 0) break;
        self->mixFrames(self->mixBuf, frames);
        SDL_PutAudioStreamData(s, self->mixBuf, frames * int(sizeof(float) * 2));
        bytesRemaining -= frames * int(sizeof(float) * 2);
    }
}

void AudioEngine::mixFrames(float* outStereo, int numFrames) {
    if (muted.load(std::memory_order_relaxed)) {
        std::memset(outStereo, 0, size_t(numFrames) * 2 * sizeof(float));
        return;
    }

    const float dt = 1.0f / float(SAMPLE_RATE);
    const float spd = engineSpeed.load(std::memory_order_relaxed);
    const float bst = engineBoost.load(std::memory_order_relaxed);
    const bool inCombat = combatMode.load(std::memory_order_relaxed);
    const bool isBoss = bossMode.load(std::memory_order_relaxed);
    const bool playMusic = musicEnabled.load(std::memory_order_relaxed);

    // Copy active voices quickly under lock
    Voice localVoices[MAX_VOICES];
    {
        std::lock_guard<std::mutex> lock(voiceMutex);
        for (int i = 0; i < MAX_VOICES; ++i) localVoices[i] = voices[i];
    }

    // Musical progression (A minor / aero-synthwave)
    static const float normalChords[4][3] = {
        {57, 60, 64}, // Am
        {53, 57, 60}, // F
        {60, 64, 67}, // C
        {55, 59, 62}  // G
    };
    static const float bossChords[4][3] = {
        {57, 60, 64}, // Am
        {55, 58, 62}, // Gm
        {53, 57, 60}, // F
        {52, 56, 59}  // E
    };
    static const float arpPattern[8] = {0, 7, 12, 15, 19, 15, 12, 7};

    const double bpm = isBoss ? 128.0 : 114.0;
    const double beatSec = 60.0 / bpm;

    for (int f = 0; f < numFrames; ++f) {
        float L = 0.0f, R = 0.0f;

        // 1. Subtle dynamic jet engine turbine hum + airflow rush
        if (spd > 0.02f) {
            float n = fastNoise(audioRng);
            float cutoff = clampf(0.03f + spd * 0.06f + bst * 0.14f, 0.02f, 0.35f);
            engNoiseLP += (n - engNoiseLP) * cutoff;
            float engFreq = 72.0f + spd * 58.0f + bst * 65.0f;
            engPhase += engFreq * dt;
            if (engPhase > 1.0f) engPhase -= 1.0f;
            float hum = std::sin(engPhase * TAU) * 0.45f + std::sin(engPhase * TAU * 2.0f) * 0.25f;
            float engSig = (engNoiseLP * 0.55f + hum * 0.22f) * (0.045f + spd * 0.04f + bst * 0.075f);
            L += engSig;
            R += engSig;
        }

        // 2. Procedural Dynamic Synthwave / Atmospheric Soundtrack
        if (playMusic) {
            double beatPos = musicTime / beatSec;
            int bar = int(beatPos / 4.0) % 4;
            double inBar = std::fmod(beatPos, 4.0);
            double sixteenth = std::fmod(beatPos * 4.0, 1.0);
            int step16 = int(beatPos * 4.0) % 8;

            const auto& chord = isBoss ? bossChords[bar] : normalChords[bar];
            float rootHz = midiToHz(chord[0] - 24.0f);

            // Lush detuned atmospheric stereo pad
            float padL = 0.0f, padR = 0.0f;
            for (int c = 0; c < 3; ++c) {
                float hzA = midiToHz(chord[c]) * 0.996f;
                float hzB = midiToHz(chord[c]) * 1.004f;
                padPhases[c * 2]     += hzA * dt; if (padPhases[c * 2] > 1.0f) padPhases[c * 2] -= 1.0f;
                padPhases[c * 2 + 1] += hzB * dt; if (padPhases[c * 2 + 1] > 1.0f) padPhases[c * 2 + 1] -= 1.0f;
                float sA = (padPhases[c * 2] * 2.0f - 1.0f);
                float sB = (padPhases[c * 2 + 1] * 2.0f - 1.0f);
                padL += sA * 0.7f + sB * 0.3f;
                padR += sB * 0.7f + sA * 0.3f;
            }
            float padCut = 0.025f + 0.018f * std::sin(float(musicTime) * 0.4f);
            padLP += ((padL + padR) * 0.5f - padLP) * padCut;
            float padVol = inCombat ? 0.042f : 0.055f;
            L += (padLP * 0.8f + padL * 0.03f) * padVol;
            R += (padLP * 0.8f + padR * 0.03f) * padVol;

            // Rolling 16th-note synthwave bassline in combat
            if (inCombat) {
                bassPhase += rootHz * dt;
                if (bassPhase > 1.0f) bassPhase -= 1.0f;
                float saw = bassPhase * 2.0f - 1.0f;
                float env = std::exp(-float(sixteenth) * 4.5f);
                float cut = 0.04f + env * (isBoss ? 0.24f : 0.16f);
                bassLP += (saw - bassLP) * cut;
                float bassSig = bassLP * (isBoss ? 0.13f : 0.095f) * (0.35f + 0.65f * env);
                L += bassSig;
                R += bassSig;

                // Soft punchy kick on beats 0, 1, 2, 3
                float bFrac = float(std::fmod(inBar, 1.0));
                if (bFrac < 0.18f) {
                    float kEnv = std::exp(-bFrac * 24.0f);
                    float kFreq = 46.0f + 95.0f * std::exp(-bFrac * 38.0f);
                    float kick = std::sin(bFrac * kFreq * TAU) * kEnv * (isBoss ? 0.14f : 0.09f);
                    L += kick;
                    R += kick;
                }
            }

            // Sparkling stereo pentatonic arpeggio pluck
            float arpHz = midiToHz(chord[0] + arpPattern[step16]);
            arpPhase += arpHz * dt;
            if (arpPhase > 1.0f) arpPhase -= 1.0f;
            float arpEnv = std::exp(-float(sixteenth) * 7.5f);
            float tri = 1.0f - 4.0f * std::abs(arpPhase - 0.5f);
            float arpSig = tri * arpEnv * (inCombat ? 0.045f : 0.032f);
            float pan = (step16 & 1) ? 0.68f : 0.32f;
            L += arpSig * (1.0f - pan);
            R += arpSig * pan;
        }
        musicTime += dt;

        // 3. Polyphonic SFX Synthesizer
        for (int i = 0; i < MAX_VOICES; ++i) {
            Voice& v = localVoices[i];
            if (!v.active) continue;
            v.t += dt;
            if (v.t >= v.dur) { v.active = false; continue; }
            float u = v.t / v.dur;
            float sig = 0.0f;

            switch (v.type) {
                case SfxType::Laser: {
                    float freq = lerpf(980.0f, 230.0f, std::pow(u, 0.55f)) * v.pitch;
                    v.phase += freq * dt;
                    float saw = std::fmod(v.phase, 1.0f) * 2.0f - 1.0f;
                    float env = (1.0f - u) * (1.0f - u);
                    sig = (saw * 0.55f + std::sin(v.phase * TAU) * 0.45f) * env * 0.24f;
                    break;
                }
                case SfxType::EnemyLaser: {
                    float freq = lerpf(540.0f, 160.0f, u) * v.pitch;
                    v.phase += freq * dt;
                    float sq = (std::fmod(v.phase, 1.0f) < 0.5f ? 1.0f : -1.0f);
                    sig = sq * (1.0f - u) * 0.14f;
                    break;
                }
                case SfxType::LockPing: {
                    float freq = 660.0f * v.pitch;
                    v.phase += freq * dt;
                    float env = std::sin(clampf(u * 12.0f, 0.0f, 1.0f) * PI * 0.5f) * std::exp(-u * 6.5f);
                    sig = (std::sin(v.phase * TAU) + 0.35f * std::sin(v.phase * TAU * 2.0f)) * env * 0.22f;
                    break;
                }
                case SfxType::MissileLaunch: {
                    float n = fastNoise(v.rng);
                    v.lp += (n - v.lp) * (0.28f - u * 0.18f);
                    float freq = lerpf(320.0f, 95.0f, u);
                    v.phase += freq * dt;
                    float env = std::sin(clampf(u * 8.0f, 0.0f, 1.0f) * PI * 0.5f) * (1.0f - u);
                    sig = (v.lp * 0.75f + std::sin(v.phase * TAU) * 0.35f) * env * 0.30f;
                    break;
                }
                case SfxType::Explosion:
                case SfxType::BigExplosion: {
                    bool big = (v.type == SfxType::BigExplosion);
                    float n = fastNoise(v.rng);
                    v.lp += (n - v.lp) * (big ? 0.09f : 0.15f);
                    float subF = lerpf(big ? 92.0f : 125.0f, 26.0f, std::pow(u, 0.45f));
                    v.phase += subF * dt;
                    float env = std::exp(-u * (big ? 2.8f : 4.6f));
                    sig = (v.lp * 0.78f + std::sin(v.phase * TAU) * 0.65f) * env * (big ? 0.52f : 0.36f);
                    break;
                }
                case SfxType::RingGate: {
                    // Crystalline ascending triad chord!
                    static const float mults[4] = {1.0f, 1.25f, 1.5f, 2.0f};
                    int idx = std::min(3, int(u * 5.0f));
                    float base = 523.25f * v.pitch;
                    float ssum = 0.0f;
                    for (int k = 0; k <= idx; ++k) {
                        ssum += std::sin(v.t * base * mults[k] * TAU) * 0.25f;
                    }
                    float env = std::exp(-u * 3.2f);
                    sig = ssum * env * 0.32f;
                    break;
                }
                case SfxType::BarrelRoll: {
                    float n = fastNoise(v.rng);
                    float band = std::sin(u * PI);
                    v.lp += (n - v.lp) * (0.08f + band * 0.22f);
                    sig = v.lp * band * 0.26f;
                    break;
                }
                case SfxType::Deflect: {
                    float f1 = lerpf(1400.0f, 680.0f, u);
                    float fm = std::sin(v.t * 430.0f * TAU) * 2.5f;
                    sig = std::sin(v.t * f1 * TAU + fm) * std::exp(-u * 8.0f) * 0.28f;
                    break;
                }
                case SfxType::ShieldHit: {
                    float freq = lerpf(420.0f, 180.0f, u);
                    v.phase += freq * dt;
                    float tri = 1.0f - 4.0f * std::abs(std::fmod(v.phase, 1.0f) - 0.5f);
                    sig = tri * std::exp(-u * 7.5f) * 0.30f;
                    break;
                }
                case SfxType::HullHit: {
                    float n = fastNoise(v.rng);
                    v.lp += (n - v.lp) * 0.14f;
                    v.phase += lerpf(160.0f, 45.0f, u) * dt;
                    sig = (v.lp * 0.6f + std::sin(v.phase * TAU) * 0.7f) * std::exp(-u * 6.5f) * 0.38f;
                    break;
                }
                case SfxType::Upgrade: {
                    static const float notes[3] = {523.25f, 659.25f, 783.99f};
                    int ni = std::min(2, int(u * 3.0f));
                    float lu = std::fmod(u * 3.0f, 1.0f);
                    sig = (std::sin(v.t * notes[ni] * TAU) + 0.3f * std::sin(v.t * notes[ni] * 2.0f * TAU))
                        * std::exp(-lu * 4.0f) * 0.25f;
                    break;
                }
                case SfxType::WarningBeep: {
                    float freq = 920.0f * v.pitch;
                    v.phase += freq * dt;
                    float env = std::sin(clampf(u * 10.0f, 0.0f, 1.0f) * PI * 0.5f) * (1.0f - u);
                    float sq = std::sin(v.phase * TAU) + 0.25f * std::sin(v.phase * TAU * 3.0f);
                    sig = sq * env * 0.16f;
                    break;
                }
                case SfxType::HitMarker: {
                    float freq = lerpf(1650.0f, 1050.0f, u) * v.pitch;
                    v.phase += freq * dt;
                    sig = std::sin(v.phase * TAU) * (1.0f - u) * (1.0f - u) * 0.16f;
                    break;
                }
                case SfxType::WaterSplash: {
                    float n = fastNoise(v.rng);
                    float cut = lerpf(0.32f, 0.06f, u);
                    v.lp += (n - v.lp) * cut;
                    v.phase += lerpf(110.0f, 38.0f, u) * dt;
                    float env = std::sin(clampf(u * 12.0f, 0.0f, 1.0f) * PI * 0.5f) * std::exp(-u * 3.2f);
                    sig = (v.lp * 0.85f + std::sin(v.phase * TAU) * 0.35f) * env * 0.45f;
                    break;
                }
                case SfxType::Scrape: {
                    float n = fastNoise(v.rng);
                    v.lp += (n - v.lp) * 0.24f;
                    float screech = std::sin(v.t * lerpf(620.0f, 240.0f, u) * TAU + v.lp * 3.5f);
                    float thud = std::sin(v.t * lerpf(135.0f, 42.0f, u) * TAU);
                    sig = (v.lp * 0.55f + screech * 0.35f + thud * 0.55f) * std::exp(-u * 5.2f) * 0.44f;
                    break;
                }
            }
            float sVal = sig * v.vol;
            L += sVal;
            R += sVal;
        }

        // Soft limiter to prevent any harsh clipping
        outStereo[f * 2]     = std::tanh(L * 0.95f);
        outStereo[f * 2 + 1] = std::tanh(R * 0.95f);
    }

    // Write back updated voice states safely using generation counter (never overwrite newly spawned voices)
    {
        std::lock_guard<std::mutex> lock(voiceMutex);
        for (int i = 0; i < MAX_VOICES; ++i) {
            if (voices[i].gen == localVoices[i].gen) {
                voices[i] = localVoices[i];
            }
        }
    }
}
