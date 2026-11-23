#include "game.hpp"
#include <SDL3/SDL.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

static void printMemoryStats() {
    std::ifstream f("/proc/self/status");
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("VmRSS:", 0) == 0 || line.rfind("VmHWM:", 0) == 0) {
            std::printf("  %s\n", line.c_str());
        }
    }
}

static bool saveScreenshotPPM(const char* path, int w, int h) {
    std::vector<uint8_t> pixels(size_t(w) * size_t(h) * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    FILE* fp = std::fopen(path, "wb");
    if (!fp) return false;
    std::fprintf(fp, "P6\n%d %d\n255\n", w, h);
    // OpenGL origin is bottom-left; flip rows for top-down PPM
    for (int y = h - 1; y >= 0; --y) {
        std::fwrite(pixels.data() + size_t(y) * size_t(w) * 3, 1, size_t(w) * 3, fp);
    }
    std::fclose(fp);
    return true;
}

int main(int argc, char** argv) {
    int winW = 1280, winH = 720;
    int benchmarkFrames = 0;
    std::string screenshotPath;
    int screenshotSector = 0;
    bool screenshotBoss = false;
    bool qualityOverridden = false;
    QualityPreset forcedQuality = QualityPreset::Balanced;
    bool fullscreen = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--benchmark" && i + 1 < argc) {
            benchmarkFrames = std::atoi(argv[++i]);
        } else if (arg == "--screenshot" && i + 1 < argc) {
            screenshotPath = argv[++i];
        } else if (arg == "--sector" && i + 1 < argc) {
            screenshotSector = std::atoi(argv[++i]);
        } else if (arg == "--boss") {
            screenshotBoss = true;
        } else if (arg == "--quality" && i + 1 < argc) {
            std::string q = argv[++i];
            qualityOverridden = true;
            if (q == "fast") forcedQuality = QualityPreset::Fast;
            else if (q == "high") forcedQuality = QualityPreset::High;
            else forcedQuality = QualityPreset::Balanced;
        } else if (arg == "--res" && i + 1 < argc) {
            std::sscanf(argv[++i], "%dx%d", &winW, &winH);
        } else if (arg == "--fullscreen") {
            fullscreen = true;
        } else if (arg == "--help" || arg == "-h") {
            std::printf("AETHERWING - 3D Archipelago Starfighter\n"
                        "Usage: %s [options]\n"
                        "  --res WxH              Window resolution (default 1280x720)\n"
                        "  --quality fast|balanced|high\n"
                        "  --fullscreen           Start fullscreen\n"
                        "  --benchmark N          Run N frames offscreen/windowed and report FPS & RAM\n"
                        "  --screenshot file.ppm  Render a frame and write PPM image\n"
                        "  --boss --sector 0|1|2  Screenshot/demo scene options\n", argv[0]);
            return 0;
        }
    }

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        // Fallback to video-only if audio subsystem unavailable
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
            return 1;
        }
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    if (fullscreen) flags |= SDL_WINDOW_FULLSCREEN;
    if (benchmarkFrames > 0 || !screenshotPath.empty()) flags |= SDL_WINDOW_HIDDEN;

    SDL_Window* win = SDL_CreateWindow("AETHERWING :: Skybound Archipelago Starfighter", winW, winH, flags);
    if (!win) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext ctx = SDL_GL_CreateContext(win);
    if (!ctx) {
        std::fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    if (!glfn::loadAll()) {
        std::fprintf(stderr, "Failed to load required OpenGL 3.3 Core functions.\n");
        SDL_GL_DestroyContext(ctx);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    const char* glVendor   = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const char* glRenderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    const char* glVersion  = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    std::string rendStr = glRenderer ? glRenderer : "";
    bool isSoftware = (rendStr.find("llvmpipe") != std::string::npos ||
                       rendStr.find("swrast")   != std::string::npos ||
                       rendStr.find("softpipe") != std::string::npos);

    QualityPreset initQ = qualityOverridden ? forcedQuality : (isSoftware ? QualityPreset::Fast : QualityPreset::High);

    std::printf("[AETHERWING] OpenGL: %s | %s | %s\n",
                glVendor ? glVendor : "?", glRenderer ? glRenderer : "?", glVersion ? glVersion : "?");
    std::printf("[AETHERWING] Mode: %s renderer -> starting at %s quality (press F1 to cycle)\n",
                isSoftware ? "Software (llvmpipe)" : "Hardware GPU",
                initQ == QualityPreset::High ? "HIGH" : (initQ == QualityPreset::Balanced ? "BALANCED" : "FAST"));

    // VSync on for interactive play, off for benchmark
    SDL_GL_SetSwapInterval(benchmarkFrames > 0 ? 0 : 1);

    int pixW = winW, pixH = winH;
    SDL_GetWindowSizeInPixels(win, &pixW, &pixH);

    Game game;
    if (!game.init(pixW, pixH, isSoftware, initQ)) {
        std::fprintf(stderr, "Game initialization failed!\n");
        SDL_GL_DestroyContext(ctx);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    // Automated Screenshot Mode
    if (!screenshotPath.empty()) {
        if (screenshotBoss) game.forceBossDemo(screenshotSector);
        else game.setTitleDemoCamera(screenshotSector);
        for (int i = 0; i < 12; ++i) game.update(1.0f / 60.0f);
        game.render();
        glFinish();
        GLenum err = glGetError();
        bool ok = saveScreenshotPPM(screenshotPath.c_str(), pixW, pixH);
        std::printf("[SCREENSHOT] Saved %s (%dx%d) ok=%d glError=0x%X\n",
                    screenshotPath.c_str(), pixW, pixH, int(ok), unsigned(err));
        printMemoryStats();
        game.destroy();
        SDL_GL_DestroyContext(ctx);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return (ok && err == GL_NO_ERROR) ? 0 : 1;
    }

    // Automated Benchmark / Self-Test Mode
    if (benchmarkFrames > 0) {
        game.forceBossDemo(screenshotSector);
        uint64_t freq = SDL_GetPerformanceFrequency();
        // Warmup 5 frames
        for (int i = 0; i < 5; ++i) { game.update(1.0f / 60.0f); game.render(); SDL_GL_SwapWindow(win); }
        glFinish();

        double minMs = 1e9, maxMs = 0.0, sumMs = 0.0;
        for (int i = 0; i < benchmarkFrames; ++i) {
            uint64_t t0 = SDL_GetPerformanceCounter();
            game.update(1.0f / 60.0f);
            game.render();
            SDL_GL_SwapWindow(win);
            glFinish();
            uint64_t t1 = SDL_GetPerformanceCounter();
            double ms = double(t1 - t0) * 1000.0 / double(freq);
            minMs = std::min(minMs, ms);
            maxMs = std::max(maxMs, ms);
            sumMs += ms;
        }
        GLenum err = glGetError();
        double avgMs = sumMs / double(benchmarkFrames);
        std::printf("[BENCHMARK] %d frames @ %dx%d (%s):\n", benchmarkFrames, pixW, pixH, game.qualityName());
        std::printf("  Avg: %.2f ms (%.1f FPS) | Min: %.2f ms | Max: %.2f ms | glError: 0x%X\n",
                    avgMs, 1000.0 / std::max(avgMs, 0.001), minMs, maxMs, unsigned(err));
        printMemoryStats();
        game.destroy();
        SDL_GL_DestroyContext(ctx);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return (err == GL_NO_ERROR) ? 0 : 1;
    }

    // Interactive Main Loop
    uint64_t freq = SDL_GetPerformanceFrequency();
    uint64_t prevTime = SDL_GetPerformanceCounter();
    double fpsAccum = 0.0;
    int fpsFrames = 0;

    while (!game.shouldQuit()) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_EVENT_KEY_DOWN && !ev.key.repeat && ev.key.scancode == SDL_SCANCODE_F11) {
                fullscreen = !fullscreen;
                SDL_SetWindowFullscreen(win, fullscreen);
                continue;
            }
            game.handleEvent(ev);
        }

        uint64_t now = SDL_GetPerformanceCounter();
        double dt = double(now - prevTime) / double(freq);
        prevTime = now;

        fpsAccum += dt;
        fpsFrames++;
        if (fpsAccum >= 0.4) {
            game.setFpsDisplay(float(fpsFrames / fpsAccum));
            fpsAccum = 0.0;
            fpsFrames = 0;
        }

        game.update(float(std::min(dt, 0.05)));
        game.render();
        SDL_GL_SwapWindow(win);
    }

    game.destroy();
    SDL_GL_DestroyContext(ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
