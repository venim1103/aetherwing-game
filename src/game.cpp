#include "game.hpp"
#include "shaders.hpp"
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <cstdlib>

// All possible Roguelite Upgrades between waves
static const UpgradeOption ALL_UPGRADES[] = {
    {0, "HYPER CYCLER",     "CANNON FIRE RATE +24% & +15% DMG"},
    {1, "SWARM RACK MK.II", "+2 MISSILE LOCKS & +25% MISSILE DMG"},
    {2, "AEGIS CAPACITOR",  "+35 MAX SHIELD & +50% SHIELD REGEN"},
    {3, "AFTERBURNER CORE", "+35 MAX BOOST & +45% RECHARGE"},
    {4, "RAIL SLUG DRIVER", "PIERCING HIGH-VELOCITY PLASMA BOLTS"},
    {5, "VAMPIRIC NANOBOTS","REPAIR +40 HULL & KILLS RESTORE HULL"}
};

bool Game::init(int w, int h, bool softwareGL, QualityPreset initialQuality) {
    winW = w;
    winH = h;
    isSoftwareGL = softwareGL;
    quality = initialQuality;

    if (!initShaders()) return false;
    if (!hud.init()) return false;

    glfn::GenVertexArrays(1, &emptyVao);

    // Dynamic additive FX batch VAO/VBO
    glfn::GenVertexArrays(1, &fxVao);
    glfn::GenBuffers(1, &fxVbo);
    glfn::BindVertexArray(fxVao);
    glfn::BindBuffer(GL_ARRAY_BUFFER, fxVbo);
    glfn::BufferData(GL_ARRAY_BUFFER, 65536 * sizeof(FxVert), nullptr, GL_STREAM_DRAW);
    glfn::EnableVertexAttribArray(0);
    glfn::VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(FxVert), (void*)0);
    glfn::EnableVertexAttribArray(1);
    glfn::VertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(FxVert), (void*)(3 * sizeof(float)));
    glfn::BindVertexArray(0);
    fxVerts.reserve(16384);

    // Build all procedural 3D meshes
    meshPlayer    = meshes::buildPlayerFighter();
    meshThruster  = meshes::buildThrusterCone();
    meshDart      = meshes::buildEnemyDart();
    meshLancer    = meshes::buildEnemyLancer();
    meshBossHull  = meshes::buildBossHull();
    meshBossCore  = meshes::buildBossCore();
    meshDebris    = meshes::buildDebrisChunk();
    meshBolt      = meshes::buildProjectileBolt();
    meshMissile   = meshes::buildMissile();
    meshShockRing = meshes::buildShockRing();

    world.init(2026);
    audio.init();
    loadHighScore();

    if (!initFramebuffers(winW, winH)) return false;

    // Start in live animated Title Screen flight!
    state = GameState::Title;
    player = Player{};
    player.pos = {0, 68, 220};
    player.orient = Quat::identity();
    camPos = {24, 78, 245};
    camLook = player.pos;
    return true;
}

void Game::destroy() {
    audio.shutdown();
    hud.destroy();
    world.destroy();
    destroyFramebuffers();

    meshPlayer.destroy();
    meshThruster.destroy();
    meshDart.destroy();
    meshLancer.destroy();
    meshBossHull.destroy();
    meshBossCore.destroy();
    meshDebris.destroy();
    meshBolt.destroy();
    meshMissile.destroy();
    meshShockRing.destroy();

    if (fxVbo) { glfn::DeleteBuffers(1, &fxVbo); fxVbo = 0; }
    if (fxVao) { glfn::DeleteVertexArrays(1, &fxVao); fxVao = 0; }
    if (emptyVao) { glfn::DeleteVertexArrays(1, &emptyVao); emptyVao = 0; }

    auto delProg = [](GLuint& p) { if (p) { glfn::DeleteProgram(p); p = 0; } };
    delProg(progMesh);
    delProg(progOcean);
    delProg(progSky);
    delProg(progFx);
    delProg(progBloomExtract);
    delProg(progBlur);
    delProg(progPost);
    delProg(progPostFast);
}

bool Game::initShaders() {
    progMesh         = glfn::compileProgram(shaders::MESH_VERT, shaders::MESH_FRAG, "MESH");
    progOcean        = glfn::compileProgram(shaders::OCEAN_VERT, shaders::OCEAN_FRAG, "OCEAN");
    progSky          = glfn::compileProgram(shaders::SKY_VERT, shaders::SKY_FRAG, "SKY");
    progFx           = glfn::compileProgram(shaders::FX_VERT, shaders::FX_FRAG, "FX");
    progBloomExtract = glfn::compileProgram(shaders::POST_VERT, shaders::BLOOM_EXTRACT_FRAG, "BLOOM_EXTRACT");
    progBlur         = glfn::compileProgram(shaders::POST_VERT, shaders::BLUR_FRAG, "BLUR");
    progPost         = glfn::compileProgram(shaders::POST_VERT, shaders::POST_FRAG, "POST");
    progPostFast     = glfn::compileProgram(shaders::POST_VERT, shaders::POST_FAST_FRAG, "POST_FAST");
    if (!progMesh || !progOcean || !progSky || !progFx || !progBloomExtract || !progBlur || !progPost || !progPostFast) {
        return false;
    }

    mLocViewProj     = glfn::GetUniformLocation(progMesh, "uViewProj");
    mLocModel        = glfn::GetUniformLocation(progMesh, "uModel");
    mLocCamPos       = glfn::GetUniformLocation(progMesh, "uCamPos");
    mLocSunDir       = glfn::GetUniformLocation(progMesh, "uSunDir");
    mLocSunColor     = glfn::GetUniformLocation(progMesh, "uSunColor");
    mLocSkyZenith    = glfn::GetUniformLocation(progMesh, "uSkyZenith");
    mLocSkyHorizon   = glfn::GetUniformLocation(progMesh, "uSkyHorizon");
    mLocGroundBounce = glfn::GetUniformLocation(progMesh, "uGroundBounce");
    mLocFogColor     = glfn::GetUniformLocation(progMesh, "uFogColor");
    mLocFogDensity   = glfn::GetUniformLocation(progMesh, "uFogDensity");
    mLocTint         = glfn::GetUniformLocation(progMesh, "uTint");
    mLocEmissiveAdd  = glfn::GetUniformLocation(progMesh, "uEmissiveAdd");
    mLocTime         = glfn::GetUniformLocation(progMesh, "uTime");
    mLocPointPos     = glfn::GetUniformLocation(progMesh, "uPointPos");
    mLocPointColor   = glfn::GetUniformLocation(progMesh, "uPointColor");
    mLocPointRadius  = glfn::GetUniformLocation(progMesh, "uPointRadius");
    return true;
}

void Game::destroyFramebuffers() {
    if (sceneDepthRbo) { glfn::DeleteRenderbuffers(1, &sceneDepthRbo); sceneDepthRbo = 0; }
    if (sceneColorTex) { glDeleteTextures(1, &sceneColorTex); sceneColorTex = 0; }
    if (sceneFbo)      { glfn::DeleteFramebuffers(1, &sceneFbo); sceneFbo = 0; }
    for (int i = 0; i < 2; ++i) {
        if (bloomTex[i]) { glDeleteTextures(1, &bloomTex[i]); bloomTex[i] = 0; }
        if (bloomFbo[i]) { glfn::DeleteFramebuffers(1, &bloomFbo[i]); bloomFbo[i] = 0; }
    }
}

bool Game::initFramebuffers(int w, int h) {
    destroyFramebuffers();
    float scale = (quality == QualityPreset::High) ? 1.0f : (quality == QualityPreset::Balanced ? 0.65f : 0.48f);
    fboW = std::max(240, int(w * scale));
    fboH = std::max(135, int(h * scale));
    bloomW = std::max(100, fboW / 4);
    bloomH = std::max(56, fboH / 4);

    // Use lightweight RGBA8 on FAST/BALANCED (massive speed & RAM win on llvmpipe), RGB16F HDR on HIGH!
    bool useFloat = (quality == QualityPreset::High);
    auto setupTex = [useFloat](GLuint& tex, int tw, int th) {
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        if (useFloat) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, tw, th, 0, GL_RGB, GL_FLOAT, nullptr);
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tw, th, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    };

    glfn::GenFramebuffers(1, &sceneFbo);
    glfn::BindFramebuffer(GL_FRAMEBUFFER, sceneFbo);
    setupTex(sceneColorTex, fboW, fboH);
    glfn::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneColorTex, 0);

    glfn::GenRenderbuffers(1, &sceneDepthRbo);
    glfn::BindRenderbuffer(GL_RENDERBUFFER, sceneDepthRbo);
    glfn::RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, fboW, fboH);
    glfn::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, sceneDepthRbo);

    if (glfn::CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::fprintf(stderr, "[GL] Scene FBO incomplete!\n");
        return false;
    }

    for (int i = 0; i < 2; ++i) {
        glfn::GenFramebuffers(1, &bloomFbo[i]);
        glfn::BindFramebuffer(GL_FRAMEBUFFER, bloomFbo[i]);
        setupTex(bloomTex[i], bloomW, bloomH);
        glfn::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloomTex[i], 0);
    }

    glfn::BindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

void Game::setQuality(QualityPreset q) {
    quality = q;
    initFramebuffers(winW, winH);
    addCallout(std::string("QUALITY: ") + qualityName(), {0.4f, 0.95f, 1.0f, 1.0f}, 1.6f);
}

const char* Game::qualityName() const {
    switch (quality) {
        case QualityPreset::Fast:     return "FAST";
        case QualityPreset::Balanced: return "BALANCED";
        case QualityPreset::High:     return "HIGH";
    }
    return "BALANCED";
}

void Game::setWindowSize(int w, int h) {
    if (w < 100 || h < 100) return;
    if (w == winW && h == winH) return;
    winW = w;
    winH = h;
    initFramebuffers(winW, winH);
}

void Game::loadHighScore() {
    const char* home = std::getenv("HOME");
    std::string path = home ? std::string(home) + "/.aetherwing_highscore" : ".aetherwing_highscore";
    std::ifstream in(path);
    if (in) in >> highScore;
}

void Game::saveHighScore() {
    if (score <= highScore) return;
    highScore = score;
    const char* home = std::getenv("HOME");
    std::string path = home ? std::string(home) + "/.aetherwing_highscore" : ".aetherwing_highscore";
    std::ofstream out(path);
    if (out) out << highScore << "\n";
}

void Game::startNewRun(GameMode m) {
    mode = m;
    state = GameState::Playing;
    player = Player{};
    player.pos = {0, 62, 160};
    player.orient = Quat::identity();
    player.vel = {0, 0, -player.speed};
    camPos = player.pos + Vec3{0, 10, 26};
    camLook = player.pos + Vec3{0, 0, -60};
    camFovDeg = 65.0f;

    for (auto& e : enemies) e.active = false;
    for (auto& p : projectiles) p.active = false;
    for (auto& ms : missiles) ms.active = false;
    for (auto& pt : particles) pt.active = false;
    for (auto& d : debris) d.active = false;
    for (auto& s : shockwaves) s.active = false;
    lockedIds.clear();

    score = 0;
    kills = 0;
    ringsCleared = 0;
    combo = 1;
    comboTimer = 0.0f;
    wave = 1;
    waveClearDelay = 0.0f;
    input.steerX = 0.0f;
    input.steerY = 0.0f;

    if (mode == GameMode::Assault) {
        world.setSector(0, false);
        spawnWave(1);
    } else {
        addCallout("FREE FLIGHT TOUR - PRESS [T] TO SWITCH SECTORS", {0.35f, 0.96f, 1.0f, 1.0f}, 3.2f);
    }
    audio.playSfx(SfxType::Upgrade, 0.65f, 1.0f);
}

void Game::forceBossDemo(int sectorIdx) {
    startNewRun(GameMode::Assault);
    world.setSector(sectorIdx, true);
    for (auto& e : enemies) e.active = false;
    wave = 3;
    spawnBoss();
    // Place Dreadnought right in front of the player's sweeping attack vector!
    for (auto& e : enemies) {
        if (e.active && e.type == EnemyType::BossHull) {
            e.aiPhase = 3.14159f; // center at z = -440, x = 0!
            e.pos = {0, 148, -220};
            e.orient = Quat::fromAxisAngle({0, 1, 0}, 0.45f);
        }
    }
    for (auto& e : enemies) {
        if (e.active && e.type == EnemyType::BossCore && e.parentBossIdx >= 0) {
            const Enemy& b = enemies[e.parentBossIdx];
            e.pos = b.pos + b.orient.rotate(e.localOffset);
        }
    }
    player.pos = {18, 128, -55};
    player.orient = Quat::fromAxisAngle({0, 1, 0}, 0.08f) * Quat::fromAxisAngle({0, 0, 1}, -0.24f);
    player.visualBank = -0.35f;
    player.boosting = true;
    player.boostSmooth = 0.85f;
    camPos = player.pos + Vec3{-3.0f, 7.5f, 22.0f};
    camLook = {0, 142, -220};
    addCallout("AEGIS SKY-DREADNOUGHT ENGAGED", {1.0f, 0.30f, 0.58f, 1.0f}, 3.0f);
    // Add active escorts, lasers & explosion sparks!
    spawnEnemy(EnemyType::Dart, {-58, 138, -145});
    spawnEnemy(EnemyType::Lancer, {64, 142, -165});
    spawnExplosion({-18, 148, -195}, 2.0f, {1.0f, 0.55f, 0.15f}, false);
    for (int i = 0; i < 6; ++i) {
        Vec3 fwd = player.orient.forward();
        Projectile& pr = projectiles[i];
        pr.active = true;
        pr.fromPlayer = true;
        pr.pos = player.pos + fwd * (18.0f + i * 24.0f) + Vec3{(i % 2 ? 1.8f : -1.8f), -0.4f, 0};
        pr.vel = fwd * 380.0f;
        pr.color = {0.25f, 0.95f, 1.0f};
        pr.life = 1.5f;
    }
}

void Game::setTitleDemoCamera(int sectorIdx) {
    state = GameState::Title;
    world.setSector(sectorIdx, true);
    totalTime = 2.5f;
    updateTitleAutopilot(0.016f);
}

void Game::handleEvent(const SDL_Event& ev) {
    if (ev.type == SDL_EVENT_QUIT) {
        quitRequested = true;
        return;
    }
    if (ev.type == SDL_EVENT_WINDOW_RESIZED || ev.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
        setWindowSize(ev.window.data1, ev.window.data2);
        return;
    }
    if (ev.type == SDL_EVENT_MOUSE_MOTION) {
        input.mouseX = ev.motion.x;
        input.mouseY = ev.motion.y;
        if (state == GameState::Playing) {
            float nx = (input.mouseX / float(std::max(1, winW))) * 2.0f - 1.0f;
            float ny = (input.mouseY / float(std::max(1, winH))) * 2.0f - 1.0f;
            // Comfortable small center deadzone
            auto dz = [](float v) {
                float a = std::abs(v);
                if (a < 0.06f) return 0.0f;
                return (v > 0 ? 1.0f : -1.0f) * clampf((a - 0.06f) / 0.86f, 0.0f, 1.0f);
            };
            input.steerX = dz(nx);
            input.steerY = dz(ny);
            input.usingMouseSteer = true;
        }
        return;
    }
    if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        if (ev.button.button == SDL_BUTTON_LEFT) {
            input.mouseLeft = true;
            input.mouseClicked = true;
        }
        if (ev.button.button == SDL_BUTTON_RIGHT) input.mouseRight = true;
        if (ev.button.button == SDL_BUTTON_MIDDLE) input.keyShift = true;
        return;
    }
    if (ev.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        if (ev.button.button == SDL_BUTTON_LEFT) input.mouseLeft = false;
        if (ev.button.button == SDL_BUTTON_RIGHT) input.mouseRight = false;
        if (ev.button.button == SDL_BUTTON_MIDDLE) input.keyShift = false;
        return;
    }
    if (ev.type == SDL_EVENT_MOUSE_WHEEL) {
        input.wheelDelta += ev.wheel.y;
        return;
    }
    if (ev.type == SDL_EVENT_KEY_DOWN && !ev.key.repeat) {
        SDL_Scancode sc = ev.key.scancode;
        if (sc == SDL_SCANCODE_F1) {
            int next = (int(quality) + 1) % 3;
            setQuality(static_cast<QualityPreset>(next));
            return;
        }
        if (sc == SDL_SCANCODE_M) {
            audio.toggleMusic();
            addCallout(audio.isMusicEnabled() ? "MUSIC: ON" : "MUSIC: OFF", {0.6f, 0.9f, 1.0f, 1.0f}, 1.2f);
            return;
        }
        if (sc == SDL_SCANCODE_N) {
            audio.toggleMute();
            addCallout(audio.isMuted() ? "AUDIO: MUTED" : "AUDIO: ON", {0.6f, 0.9f, 1.0f, 1.0f}, 1.2f);
            return;
        }
        if (sc == SDL_SCANCODE_C && state == GameState::Playing) {
            camMode = (camMode + 1) % 3;
            const char* names[3] = {"CAMERA: DYNAMIC CHASE", "CAMERA: CLOSE ACTION", "CAMERA: COCKPIT NOSE"};
            addCallout(names[camMode], {0.5f, 0.95f, 1.0f, 1.0f}, 1.4f);
            return;
        }
        if (sc == SDL_SCANCODE_T && (state == GameState::Title || (state == GameState::Playing && mode == GameMode::FreeFlight))) {
            world.setSector(world.currentSector + 1, false);
            addCallout(std::string("SECTOR: ") + world.targetPalette.name, {1.0f, 0.85f, 0.35f, 1.0f}, 2.2f);
            return;
        }

        if (state == GameState::Title) {
            if (sc == SDL_SCANCODE_RETURN || sc == SDL_SCANCODE_SPACE) startNewRun(GameMode::Assault);
            else if (sc == SDL_SCANCODE_F) startNewRun(GameMode::FreeFlight);
            else if (sc == SDL_SCANCODE_1) world.setSector(0, false);
            else if (sc == SDL_SCANCODE_2) world.setSector(1, false);
            else if (sc == SDL_SCANCODE_3) world.setSector(2, false);
            else if (sc == SDL_SCANCODE_ESCAPE) quitRequested = true;
            return;
        }
        if (state == GameState::UpgradeDraft) {
            if (sc == SDL_SCANCODE_1) applyUpgrade(0);
            else if (sc == SDL_SCANCODE_2) applyUpgrade(1);
            else if (sc == SDL_SCANCODE_3) applyUpgrade(2);
            return;
        }
        if (state == GameState::Paused) {
            if (sc == SDL_SCANCODE_ESCAPE || sc == SDL_SCANCODE_P) state = GameState::Playing;
            else if (sc == SDL_SCANCODE_R) startNewRun(mode);
            else if (sc == SDL_SCANCODE_Q) state = GameState::Title;
            return;
        }
        if (state == GameState::GameOver) {
            if (sc == SDL_SCANCODE_RETURN || sc == SDL_SCANCODE_R || sc == SDL_SCANCODE_SPACE) startNewRun(GameMode::Assault);
            else if (sc == SDL_SCANCODE_ESCAPE) state = GameState::Title;
            return;
        }
        if (state == GameState::Playing) {
            if (sc == SDL_SCANCODE_ESCAPE || sc == SDL_SCANCODE_P) {
                state = GameState::Paused;
                return;
            }
            if (sc == SDL_SCANCODE_SPACE) input.spacePressed = true;
            if (sc == SDL_SCANCODE_Q) input.qPressed = true;
            if (sc == SDL_SCANCODE_E) input.ePressed = true;
        }
    }
}

void Game::update(float dt) {
    dt = std::min(dt, 0.05f);

    // Poll live keyboard state from SDL3
    const bool* keys = SDL_GetKeyboardState(nullptr);
    input.keyW     = keys[SDL_SCANCODE_W];
    input.keyS     = keys[SDL_SCANCODE_S];
    input.keyA     = keys[SDL_SCANCODE_A];
    input.keyD     = keys[SDL_SCANCODE_D];
    input.keyUp    = keys[SDL_SCANCODE_UP];
    input.keyDown  = keys[SDL_SCANCODE_DOWN];
    input.keyLeft  = keys[SDL_SCANCODE_LEFT];
    input.keyRight = keys[SDL_SCANCODE_RIGHT];
    input.keyShift = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
    input.keySpace = keys[SDL_SCANCODE_SPACE];
    input.keyFire  = keys[SDL_SCANCODE_J] || keys[SDL_SCANCODE_Z];
    input.keyMissile = keys[SDL_SCANCODE_K] || keys[SDL_SCANCODE_X];

    // Touching arrow keys seamlessly switches out of mouse steer so pure-keyboard feels instant
    if (input.keyUp || input.keyDown || input.keyLeft || input.keyRight) {
        input.usingMouseSteer = false;
        input.steerX = 0.0f;
        input.steerY = 0.0f;
    }

    if (callout.timer > 0.0f) callout.timer = std::max(0.0f, callout.timer - dt);
    if (waveBannerTimer > 0.0f) waveBannerTimer = std::max(0.0f, waveBannerTimer - dt);

    if (state == GameState::Paused) {
        audio.setFlightParams(0.0f, 0.0f, false, false);
        input.mouseClicked = false;
        return;
    }

    totalTime += dt;
    world.update(dt);

    if (state == GameState::Title) {
        updateTitleAutopilot(dt);
        updateEffects(dt);
        audio.setFlightParams(0.35f, 0.0f, false, false);
        // Handle clickable title buttons
        float cx = winW * 0.5f;
        float by1 = winH * 0.58f;
        float by2 = winH * 0.67f;
        titleHover = -1;
        if (std::abs(input.mouseX - cx) < 180.0f) {
            if (input.mouseY >= by1 && input.mouseY <= by1 + 48.0f) titleHover = 0;
            else if (input.mouseY >= by2 && input.mouseY <= by2 + 44.0f) titleHover = 1;
        }
        if (input.mouseClicked) {
            if (titleHover == 0) startNewRun(GameMode::Assault);
            else if (titleHover == 1) startNewRun(GameMode::FreeFlight);
        }
        input.mouseClicked = false;
        return;
    }

    if (state == GameState::UpgradeDraft) {
        // Keep world gracefully cruising in slow motion behind upgrade cards!
        updateTitleAutopilot(dt * 0.25f);
        updateEffects(dt * 0.35f);
        audio.setFlightParams(0.3f, 0.0f, false, false);
        // Mouse click support on the 3 upgrade cards
        draftHover = -1;
        float cardW = std::min(290.0f, (winW - 140.0f) / 3.0f);
        float cardH = 190.0f;
        float gap = 24.0f;
        float startX = winW * 0.5f - (cardW * 3 + gap * 2) * 0.5f;
        float startY = winH * 0.42f;
        for (int i = 0; i < 3; ++i) {
            float x = startX + i * (cardW + gap);
            if (input.mouseX >= x && input.mouseX <= x + cardW &&
                input.mouseY >= startY && input.mouseY <= startY + cardH) {
                draftHover = i;
                if (input.mouseClicked) applyUpgrade(i);
            }
        }
        input.mouseClicked = false;
        return;
    }

    if (state == GameState::GameOver) {
        updateEffects(dt * 0.65f);
        camPos = expDecay(camPos, player.pos + Vec3{std::cos(totalTime * 0.5f) * 55.0f, 26.0f, std::sin(totalTime * 0.5f) * 55.0f}, 2.0f, dt);
        camLook = expDecay(camLook, player.pos, 4.0f, dt);
        audio.setFlightParams(0.0f, 0.0f, false, false);
        if (input.mouseClicked) startNewRun(GameMode::Assault);
        input.mouseClicked = false;
        return;
    }

    // Active gameplay hitstop (brief dramatic impact freeze on boss core kills)
    if (hitstopTimer > 0.0f) {
        hitstopTimer -= dt;
        dt *= 0.12f;
    }

    updatePlayer(dt);
    updateWeaponsAndProjectiles(dt);
    if (mode == GameMode::Assault) {
        updateEnemies(dt);
        updateWaves(dt);
    }
    updateEffects(dt);
    updateCamera(dt);

    // Check if boss is active for dynamic soundtrack shift
    bool bossAlive = false;
    for (const auto& e : enemies) {
        if (e.active && e.type == EnemyType::BossHull) { bossAlive = true; break; }
    }
    audio.setFlightParams(player.speed / 130.0f, player.boostSmooth, mode == GameMode::Assault, bossAlive);

    // Decay transient inputs
    input.spacePressed = false;
    input.qPressed = false;
    input.ePressed = false;
    input.mouseClicked = false;
    input.wheelDelta = 0.0f;
}

void Game::updateTitleAutopilot(float dt) {
    // Sweeping scenic flight figure-8 over the sunlit archipelago!
    float a = totalTime * 0.18f;
    Vec3 targetPos{std::sin(a) * 290.0f, 88.0f + std::sin(a * 2.1f) * 24.0f, std::cos(a) * 320.0f - 80.0f};
    Vec3 nextPos{std::sin(a + 0.08f) * 290.0f, 88.0f + std::sin((a + 0.08f) * 2.1f) * 24.0f, std::cos(a + 0.08f) * 320.0f - 80.0f};
    Vec3 fwd = normalize(nextPos - targetPos);
    Vec3 right = normalize(cross(fwd, {0, 1, 0}));
    Vec3 up = cross(right, fwd);
    // Construct orientation looking along fwd with slight scenic bank
    float bank = -0.32f * std::cos(a);
    Quat bankQ = Quat::fromAxisAngle(fwd, bank);
    player.pos = targetPos;
    player.vel = fwd * 85.0f;
    player.visualBank = bank;

    // Derive yaw & pitch from fwd
    float yaw = std::atan2(-fwd.x, -fwd.z);
    float pitch = std::asin(clampf(fwd.y, -0.95f, 0.95f));
    player.orient = (Quat::fromAxisAngle({0, 1, 0}, yaw) *
                     Quat::fromAxisAngle({1, 0, 0}, pitch) *
                     Quat::fromAxisAngle({0, 0, 1}, bank)).normalized();
    (void)bankQ; (void)up;

    // Wingtip vapor trail
    player.trailSampleAccum += dt;
    if (player.trailSampleAccum >= 0.025f) {
        player.trailSampleAccum = 0.0f;
        Vec3 r = player.orient.right();
        Vec3 b = -player.orient.forward();
        Vec3 wL = player.pos - r * 4.25f + b * 1.9f;
        Vec3 wR = player.pos + r * 4.25f + b * 1.9f;
        for (int i = Player::TRAIL_LEN - 1; i > 0; --i) {
            player.trailL[i] = player.trailL[i - 1];
            player.trailR[i] = player.trailR[i - 1];
        }
        player.trailL[0] = wL;
        player.trailR[0] = wR;
        if (player.trailCount < Player::TRAIL_LEN) player.trailCount++;
    }

    // Sweeping heroic showcase camera orbiting off the starboard-rear quarter
    Vec3 pFwd = player.orient.forward();
    Vec3 pRight = player.orient.right();
    float orbit = std::sin(totalTime * 0.25f) * 8.0f;
    camPos = player.pos - pFwd * 24.0f + pRight * (9.5f + orbit) + Vec3{0, 6.5f, 0};
    camLook = player.pos + pFwd * 38.0f;
    camUp = {0, 1, 0};
    camFovDeg = 62.0f;
}

void Game::updatePlayer(float dt) {
    // 1. Throttle & Afterburner Boost
    if (input.keyW) player.throttle = clampf(player.throttle + dt * 0.85f, 0.15f, 1.0f);
    if (input.keyS) player.throttle = clampf(player.throttle - dt * 0.95f, 0.15f, 1.0f);
    if (std::abs(input.wheelDelta) > 0.01f) {
        player.throttle = clampf(player.throttle + input.wheelDelta * 0.10f, 0.15f, 1.0f);
    }

    bool wantBoost = input.keyShift && player.boost > 6.0f;
    if (wantBoost && !player.boosting && player.boost > 18.0f) {
        // Trigger sonic boom shockwave on ignition!
        spawnShockwave(player.pos - player.orient.forward() * 2.5f, player.orient, {0.35f, 0.92f, 1.0f}, 14.0f, 0.35f);
        audio.playSfx(SfxType::BarrelRoll, 0.55f, 1.25f);
    }
    player.boosting = wantBoost && (player.boosting || player.boost > 14.0f);
    if (player.boosting) {
        player.boost = std::max(0.0f, player.boost - 34.0f * dt);
        if (player.boost <= 0.5f) player.boosting = false;
    } else {
        player.boost = std::min(player.maxBoost, player.boost + player.boostRecharge * dt);
    }
    player.boostSmooth = expDecay(player.boostSmooth, player.boosting ? 1.0f : 0.0f, 7.0f, dt);

    float targetSpeed = lerpf(58.0f, 112.0f, player.throttle) + player.boostSmooth * 68.0f;
    player.speed = expDecay(player.speed, targetSpeed, 4.5f, dt);

    // 2. Steering (Hybrid Cursor-Steer + Keyboard/Arrows)
    float yawCmd = 0.0f;
    float pitchCmd = 0.0f;
    if (input.usingMouseSteer) {
        yawCmd = -input.steerX;
        pitchCmd = -input.steerY; // Mouse up -> nose up! (arcade intuitive)
    }
    if (input.keyLeft || input.keyA)  yawCmd += 0.95f;
    if (input.keyRight || input.keyD) yawCmd -= 0.95f;
    if (input.keyUp)   pitchCmd += 0.90f;
    if (input.keyDown) pitchCmd -= 0.90f;

    yawCmd = clampf(yawCmd, -1.15f, 1.15f);
    pitchCmd = clampf(pitchCmd, -1.0f, 1.0f);

    player.yawInput = expDecay(player.yawInput, yawCmd, 11.0f, dt);
    player.pitchInput = expDecay(player.pitchInput, pitchCmd, 11.0f, dt);

    // Apply crisp arcade flight orientation with automatic horizon leveling
    Vec3 fwd = player.orient.forward();
    float curYaw = std::atan2(-fwd.x, -fwd.z);
    float curPitch = std::asin(clampf(fwd.y, -0.95f, 0.95f));

    curYaw += player.yawInput * 1.65f * dt;
    curPitch = clampf(curPitch + player.pitchInput * 1.45f * dt, -1.08f, 1.08f);

    // Gentle auto-pull-up above ocean waves so skimming feels thrilling, never frustrating!
    if (player.pos.y < 14.0f && curPitch < 0.05f) {
        float pull = smoothstepf(14.0f, 4.0f, player.pos.y);
        curPitch = lerpf(curPitch, 0.16f, pull * clampf(dt * 6.0f, 0.0f, 1.0f));
    }

    // Soft arena boundary turn-back so you never get lost in empty ocean
    float distFromCenter = std::sqrt(player.pos.x * player.pos.x + player.pos.z * player.pos.z);
    if (distFromCenter > World::ARENA_RADIUS) {
        Vec3 toCenter = normalize({-player.pos.x, 0, -player.pos.z});
        float desiredYaw = std::atan2(-toCenter.x, -toCenter.z);
        float diff = desiredYaw - curYaw;
        while (diff > PI) diff -= TAU;
        while (diff < -PI) diff += TAU;
        curYaw += diff * clampf(dt * 2.2f, 0.0f, 1.0f);
        if (callout.timer <= 0.1f) addCallout("RETURNING TO ARCHIPELAGO AIRSPACE", {1.0f, 0.78f, 0.25f, 1.0f}, 1.5f);
    }

    // 3. Snap 360 Barrel Roll (deflects enemy bolts!)
    if (player.rollCooldown > 0.0f) player.rollCooldown = std::max(0.0f, player.rollCooldown - dt);
    if ((input.spacePressed || input.qPressed || input.ePressed) && player.rollCooldown <= 0.0f && player.rollTimer <= 0.0f) {
        player.rollTimer = 0.42f;
        player.rollCooldown = 0.85f;
        player.rollDir = input.qPressed ? 1.0f : (input.ePressed ? -1.0f : (player.yawInput >= 0 ? 1.0f : -1.0f));
        audio.playSfx(SfxType::BarrelRoll, 0.65f, 1.0f);
        spawnShockwave(player.pos, player.orient, {0.3f, 0.95f, 1.0f}, 10.0f, 0.28f);
    }

    float targetBank = player.yawInput * 0.82f;
    player.visualBank = expDecay(player.visualBank, targetBank, 9.0f, dt);

    float rollAngle = player.visualBank;
    if (player.rollTimer > 0.0f) {
        player.rollTimer = std::max(0.0f, player.rollTimer - dt);
        float u = 1.0f - (player.rollTimer / 0.42f);
        rollAngle += player.rollDir * TAU * smoothstepf(0.0f, 1.0f, u);
        // Side-juke during barrel roll for crisp evasion
        player.pos += player.orient.right() * (-player.rollDir * 24.0f * dt);
    }

    player.orient = (Quat::fromAxisAngle({0, 1, 0}, curYaw) *
                     Quat::fromAxisAngle({1, 0, 0}, curPitch) *
                     Quat::fromAxisAngle({0, 0, 1}, rollAngle)).normalized();

    player.vel = player.orient.forward() * player.speed;
    player.pos += player.vel * dt;

    // Altitude ceiling & wave-skim floor
    if (player.pos.y > 390.0f) player.pos.y = 390.0f;
    if (player.pos.y < 5.5f) player.pos.y = 5.5f;

    // Exciting Ocean Wave-Skim Spray & Boost Recharge Bonus!
    if (player.pos.y < 15.5f) {
        player.waveSkimTimer += dt;
        player.boost = std::min(player.maxBoost, player.boost + 24.0f * dt);
        for (int i = 0; i < 2; ++i) {
            Vec3 sp = {player.pos.x + rng.range(-5.0f, 5.0f), 1.4f, player.pos.z + rng.range(-5.0f, 5.0f)};
            Vec3 sv = {rng.range(-12.0f, 12.0f), rng.range(8.0f, 22.0f), rng.range(-12.0f, 12.0f)};
            spawnParticle(sp, sv - player.vel * 0.25f, {0.72f, 0.96f, 1.0f}, rng.range(1.2f, 2.5f), 0.45f, 2.2f);
        }
        if (player.waveSkimTimer > 1.2f) {
            player.waveSkimTimer = 0.0f;
            score += 100 * combo;
            addCallout("WAVE SKIM! +BOOST", {0.35f, 0.95f, 1.0f, 1.0f}, 1.1f);
        }
    } else {
        player.waveSkimTimer = 0.0f;
    }

    // Solid terrain / floating-island soft deflection collision
    Vec3 pushDir;
    if (world.checkSolidCollision(player.pos, 4.8f, pushDir)) {
        player.pos += pushDir * 11.0f;
        if (player.invulnTimer <= 0.0f) {
            damagePlayer(18.0f);
            player.invulnTimer = 0.55f;
            shakeAmt = std::max(shakeAmt, 0.85f);
            addCallout("TERRAIN IMPACT!", {1.0f, 0.35f, 0.25f, 1.0f}, 1.0f);
        }
    }

    // Ring Gate Fly-Through Detection!
    for (auto& g : world.ringGates) {
        if (g.cooldown > 0.0f) continue;
        float dist = (player.pos - g.pos).len();
        if (dist < g.radius) {
            g.cooldown = 5.0f;
            ringsCleared++;
            player.boost = player.maxBoost;
            player.shield = std::min(player.maxShield, player.shield + 28.0f);
            player.speed = std::max(player.speed, 165.0f);
            combo = std::min(12, combo + 1);
            comboTimer = 8.0f;
            score += 350 * combo;
            shakeAmt = std::max(shakeAmt, 0.35f);
            audio.playSfx(SfxType::RingGate, 0.78f, 1.0f + std::min(combo, 8) * 0.04f);
            spawnShockwave(g.pos, player.orient, {1.0f, 0.85f, 0.25f}, 36.0f, 0.65f);
            for (int k = 0; k < 22; ++k) {
                spawnParticle(g.pos + rng.unitSphere() * 14.0f, rng.unitSphere() * 32.0f,
                              {1.0f, 0.86f, 0.30f}, rng.range(1.6f, 3.2f), 0.75f);
            }
            addCallout("RING BOOST! +SHIELD  X" + std::to_string(combo), {1.0f, 0.88f, 0.28f, 1.0f}, 1.6f);
        }
    }

    // Shield regen & timers
    if (player.invulnTimer > 0.0f) player.invulnTimer = std::max(0.0f, player.invulnTimer - dt);
    if (player.damageFlash > 0.0f) player.damageFlash = std::max(0.0f, player.damageFlash - dt * 2.4f);
    if (player.shieldFlash > 0.0f) player.shieldFlash = std::max(0.0f, player.shieldFlash - dt * 2.8f);
    if (player.shieldRegenDelay > 0.0f) {
        player.shieldRegenDelay -= dt;
    } else if (player.shield < player.maxShield) {
        player.shield = std::min(player.maxShield, player.shield + player.shieldRegenRate * dt);
    }

    if (comboTimer > 0.0f) {
        comboTimer -= dt;
        if (comboTimer <= 0.0f) combo = 1;
    }

    // Update wingtip vapor trails
    player.trailSampleAccum += dt;
    if (player.trailSampleAccum >= 0.018f) {
        player.trailSampleAccum = 0.0f;
        Vec3 r = player.orient.right();
        Vec3 b = -player.orient.forward();
        Vec3 wL = player.pos - r * 4.25f + b * 1.9f;
        Vec3 wR = player.pos + r * 4.25f + b * 1.9f;
        for (int i = Player::TRAIL_LEN - 1; i > 0; --i) {
            player.trailL[i] = player.trailL[i - 1];
            player.trailR[i] = player.trailR[i - 1];
        }
        player.trailL[0] = wL;
        player.trailR[0] = wR;
        if (player.trailCount < Player::TRAIL_LEN) player.trailCount++;
    }
}

void Game::updateCamera(float dt) {
    Vec3 fwd = player.orient.forward();
    Vec3 up = player.orient.up();
    Vec3 right = player.orient.right();

    Vec3 desiredPos, desiredLook, desiredUp;
    float desiredFov = 64.0f + player.boostSmooth * 18.0f;

    if (camMode == 0) {
        // Dynamic Arcade Chase Camera
        desiredPos = player.pos - fwd * (21.5f + player.boostSmooth * 4.5f) + Vec3{0, 6.2f, 0} + right * (-player.yawInput * 3.8f);
        desiredLook = player.pos + fwd * 85.0f + Vec3{0, 1.8f, 0};
        desiredUp = normalize(lerp({0, 1, 0}, up, 0.25f));
    } else if (camMode == 1) {
        // Close Cinematic Action Camera
        desiredPos = player.pos - fwd * (14.5f + player.boostSmooth * 2.5f) + Vec3{0, 4.2f, 0};
        desiredLook = player.pos + fwd * 95.0f;
        desiredUp = normalize(lerp({0, 1, 0}, up, 0.42f));
    } else {
        // Cockpit Nose Camera
        desiredPos = player.pos + fwd * 2.2f + up * 1.2f;
        desiredLook = player.pos + fwd * 120.0f;
        desiredUp = up;
        desiredFov += 8.0f;
    }

    if (desiredPos.y < 3.5f) desiredPos.y = 3.5f;

    float posLambda = (camMode == 2) ? 40.0f : 11.5f;
    camPos  = expDecay(camPos, desiredPos, posLambda, dt);
    camLook = expDecay(camLook, desiredLook, 18.0f, dt);
    camUp   = normalize(expDecay(camUp, desiredUp, 10.0f, dt));
    camFovDeg = expDecay(camFovDeg, desiredFov, 7.0f, dt);

    if (shakeAmt > 0.001f) {
        camPos += rng.unitSphere() * (shakeAmt * 1.45f);
        shakeAmt = std::max(0.0f, shakeAmt - dt * 3.2f);
    }
}

void Game::updateWeaponsAndProjectiles(float dt) {
    // 1. Twin Pulse Cannons
    if (player.fireCooldown > 0.0f) player.fireCooldown -= dt;
    if ((input.mouseLeft || input.keyFire) && player.fireCooldown <= 0.0f) {
        player.fireCooldown = player.fireRate;
        player.wingToggle ^= 1;

        Vec3 fwd = player.orient.forward();
        Vec3 right = player.orient.right();
        Vec3 up = player.orient.up();

        // Subtle smart aim-assist toward enemy closest to crosshair (makes dogfighting feel AMAZING!)
        Vec3 shootDir = fwd;
        float bestDot = 0.965f; // ~15 degree cone
        for (const auto& e : enemies) {
            if (!e.active || e.type == EnemyType::BossHull) continue;
            Vec3 toE = e.pos - player.pos;
            float dist = toE.len();
            if (dist < 10.0f || dist > 620.0f) continue;
            // Lead moving target slightly
            float tFlight = dist / (player.piercingBolts ? 540.0f : 420.0f);
            Vec3 leadPos = e.pos + e.vel * (tFlight * 0.75f);
            Vec3 d = normalize(leadPos - player.pos);
            float dp = dot(d, fwd);
            if (dp > bestDot) {
                bestDot = dp;
                shootDir = d;
            }
        }

        // Fire from alternating or twin wing cannons
        for (float sx : {-1.65f, 1.65f}) {
            for (auto& pr : projectiles) {
                if (pr.active) continue;
                pr.active = true;
                pr.fromPlayer = true;
                pr.piercing = player.piercingBolts;
                pr.pos = player.pos + right * sx - up * 0.25f + fwd * 3.2f;
                float spd = player.piercingBolts ? 560.0f : 430.0f;
                pr.vel = shootDir * spd + player.vel * 0.25f;
                pr.color = player.piercingBolts ? Vec3{0.45f, 1.0f, 0.75f} : Vec3{0.20f, 0.92f, 1.0f};
                pr.damage = player.cannonDamage * (player.piercingBolts ? 1.2f : 1.0f);
                pr.life = 1.65f;
                pr.radius = 2.0f;
                break;
            }
        }
        // Dynamic muzzle flash light
        dynLightPos = player.pos + fwd * 4.5f;
        dynLightColor = {0.25f, 0.92f, 1.0f};
        dynLightRadius = std::max(dynLightRadius, 24.0f);
        audio.playSfx(SfxType::Laser, 0.42f, rng.range(0.94f, 1.08f));
    }

    // 2. Multi-Target Swarm Lock-On Missiles!
    if (player.missileCooldown > 0.0f) player.missileCooldown -= dt;
    bool holdMissile = input.mouseRight || input.keyMissile;
    if (holdMissile && player.missileCooldown <= 0.0f) {
        lockingActive = true;
        lockScanTimer -= dt;
        // Remove invalid / dead locked IDs
        lockedIds.erase(std::remove_if(lockedIds.begin(), lockedIds.end(), [&](int id) {
            for (const auto& e : enemies) if (e.active && e.id == id) return false;
            return true;
        }), lockedIds.end());

        if (lockScanTimer <= 0.0f && int(lockedIds.size()) < player.maxMissileLocks) {
            Vec3 fwd = player.orient.forward();
            int bestId = -1;
            float bestScore = -1.0f;
            for (const auto& e : enemies) {
                if (!e.active || e.type == EnemyType::BossHull) continue;
                Vec3 toE = e.pos - player.pos;
                float dist = toE.len();
                if (dist > 780.0f || dist < 15.0f) continue;
                float dp = dot(toE / dist, fwd);
                if (dp < 0.72f) continue; // ~44 deg lock cone
                // Prefer targets not yet locked (allow double-locking if fewer enemies than rack)
                int already = int(std::count(lockedIds.begin(), lockedIds.end(), e.id));
                float sc = dp - already * 0.45f - dist * 0.0003f;
                if (sc > bestScore) { bestScore = sc; bestId = e.id; }
            }
            if (bestId != -1) {
                lockedIds.push_back(bestId);
                lockScanTimer = 0.14f;
                float pitch = 1.0f + float(lockedIds.size() - 1) * 0.14f;
                audio.playSfx(SfxType::LockPing, 0.65f, pitch);
            }
        }
    } else if (lockingActive) {
        // Released missile key/button -> UNLEASH SWARM SALVO!
        lockingActive = false;
        if (!lockedIds.empty()) {
            player.missileCooldown = 1.15f;
            Vec3 fwd = player.orient.forward();
            Vec3 right = player.orient.right();
            Vec3 up = player.orient.up();
            for (size_t i = 0; i < lockedIds.size(); ++i) {
                for (auto& ms : missiles) {
                    if (ms.active) continue;
                    ms.active = true;
                    float side = (i % 2 == 0) ? -1.0f : 1.0f;
                    float spread = float(i / 2 + 1) * 1.4f;
                    ms.pos = player.pos + right * (side * 2.5f) - up * 0.8f;
                    // Macross-style fanning launch velocity before homing curves in!
                    ms.vel = fwd * (player.speed + 55.0f) + right * (side * 22.0f * spread) + up * rng.range(10.0f, 24.0f);
                    ms.targetId = lockedIds[i];
                    ms.life = 5.5f;
                    ms.damage = player.missileDamage;
                    ms.trailCount = 0;
                    ms.trailAccum = 0.0f;
                    break;
                }
            }
            audio.playSfx(SfxType::MissileLaunch, 0.75f, 1.0f);
            shakeAmt = std::max(shakeAmt, 0.25f);
        }
        lockedIds.clear();
    }

    // 3. Update Projectiles & Deflection / Collisions
    for (auto& pr : projectiles) {
        if (!pr.active) continue;
        pr.life -= dt;
        if (pr.life <= 0.0f) { pr.active = false; continue; }
        pr.pos += pr.vel * dt;
        if (pr.pos.y < 0.8f) {
            spawnParticle(pr.pos, {0, 14, 0}, {0.65f, 0.92f, 1.0f}, 1.5f, 0.3f);
            pr.active = false;
            continue;
        }
        if (pr.fromPlayer) {
            for (auto& e : enemies) {
                if (!e.active || e.type == EnemyType::BossHull) continue;
                float r = e.radius + pr.radius;
                if ((pr.pos - e.pos).lenSq() <= r * r) {
                    damageEnemy(e, pr.damage, pr.pos);
                    if (!pr.piercing) { pr.active = false; break; }
                }
            }
        } else {
            // Enemy plasma bolt vs Player (check Barrel Roll deflection first!)
            float dSq = (pr.pos - player.pos).lenSq();
            if (player.rollTimer > 0.0f && dSq < 12.0f * 12.0f) {
                // BARREL ROLL DEFLECT!! Reflect bolt back!
                pr.fromPlayer = true;
                pr.vel = -pr.vel * 1.85f;
                pr.color = {0.35f, 1.0f, 1.0f};
                pr.damage = 38.0f;
                pr.life = 1.8f;
                score += 150 * combo;
                audio.playSfx(SfxType::Deflect, 0.72f, 1.0f);
                spawnShockwave(player.pos, player.orient, {0.35f, 1.0f, 1.0f}, 9.0f, 0.25f);
                addCallout("DEFLECTED!", {0.35f, 1.0f, 1.0f, 1.0f}, 1.0f);
            } else if (dSq < 4.6f * 4.6f) {
                pr.active = false;
                damagePlayer(pr.damage);
            }
        }
    }

    // 4. Update Homing Swarm Missiles
    for (auto& ms : missiles) {
        if (!ms.active) continue;
        ms.life -= dt;
        if (ms.life <= 0.0f) { ms.active = false; continue; }

        // Find locked target
        Enemy* target = nullptr;
        for (auto& e : enemies) {
            if (e.active && e.id == ms.targetId) { target = &e; break; }
        }
        float speed = 255.0f;
        if (target) {
            Vec3 toT = target->pos - ms.pos;
            float dist = toT.len();
            if (dist < target->radius + 5.5f) {
                damageEnemy(*target, ms.damage, ms.pos);
                spawnExplosion(ms.pos, 1.25f, {1.0f, 0.65f, 0.20f}, false);
                ms.active = false;
                continue;
            }
            Vec3 desiredDir = toT / std::max(dist, 0.001f);
            Vec3 curDir = normalize(ms.vel);
            Vec3 newDir = normalize(lerp(curDir, desiredDir, clampf(dt * 7.5f, 0.0f, 1.0f)));
            ms.vel = newDir * speed;
        } else {
            ms.vel = normalize(ms.vel) * speed;
        }
        ms.pos += ms.vel * dt;

        // Trail history
        ms.trailAccum += dt;
        if (ms.trailAccum >= 0.016f) {
            ms.trailAccum = 0.0f;
            for (int i = Missile::TRAIL_LEN - 1; i > 0; --i) ms.trail[i] = ms.trail[i - 1];
            ms.trail[0] = ms.pos;
            if (ms.trailCount < Missile::TRAIL_LEN) ms.trailCount++;
        }
    }
}

void Game::spawnWave(int w) {
    wave = w;
    waveBannerTimer = 3.0f;
    bool isBossWave = (w % 3 == 0);

    if (isBossWave) {
        spawnBoss();
        // Spawn escort darts
        int escorts = 2 + std::min(4, w / 3);
        for (int i = 0; i < escorts; ++i) {
            float ang = TAU * float(i) / escorts;
            spawnEnemy(EnemyType::Dart, {std::cos(ang) * 320.0f, rng.range(90.0f, 160.0f), std::sin(ang) * 320.0f - 180.0f});
        }
        addCallout("WARNING: AEGIS SKY-DREADNOUGHT DETECTED!", {1.0f, 0.25f, 0.40f, 1.0f}, 3.6f);
    } else {
        int darts = 4 + w * 2;
        int lancers = (w >= 2) ? (1 + w / 2) : 0;
        for (int i = 0; i < darts; ++i) {
            float ang = rng.range(0, TAU);
            float dist = rng.range(360.0f, 680.0f);
            Vec3 p = player.pos + Vec3{std::cos(ang) * dist, rng.range(20.0f, 95.0f), std::sin(ang) * dist};
            p.y = clampf(p.y, 45.0f, 220.0f);
            spawnEnemy(EnemyType::Dart, p);
        }
        for (int i = 0; i < lancers; ++i) {
            float ang = rng.range(0, TAU);
            float dist = rng.range(420.0f, 700.0f);
            Vec3 p = player.pos + Vec3{std::cos(ang) * dist, rng.range(45.0f, 110.0f), std::sin(ang) * dist};
            p.y = clampf(p.y, 65.0f, 230.0f);
            spawnEnemy(EnemyType::Lancer, p);
        }
        addCallout("WAVE " + std::to_string(w) + " - INTERCEPT HOSTILES", {0.35f, 0.96f, 1.0f, 1.0f}, 2.4f);
    }
}

void Game::spawnEnemy(EnemyType type, const Vec3& pos) {
    for (auto& e : enemies) {
        if (e.active) continue;
        e.active = true;
        e.id = nextEnemyId++;
        e.type = type;
        e.pos = pos;
        e.vel = {0, 0, 50};
        e.orient = Quat::identity();
        e.aiTimer = rng.range(0.0f, 4.0f);
        e.aiPhase = rng.range(0.0f, TAU);
        e.hitFlash = 0.0f;
        e.parentBossIdx = -1;

        float scaleHp = 1.0f + float(wave - 1) * 0.16f;
        if (type == EnemyType::Dart) {
            e.maxHp = 34.0f * scaleHp;
            e.radius = 5.4f;
            e.fireTimer = rng.range(1.0f, 2.5f);
        } else if (type == EnemyType::Lancer) {
            e.maxHp = 82.0f * scaleHp;
            e.radius = 7.2f;
            e.fireTimer = rng.range(1.4f, 2.8f);
        }
        e.hp = e.maxHp;
        return;
    }
}

void Game::spawnBoss() {
    int bossIdx = -1;
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        if (!enemies[i].active) { bossIdx = i; break; }
    }
    if (bossIdx < 0) return;

    Enemy& b = enemies[bossIdx];
    b.active = true;
    b.id = nextEnemyId++;
    b.type = EnemyType::BossHull;
    b.pos = {0, 148, -260};
    b.vel = {0, 0, 0};
    b.orient = Quat::identity();
    b.maxHp = 1000.0f;
    b.hp = 1000.0f;
    b.radius = 28.0f;
    b.aiPhase = 0.0f;
    b.fireTimer = 1.8f;

    // Mount 4 Destructible Reactor Cores on the port/starboard sponsons!
    const Vec3 offsets[4] = {
        {-17.0f, 6.2f, -6.5f},
        { 17.0f, 6.2f, -6.5f},
        {-17.0f, 6.2f, 10.5f},
        { 17.0f, 6.2f, 10.5f}
    };
    float coreHp = 155.0f + float(wave) * 32.0f;
    for (int c = 0; c < 4; ++c) {
        for (auto& ce : enemies) {
            if (ce.active) continue;
            ce.active = true;
            ce.id = nextEnemyId++;
            ce.type = EnemyType::BossCore;
            ce.parentBossIdx = bossIdx;
            ce.localOffset = offsets[c];
            ce.pos = b.pos + offsets[c];
            ce.maxHp = coreHp;
            ce.hp = coreHp;
            ce.radius = 6.2f;
            ce.hitFlash = 0.0f;
            break;
        }
    }
}

void Game::updateEnemies(float dt) {
    // First update BossHull so its mounted Reactor Cores follow smoothly
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        Enemy& e = enemies[i];
        if (!e.active || e.type != EnemyType::BossHull) continue;
        e.aiPhase += dt * 0.14f;
        Vec3 targetPos{std::sin(e.aiPhase) * 340.0f, 148.0f + std::sin(e.aiPhase * 2.0f) * 14.0f, std::cos(e.aiPhase) * 340.0f - 100.0f};
        Vec3 move = targetPos - e.pos;
        if (move.lenSq() > 1e-4f) {
            Vec3 fwd = normalize(move);
            float yaw = std::atan2(-fwd.x, -fwd.z);
            e.orient = Quat::fromAxisAngle({0, 1, 0}, yaw);
            e.vel = move / std::max(dt, 0.01f);
            e.pos = targetPos;
        }
        // Check if any cores remain
        int coresLeft = 0;
        for (const auto& ce : enemies) {
            if (ce.active && ce.type == EnemyType::BossCore && ce.parentBossIdx == i) coresLeft++;
        }
        if (coresLeft == 0) {
            destroyEnemy(e);
            continue;
        }
        // Boss dorsal flak bursts!
        e.fireTimer -= dt;
        if (e.fireTimer <= 0.0f) {
            e.fireTimer = 1.35f;
            Vec3 toP = player.pos - e.pos;
            if (toP.len() < 780.0f) {
                Vec3 aim = normalize(toP + player.vel * 0.45f);
                for (int k = -1; k <= 1; ++k) {
                    for (auto& pr : projectiles) {
                        if (pr.active) continue;
                        pr.active = true;
                        pr.fromPlayer = false;
                        pr.pos = e.pos + Vec3{float(k) * 8.0f, 9.5f, 0};
                        pr.vel = normalize(aim + rng.unitSphere() * 0.06f) * 205.0f;
                        pr.color = {1.0f, 0.25f, 0.55f};
                        pr.damage = 14.0f;
                        pr.life = 4.2f;
                        pr.radius = 2.6f;
                        break;
                    }
                }
                audio.playSfx(SfxType::EnemyLaser, 0.45f, 0.72f);
            }
        }
    }

    // Update Boss Cores & Fighters
    for (auto& e : enemies) {
        if (!e.active) continue;
        if (e.hitFlash > 0.0f) e.hitFlash = std::max(0.0f, e.hitFlash - dt * 5.0f);

        if (e.type == EnemyType::BossCore) {
            if (e.parentBossIdx >= 0 && enemies[e.parentBossIdx].active) {
                const Enemy& boss = enemies[e.parentBossIdx];
                e.pos = boss.pos + boss.orient.rotate(e.localOffset);
                e.vel = boss.vel;
                e.orient = Quat::fromAxisAngle({0, 1, 0}, totalTime * 2.2f);
            } else {
                e.active = false;
            }
            continue;
        }
        if (e.type == EnemyType::BossHull) continue;

        // Fighter AI (Crimson Dart / Obsidian Lancer)
        e.aiTimer += dt;
        Vec3 toP = player.pos - e.pos;
        float dist = toP.len();
        bool isDart = (e.type == EnemyType::Dart);
        float maxSpd = isDart ? 86.0f : 64.0f;

        // Orbit & attack run behavior: charge in until 110m, then weave/circle cleanly!
        Vec3 desiredDir;
        if (dist > 145.0f) {
            Vec3 weave{std::sin(e.aiTimer * 1.8f + e.aiPhase) * 28.0f,
                       std::cos(e.aiTimer * 1.3f + e.aiPhase) * 18.0f,
                       std::cos(e.aiTimer * 1.6f + e.aiPhase) * 28.0f};
            desiredDir = normalize((player.pos + weave) - e.pos);
        } else {
            // Break off laterally & climb slightly
            Vec3 side = normalize(cross(toP, {0, 1, 0}));
            desiredDir = normalize(side * (std::sin(e.aiPhase) >= 0 ? 1.0f : -1.0f) - toP / std::max(dist, 1.0f) * 0.45f + Vec3{0, 0.25f, 0});
        }

        // Keep above sea & islands
        float minAlt = std::max(28.0f, world.sampleSeaIslandHeight(e.pos.x, e.pos.z) + 28.0f);
        if (e.pos.y < minAlt) desiredDir = normalize(desiredDir + Vec3{0, 0.65f, 0});

        Vec3 curDir = (e.vel.lenSq() > 1.0f) ? normalize(e.vel) : desiredDir;
        Vec3 newDir = normalize(lerp(curDir, desiredDir, clampf(dt * (isDart ? 2.6f : 1.8f), 0.0f, 1.0f)));
        e.vel = newDir * maxSpd;
        e.pos += e.vel * dt;

        // Smooth banked fighter orientation
        float yaw = std::atan2(-newDir.x, -newDir.z);
        float pitch = std::asin(clampf(newDir.y, -0.85f, 0.85f));
        Vec3 turnCross = cross(curDir, newDir);
        float bank = clampf(-turnCross.y * 12.0f, -0.85f, 0.85f);
        e.orient = (Quat::fromAxisAngle({0, 1, 0}, yaw) *
                    Quat::fromAxisAngle({1, 0, 0}, pitch) *
                    Quat::fromAxisAngle({0, 0, 1}, bank)).normalized();

        // Firing logic
        e.fireTimer -= dt;
        if (e.fireTimer <= 0.0f && dist < 440.0f && dist > 45.0f) {
            float align = dot(newDir, toP / dist);
            if (align > 0.78f) {
                e.fireTimer = isDart ? rng.range(1.5f, 2.4f) : rng.range(1.7f, 2.5f);
                int shots = isDart ? 1 : 3;
                Vec3 aim = normalize((player.pos + player.vel * (dist / 260.0f) * 0.55f) - e.pos);
                for (int s = 0; s < shots; ++s) {
                    for (auto& pr : projectiles) {
                        if (pr.active) continue;
                        pr.active = true;
                        pr.fromPlayer = false;
                        pr.pos = e.pos + newDir * 4.0f + Vec3{float(s - 1) * 1.4f, 0, 0};
                        pr.vel = normalize(aim + rng.unitSphere() * 0.035f) * 235.0f;
                        pr.color = isDart ? Vec3{1.0f, 0.22f, 0.25f} : Vec3{1.0f, 0.60f, 0.14f};
                        pr.damage = isDart ? 10.0f : 13.0f;
                        pr.life = 2.8f;
                        pr.radius = 1.8f;
                        break;
                    }
                }
                audio.playSfx(SfxType::EnemyLaser, 0.30f, isDart ? 1.1f : 0.85f);
            }
        }
    }
}

void Game::damageEnemy(Enemy& e, float dmg, const Vec3& hitPos) {
    e.hp -= dmg;
    e.hitFlash = 1.0f;
    for (int i = 0; i < 6; ++i) {
        spawnParticle(hitPos, rng.unitSphere() * 34.0f, {1.0f, 0.85f, 0.35f}, rng.range(0.8f, 1.8f), 0.32f);
    }
    if (e.hp <= 0.0f) {
        destroyEnemy(e);
    }
}

void Game::destroyEnemy(Enemy& e) {
    e.active = false;
    kills++;
    combo = std::min(12, combo + 1);
    comboTimer = 7.5f;

    if (player.vampiricHull) {
        player.hull = std::min(player.maxHull, player.hull + 5.0f);
    }

    if (e.type == EnemyType::BossHull) {
        score += 5000 * combo;
        hitstopTimer = 0.08f;
        shakeAmt = 1.5f;
        for (int k = 0; k < 5; ++k) {
            spawnExplosion(e.pos + rng.unitSphere() * 24.0f, 4.2f, {1.0f, 0.55f, 0.18f}, true);
        }
        audio.playSfx(SfxType::BigExplosion, 1.0f, 0.85f);
        addCallout("DREADNOUGHT DESTROYED! +" + std::to_string(5000 * combo), {1.0f, 0.85f, 0.22f, 1.0f}, 3.2f);
        // Sector transition on Boss defeat!
        world.setSector(world.currentSector + 1, false);
        player.hull = std::min(player.maxHull, player.hull + 35.0f);
        player.shield = player.maxShield;
    } else if (e.type == EnemyType::BossCore) {
        score += 1200 * combo;
        hitstopTimer = 0.045f;
        shakeAmt = std::max(shakeAmt, 0.95f);
        spawnExplosion(e.pos, 2.8f, {1.0f, 0.28f, 0.58f}, true);
        audio.playSfx(SfxType::BigExplosion, 0.85f, 1.15f);
        addCallout("REACTOR CORE SHATTERED!", {1.0f, 0.35f, 0.65f, 1.0f}, 1.8f);
    } else {
        int pts = (e.type == EnemyType::Lancer) ? 400 : 200;
        score += pts * combo;
        shakeAmt = std::max(shakeAmt, 0.32f);
        spawnExplosion(e.pos, (e.type == EnemyType::Lancer) ? 1.85f : 1.35f, {1.0f, 0.52f, 0.14f}, false);
        audio.playSfx(SfxType::Explosion, 0.72f, rng.range(0.90f, 1.15f));
    }
    saveHighScore();
}

void Game::damagePlayer(float dmg) {
    if (player.invulnTimer > 0.0f) return;
    player.shieldRegenDelay = 3.2f;
    shakeAmt = std::max(shakeAmt, 0.55f);

    if (player.shield > 0.0f) {
        float absorbed = std::min(player.shield, dmg);
        player.shield -= absorbed;
        dmg -= absorbed;
        player.shieldFlash = 1.0f;
        audio.playSfx(SfxType::ShieldHit, 0.70f, 1.0f);
    }
    if (dmg > 0.0f) {
        player.hull -= dmg;
        player.damageFlash = 1.0f;
        audio.playSfx(SfxType::HullHit, 0.82f, 1.0f);
        if (player.hull <= 0.0f) {
            player.hull = 0.0f;
            spawnExplosion(player.pos, 3.0f, {1.0f, 0.45f, 0.12f}, true);
            audio.playSfx(SfxType::BigExplosion, 0.95f, 1.0f);
            saveHighScore();
            state = GameState::GameOver;
        }
    }
}

void Game::updateWaves(float dt) {
    int remaining = 0;
    for (const auto& e : enemies) {
        if (e.active && e.type != EnemyType::BossHull) remaining++;
    }
    if (remaining == 0) {
        waveClearDelay += dt;
        if (waveClearDelay >= 1.8f) {
            waveClearDelay = 0.0f;
            prepareUpgradeDraft();
            state = GameState::UpgradeDraft;
            audio.playSfx(SfxType::Upgrade, 0.70f, 1.0f);
        }
    } else {
        waveClearDelay = 0.0f;
    }
}

void Game::prepareUpgradeDraft() {
    // Pick 3 distinct upgrades at random
    int pool[6] = {0, 1, 2, 3, 4, 5};
    for (int i = 5; i > 0; --i) {
        int j = rng.rangeInt(0, i);
        std::swap(pool[i], pool[j]);
    }
    for (int i = 0; i < 3; ++i) draftOptions[i] = ALL_UPGRADES[pool[i]];
    draftHover = -1;
}

void Game::applyUpgrade(int slotIndex) {
    if (slotIndex < 0 || slotIndex > 2) return;
    int id = draftOptions[slotIndex].id;
    switch (id) {
        case 0: player.fireRate = std::max(0.055f, player.fireRate * 0.78f); player.cannonDamage *= 1.15f; break;
        case 1: player.maxMissileLocks = std::min(10, player.maxMissileLocks + 2); player.missileDamage *= 1.25f; break;
        case 2: player.maxShield += 35.0f; player.shield = player.maxShield; player.shieldRegenRate *= 1.45f; break;
        case 3: player.maxBoost += 35.0f; player.boost = player.maxBoost; player.boostRecharge *= 1.45f; break;
        case 4: player.piercingBolts = true; player.cannonDamage *= 1.12f; break;
        case 5: player.vampiricHull = true; player.hull = std::min(player.maxHull, player.hull + 40.0f); break;
    }
    audio.playSfx(SfxType::Upgrade, 0.78f, 1.25f);
    state = GameState::Playing;
    spawnWave(wave + 1);
}

void Game::spawnExplosion(const Vec3& pos, float scale, const Vec3& col, bool big) {
    dynLightPos = pos;
    dynLightColor = col;
    dynLightRadius = std::max(dynLightRadius, 55.0f * scale);

    spawnShockwave(pos, Quat::fromAxisAngle(rng.unitSphere(), rng.range(0, TAU)), col, 18.0f * scale, big ? 0.75f : 0.45f);
    if (big) {
        spawnShockwave(pos, Quat::fromAxisAngle({1, 0, 0}, PI * 0.5f), {1.0f, 0.92f, 0.65f}, 26.0f * scale, 0.90f);
    }

    int pCount = (quality == QualityPreset::Fast) ? int(16 * scale) : int(30 * scale);
    pCount = std::min(pCount, 85);
    for (int i = 0; i < pCount; ++i) {
        Vec3 dir = rng.unitSphere();
        float spd = rng.range(18.0f, 88.0f) * scale;
        Vec3 pc = (i % 3 == 0) ? Vec3{1.0f, 0.95f, 0.72f} : (i % 3 == 1 ? col : Vec3{1.0f, 0.25f, 0.08f});
        spawnParticle(pos + dir * (2.0f * scale), dir * spd, pc, rng.range(1.8f, 4.4f) * scale, rng.range(0.35f, 0.95f), 2.1f);
    }

    int dCount = big ? 16 : 8;
    for (int i = 0; i < dCount; ++i) {
        for (auto& d : debris) {
            if (d.active) continue;
            d.active = true;
            d.pos = pos + rng.unitSphere() * (2.0f * scale);
            d.vel = rng.unitSphere() * rng.range(24.0f, 76.0f) * scale + Vec3{0, 12.0f, 0};
            d.rotAxis = rng.unitSphere();
            d.rotAngle = rng.range(0, TAU);
            d.rotSpeed = rng.range(3.5f, 9.0f);
            d.scale = rng.range(0.7f, 1.9f) * scale;
            d.color = (i % 2 == 0) ? Vec3{0.22f, 0.24f, 0.30f} : col;
            d.life = rng.range(1.1f, 2.2f);
            break;
        }
    }
}

void Game::spawnParticle(const Vec3& pos, const Vec3& vel, const Vec3& col, float size, float life, float drag) {
    for (auto& p : particles) {
        if (p.active) continue;
        p.active = true;
        p.pos = pos;
        p.vel = vel;
        p.color = col;
        p.size = size;
        p.life = life;
        p.maxLife = life;
        p.drag = drag;
        return;
    }
}

void Game::spawnShockwave(const Vec3& pos, const Quat& orient, const Vec3& col, float maxR, float dur) {
    for (auto& s : shockwaves) {
        if (s.active) continue;
        s.active = true;
        s.pos = pos;
        s.orient = orient;
        s.color = col;
        s.radius = 1.0f;
        s.maxRadius = maxR;
        s.life = dur;
        s.maxLife = dur;
        return;
    }
}

void Game::addCallout(const std::string& msg, const Vec4& col, float dur) {
    callout.text = msg;
    callout.color = col;
    callout.timer = dur;
    callout.maxTimer = dur;
}

void Game::updateEffects(float dt) {
    if (dynLightRadius > 0.1f) {
        dynLightRadius = std::max(0.0f, dynLightRadius - dt * 95.0f);
    }
    for (auto& p : particles) {
        if (!p.active) continue;
        p.life -= dt;
        if (p.life <= 0.0f) { p.active = false; continue; }
        p.pos += p.vel * dt;
        p.vel *= std::exp(-p.drag * dt);
    }
    for (auto& d : debris) {
        if (!d.active) continue;
        d.life -= dt;
        if (d.life <= 0.0f || d.pos.y < -5.0f) { d.active = false; continue; }
        d.vel.y -= 28.0f * dt;
        d.pos += d.vel * dt;
        d.rotAngle += d.rotSpeed * dt;
    }
    for (auto& s : shockwaves) {
        if (!s.active) continue;
        s.life -= dt;
        if (s.life <= 0.0f) { s.active = false; continue; }
        float u = 1.0f - (s.life / s.maxLife);
        s.radius = lerpf(1.2f, s.maxRadius, 1.0f - (1.0f - u) * (1.0f - u));
    }
}

// =============================================================================
// 3D RENDERING PIPELINE (HDR FBO -> Optional Fast Bloom -> ACES Post -> Native HUD)
// =============================================================================
void Game::render() {
    float aspect = float(winW) / float(std::max(1, winH));
    curView = Mat4::lookAt(camPos, camLook, camUp);
    curProj = Mat4::perspective(camFovDeg * DEG2RAD, aspect, 0.8f, 3400.0f);
    curViewProj = curProj * curView;

    // 1. Render 3D scene into internal scaled HDR FBO
    glfn::BindFramebuffer(GL_FRAMEBUFFER, sceneFbo);
    glViewport(0, 0, fboW, fboH);
    glClearColor(0.05f, 0.10f, 0.18f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderScene3D();

    // 2. Post-process & optional bloom onto full-resolution default framebuffer
    renderPostProcess();

    // 3. Crisp 1:1 Native Resolution 2D HUD & UI Overlays
    glViewport(0, 0, winW, winH);
    renderHudOverlay();
}

void Game::renderSky() {
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    const auto& pal = world.activePalette;
    Vec3 fwd = normalize(camLook - camPos);
    Vec3 right = normalize(cross(fwd, camUp));
    Vec3 up = cross(right, fwd);
    float aspect = float(winW) / float(std::max(1, winH));

    glfn::UseProgram(progSky);
    glfn::Uniform3f(glfn::GetUniformLocation(progSky, "uCamForward"), fwd.x, fwd.y, fwd.z);
    glfn::Uniform3f(glfn::GetUniformLocation(progSky, "uCamRight"), right.x, right.y, right.z);
    glfn::Uniform3f(glfn::GetUniformLocation(progSky, "uCamUp"), up.x, up.y, up.z);
    glfn::Uniform1f(glfn::GetUniformLocation(progSky, "uTanHalfFov"), std::tan(camFovDeg * 0.5f * DEG2RAD));
    glfn::Uniform1f(glfn::GetUniformLocation(progSky, "uAspect"), aspect);
    glfn::Uniform3f(glfn::GetUniformLocation(progSky, "uSunDir"), pal.sunDir.x, pal.sunDir.y, pal.sunDir.z);
    glfn::Uniform3f(glfn::GetUniformLocation(progSky, "uSunColor"), pal.sunColor.x, pal.sunColor.y, pal.sunColor.z);
    glfn::Uniform3f(glfn::GetUniformLocation(progSky, "uSkyZenith"), pal.skyZenith.x, pal.skyZenith.y, pal.skyZenith.z);
    glfn::Uniform3f(glfn::GetUniformLocation(progSky, "uSkyHorizon"), pal.skyHorizon.x, pal.skyHorizon.y, pal.skyHorizon.z);
    glfn::Uniform3f(glfn::GetUniformLocation(progSky, "uSkyBelow"), pal.skyBelow.x, pal.skyBelow.y, pal.skyBelow.z);
    glfn::Uniform1f(glfn::GetUniformLocation(progSky, "uStarIntensity"), pal.starIntensity);
    glfn::Uniform1f(glfn::GetUniformLocation(progSky, "uAuroraIntensity"), pal.auroraIntensity);
    glfn::Uniform1f(glfn::GetUniformLocation(progSky, "uTime"), totalTime);

    glfn::BindVertexArray(emptyVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glDepthMask(GL_TRUE);
}

void Game::renderOcean() {
    const auto& pal = world.activePalette;
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // Snap ocean grid center to 40m step so vertices never swim/shimmer!
    float snapX = std::floor(camPos.x / 40.0f) * 40.0f;
    float snapZ = std::floor(camPos.z / 40.0f) * 40.0f;

    glfn::UseProgram(progOcean);
    glfn::UniformMatrix4fv(glfn::GetUniformLocation(progOcean, "uViewProj"), 1, GL_FALSE, curViewProj.m);
    glfn::Uniform2f(glfn::GetUniformLocation(progOcean, "uCenterXZ"), snapX, snapZ);
    glfn::Uniform1f(glfn::GetUniformLocation(progOcean, "uTime"), totalTime);
    glfn::Uniform3f(glfn::GetUniformLocation(progOcean, "uCamPos"), camPos.x, camPos.y, camPos.z);
    glfn::Uniform3f(glfn::GetUniformLocation(progOcean, "uSunDir"), pal.sunDir.x, pal.sunDir.y, pal.sunDir.z);
    glfn::Uniform3f(glfn::GetUniformLocation(progOcean, "uSunColor"), pal.sunColor.x, pal.sunColor.y, pal.sunColor.z);
    glfn::Uniform3f(glfn::GetUniformLocation(progOcean, "uSkyZenith"), pal.skyZenith.x, pal.skyZenith.y, pal.skyZenith.z);
    glfn::Uniform3f(glfn::GetUniformLocation(progOcean, "uSkyHorizon"), pal.skyHorizon.x, pal.skyHorizon.y, pal.skyHorizon.z);
    glfn::Uniform3f(glfn::GetUniformLocation(progOcean, "uWaterDeep"), pal.waterDeep.x, pal.waterDeep.y, pal.waterDeep.z);
    glfn::Uniform3f(glfn::GetUniformLocation(progOcean, "uWaterShallow"), pal.waterShallow.x, pal.waterShallow.y, pal.waterShallow.z);
    glfn::Uniform3f(glfn::GetUniformLocation(progOcean, "uFogColor"), pal.fogColor.x, pal.fogColor.y, pal.fogColor.z);
    glfn::Uniform1f(glfn::GetUniformLocation(progOcean, "uFogDensity"), pal.fogDensity);

    world.oceanMesh.draw();
}

void Game::renderMesh(const Mesh& m, const Mat4& model, const Vec3& tint, const Vec3& emissiveAdd) {
    glfn::UniformMatrix4fv(mLocModel, 1, GL_FALSE, model.m);
    glfn::Uniform3f(mLocTint, tint.x, tint.y, tint.z);
    glfn::Uniform3f(mLocEmissiveAdd, emissiveAdd.x, emissiveAdd.y, emissiveAdd.z);
    m.draw();
}

void Game::renderScene3D() {
    renderSky();
    renderOcean();

    // Setup stylized 3D mesh shader uniforms once per frame
    const auto& pal = world.activePalette;
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDisable(GL_BLEND);

    glfn::UseProgram(progMesh);
    glfn::UniformMatrix4fv(mLocViewProj, 1, GL_FALSE, curViewProj.m);
    glfn::Uniform3f(mLocCamPos, camPos.x, camPos.y, camPos.z);
    glfn::Uniform3f(mLocSunDir, pal.sunDir.x, pal.sunDir.y, pal.sunDir.z);
    glfn::Uniform3f(mLocSunColor, pal.sunColor.x, pal.sunColor.y, pal.sunColor.z);
    glfn::Uniform3f(mLocSkyZenith, pal.skyZenith.x, pal.skyZenith.y, pal.skyZenith.z);
    glfn::Uniform3f(mLocSkyHorizon, pal.skyHorizon.x, pal.skyHorizon.y, pal.skyHorizon.z);
    glfn::Uniform3f(mLocGroundBounce, pal.groundBounce.x, pal.groundBounce.y, pal.groundBounce.z);
    glfn::Uniform3f(mLocFogColor, pal.fogColor.x, pal.fogColor.y, pal.fogColor.z);
    glfn::Uniform1f(mLocFogDensity, pal.fogDensity);
    glfn::Uniform1f(mLocTime, totalTime);
    glfn::Uniform3f(mLocPointPos, dynLightPos.x, dynLightPos.y, dynLightPos.z);
    glfn::Uniform3f(mLocPointColor, dynLightColor.x, dynLightColor.y, dynLightColor.z);
    glfn::Uniform1f(mLocPointRadius, dynLightRadius);

    // 1. Static Archipelago Terrain & Floating Sky-Islands (gentle floating bob!)
    renderMesh(world.terrainMesh, Mat4::identity());
    float flBob = std::sin(totalTime * 0.7f) * 2.2f;
    renderMesh(world.floatingMesh, Mat4::translate({0, flBob, 0}));

    // 2. Stylized 3D Clouds (with quick frustum culling)
    Vec3 camFwd = normalize(camLook - camPos);
    Vec3 cloudTint = lerp({1.0f, 1.0f, 1.0f}, pal.skyHorizon * 1.12f, 0.35f);
    for (const auto& c : world.clouds) {
        Vec3 toC = c.pos - camPos;
        if (dot(toC, camFwd) < -90.0f) continue;
        Mat4 cm = Mat4::translate(c.pos) * Mat4::rotateY(c.yaw) * Mat4::scale({c.scale, c.scale, c.scale});
        renderMesh(world.cloudMeshes[c.meshIdx], cm, cloudTint);
    }

    // 3. Ancient Energy Ring Gates (with frustum culling)
    for (const auto& g : world.ringGates) {
        if (dot(g.pos - camPos, camFwd) < -50.0f) continue;
        Vec3 r = normalize(cross({0, 1, 0}, g.forward));
        Vec3 u = cross(g.forward, r);
        Mat4 gm = Mat4::fromBasis(r, u, g.forward, g.pos);
        renderMesh(world.ringFrameMesh, gm);
        Vec3 ringCol = (g.cooldown > 0.0f) ? Vec3{0.25f, 1.0f, 0.55f} : Vec3{1.0f, 0.85f, 0.28f};
        Mat4 em = gm * Mat4::rotateZ(g.spinAngle);
        renderMesh(world.ringEnergyMesh, em, ringCol, ringCol * (0.35f + 0.25f * std::sin(totalTime * 4.0f)));
    }

    // 4. Player Valkyrie-IX Starfighter (skip in Cockpit Nose cam)
    if (state != GameState::GameOver && !(state == GameState::Playing && camMode == 2)) {
        Mat4 pm = player.orient.toMat4(player.pos);
        Vec3 shieldGlow = (player.rollTimer > 0.0f) ? Vec3{0.15f, 0.55f, 0.75f} : Vec3{0, 0, 0};
        renderMesh(meshPlayer, pm, {1, 1, 1}, shieldGlow);

        // Twin glowing plasma afterburner cones!
        glDisable(GL_CULL_FACE);
        float flick = 0.88f + 0.22f * std::sin(totalTime * 42.0f);
        float lenScale = (0.75f + player.throttle * 0.55f + player.boostSmooth * 1.55f) * flick;
        Vec3 flameTint = lerp({0.35f, 0.92f, 1.0f}, {1.0f, 0.82f, 0.35f}, player.boostSmooth);
        for (float sx : {-0.46f, 0.46f}) {
            Mat4 tm = pm * Mat4::translate({sx, -0.02f, 2.65f}) * Mat4::scale({1.0f + player.boostSmooth * 0.35f, 1.0f + player.boostSmooth * 0.35f, lenScale});
            renderMesh(meshThruster, tm, flameTint, flameTint * 0.6f);
        }
        glEnable(GL_CULL_FACE);
    }

    // 5. Enemies & Boss Dreadnought
    for (const auto& e : enemies) {
        if (!e.active) continue;
        Vec3 flashAdd = e.hitFlash > 0.01f ? Vec3{e.hitFlash * 1.1f, e.hitFlash * 0.9f, e.hitFlash * 0.7f} : Vec3{0, 0, 0};
        Mat4 em = e.orient.toMat4(e.pos);
        if (e.type == EnemyType::Dart) {
            renderMesh(meshDart, em, {1, 1, 1}, flashAdd);
        } else if (e.type == EnemyType::Lancer) {
            renderMesh(meshLancer, em, {1, 1, 1}, flashAdd);
        } else if (e.type == EnemyType::BossHull) {
            renderMesh(meshBossHull, em, {1, 1, 1}, flashAdd);
        } else if (e.type == EnemyType::BossCore) {
            float pulse = 0.92f + 0.14f * std::sin(totalTime * 6.5f);
            renderMesh(meshBossCore, em * Mat4::scale({pulse, pulse, pulse}), {1, 1, 1}, flashAdd + Vec3{0.35f, 0.05f, 0.18f});
        }
    }

    // 6. Projectiles, Missiles, Debris & Shockwaves
    for (const auto& pr : projectiles) {
        if (!pr.active) continue;
        Vec3 dir = normalize(pr.vel);
        Vec3 right = normalize(cross(dir, {0, 1, 0}));
        Vec3 up = cross(right, dir);
        Mat4 bm = Mat4::fromBasis(right, up, dir, pr.pos) * Mat4::scale({1.0f, 1.0f, pr.fromPlayer ? 1.85f : 1.15f});
        renderMesh(meshBolt, bm, pr.color, pr.color * 0.9f);
    }
    for (const auto& ms : missiles) {
        if (!ms.active) continue;
        Vec3 dir = normalize(ms.vel);
        Vec3 right = normalize(cross(dir, {0, 1, 0}));
        Vec3 up = cross(right, dir);
        Mat4 mm = Mat4::fromBasis(right, up, dir, ms.pos) * Mat4::scale({1.25f, 1.25f, 1.25f});
        renderMesh(meshMissile, mm);
    }
    for (const auto& d : debris) {
        if (!d.active) continue;
        Mat4 dm = Mat4::translate(d.pos) * Quat::fromAxisAngle(d.rotAxis, d.rotAngle).toMat4() * Mat4::scale({d.scale, d.scale, d.scale});
        renderMesh(meshDebris, dm, d.color);
    }

    // Additive shockwave rings
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    for (const auto& s : shockwaves) {
        if (!s.active) continue;
        float alpha = clampf(s.life / s.maxLife, 0.0f, 1.0f);
        Mat4 sm = s.orient.toMat4(s.pos) * Mat4::scale({s.radius, s.radius, s.radius});
        renderMesh(meshShockRing, sm, s.color * alpha, s.color * alpha);
    }

    // 7. Additive Particles + Wingtip & Missile Ribbons
    renderFxBatch();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void Game::renderFxBatch() {
    fxVerts.clear();

    Vec3 camRight{curView.m[0], curView.m[4], curView.m[8]};
    Vec3 camUpV{curView.m[1], curView.m[5], curView.m[9]};

    auto pushQuad = [&](const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d, const Vec4& ca, const Vec4& cb) {
        if (fxVerts.size() + 6 > 65500) return;
        fxVerts.push_back({a.x, a.y, a.z, ca.x, ca.y, ca.z, ca.w});
        fxVerts.push_back({b.x, b.y, b.z, ca.x, ca.y, ca.z, ca.w});
        fxVerts.push_back({c.x, c.y, c.z, cb.x, cb.y, cb.z, cb.w});
        fxVerts.push_back({a.x, a.y, a.z, ca.x, ca.y, ca.z, ca.w});
        fxVerts.push_back({c.x, c.y, c.z, cb.x, cb.y, cb.z, cb.w});
        fxVerts.push_back({d.x, d.y, d.z, cb.x, cb.y, cb.z, cb.w});
    };

    // Player wingtip vapor ribbons!
    auto emitRibbon = [&](const Vec3* pts, int count, float baseWidth, const Vec3& col, float alphaScale) {
        for (int i = 0; i < count - 1; ++i) {
            float t0 = 1.0f - float(i) / float(count);
            float t1 = 1.0f - float(i + 1) / float(count);
            Vec3 seg = pts[i + 1] - pts[i];
            if (seg.lenSq() > 400.0f) continue;
            Vec3 side = normalize(cross(seg, camPos - pts[i]));
            float w0 = baseWidth * t0;
            float w1 = baseWidth * t1;
            Vec4 c0{col.x, col.y, col.z, t0 * t0 * alphaScale};
            Vec4 c1{col.x, col.y, col.z, t1 * t1 * alphaScale};
            pushQuad(pts[i] - side * w0, pts[i] + side * w0, pts[i + 1] + side * w1, pts[i + 1] - side * w1, c0, c1);
        }
    };

    if (state != GameState::GameOver && player.trailCount > 2) {
        Vec3 trailCol = lerp({0.42f, 0.92f, 1.0f}, {1.0f, 0.80f, 0.28f}, player.boostSmooth);
        float w = 0.24f + player.boostSmooth * 0.34f + std::abs(player.visualBank) * 0.16f;
        float a = 0.36f + player.boostSmooth * 0.42f;
        emitRibbon(player.trailL, player.trailCount, w, trailCol, a);
        emitRibbon(player.trailR, player.trailCount, w, trailCol, a);
    }

    // Homing missile smoke/plasma ribbons!
    for (const auto& ms : missiles) {
        if (!ms.active || ms.trailCount < 2) continue;
        emitRibbon(ms.trail, ms.trailCount, 0.55f, {1.0f, 0.72f, 0.28f}, 0.85f);
    }

    // Glowing camera-facing diamond particles
    for (const auto& p : particles) {
        if (!p.active) continue;
        float u = clampf(p.life / p.maxLife, 0.0f, 1.0f);
        float sz = p.size * (0.35f + 0.65f * u);
        Vec3 r = camRight * sz;
        Vec3 up = camUpV * sz;
        Vec4 col{p.color.x, p.color.y, p.color.z, u * 0.92f};
        pushQuad(p.pos - r, p.pos + up, p.pos + r, p.pos - up, col, col);
    }

    if (fxVerts.empty()) return;
    glfn::UseProgram(progFx);
    glfn::UniformMatrix4fv(glfn::GetUniformLocation(progFx, "uViewProj"), 1, GL_FALSE, curViewProj.m);
    glfn::BindVertexArray(fxVao);
    glfn::BindBuffer(GL_ARRAY_BUFFER, fxVbo);
    glfn::BufferSubData(GL_ARRAY_BUFFER, 0, GLsizeiptr(fxVerts.size() * sizeof(FxVert)), fxVerts.data());
    glDrawArrays(GL_TRIANGLES, 0, GLsizei(fxVerts.size()));
    glfn::BindVertexArray(0);
}

void Game::renderPostProcess() {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    if (quality == QualityPreset::Fast) {
        // Ultra-fast software-friendly composite pass
        glfn::BindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, winW, winH);
        glfn::UseProgram(progPostFast);
        glfn::ActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sceneColorTex);
        glfn::Uniform1i(glfn::GetUniformLocation(progPostFast, "uScene"), 0);
        glfn::Uniform1f(glfn::GetUniformLocation(progPostFast, "uDamageFlash"), player.damageFlash);
        glfn::Uniform1f(glfn::GetUniformLocation(progPostFast, "uShieldFlash"), player.shieldFlash);
        glfn::BindVertexArray(emptyVao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glfn::BindVertexArray(0);
        return;
    }

    bool useBloom = true;
    if (useBloom) {
        // 1. Bright pass extract into bloomFbo[0]
        glfn::BindFramebuffer(GL_FRAMEBUFFER, bloomFbo[0]);
        glViewport(0, 0, bloomW, bloomH);
        glfn::UseProgram(progBloomExtract);
        glfn::ActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sceneColorTex);
        glfn::Uniform1i(glfn::GetUniformLocation(progBloomExtract, "uScene"), 0);
        glfn::BindVertexArray(emptyVao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // 2. Separable Gaussian Blur (Horizontal -> Vertical)
        glfn::UseProgram(progBlur);
        glfn::Uniform1i(glfn::GetUniformLocation(progBlur, "uTex"), 0);
        GLint locDir = glfn::GetUniformLocation(progBlur, "uDir");

        glfn::BindFramebuffer(GL_FRAMEBUFFER, bloomFbo[1]);
        glBindTexture(GL_TEXTURE_2D, bloomTex[0]);
        glfn::Uniform2f(locDir, 1.4f / float(bloomW), 0.0f);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfn::BindFramebuffer(GL_FRAMEBUFFER, bloomFbo[0]);
        glBindTexture(GL_TEXTURE_2D, bloomTex[1]);
        glfn::Uniform2f(locDir, 0.0f, 1.4f / float(bloomH));
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    // 3. Final ACES Filmic Composite to Screen
    glfn::BindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, winW, winH);
    glfn::UseProgram(progPost);
    glfn::ActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneColorTex);
    glfn::Uniform1i(glfn::GetUniformLocation(progPost, "uScene"), 0);
    glfn::ActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, bloomTex[0]);
    glfn::Uniform1i(glfn::GetUniformLocation(progPost, "uBloom"), 1);

    glfn::Uniform1f(glfn::GetUniformLocation(progPost, "uBloomAmt"), quality == QualityPreset::High ? 0.56f : 0.42f);
    glfn::Uniform1f(glfn::GetUniformLocation(progPost, "uBoostAmt"), state == GameState::Playing ? player.boostSmooth : 0.0f);
    glfn::Uniform1f(glfn::GetUniformLocation(progPost, "uDamageFlash"), player.damageFlash);
    glfn::Uniform1f(glfn::GetUniformLocation(progPost, "uShieldFlash"), player.shieldFlash);
    glfn::Uniform1f(glfn::GetUniformLocation(progPost, "uTime"), totalTime);

    glfn::BindVertexArray(emptyVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glfn::BindVertexArray(0);
}

bool Game::projectToScreen(const Vec3& worldPos, Vec2& outScreen, float& outDepth) const {
    Vec4 clip = curViewProj.mulVec4({worldPos.x, worldPos.y, worldPos.z, 1.0f});
    outDepth = clip.w;
    if (clip.w <= 0.1f) return false;
    float ndcX = clip.x / clip.w;
    float ndcY = clip.y / clip.w;
    outScreen.x = (ndcX * 0.5f + 0.5f) * float(winW);
    outScreen.y = (0.5f - ndcY * 0.5f) * float(winH);
    return (ndcX >= -1.05f && ndcX <= 1.05f && ndcY >= -1.05f && ndcY <= 1.05f);
}

// =============================================================================
// 2D NATIVE-RES HUD & UI OVERLAYS
// =============================================================================
void Game::renderHudOverlay() {
    hud.begin(winW, winH);
    switch (state) {
        case GameState::Title:        renderTitleUI(); break;
        case GameState::Playing:      renderFlightHUD(); break;
        case GameState::UpgradeDraft: renderFlightHUD(); renderUpgradeUI(); break;
        case GameState::Paused:       renderFlightHUD(); renderPauseUI(); break;
        case GameState::GameOver:     renderGameOverUI(); break;
    }
    hud.flush();
}

void Game::renderTitleUI() {
    float cx = winW * 0.5f;
    const Vec4 cyan{0.25f, 0.94f, 1.0f, 1.0f};
    const Vec4 gold{1.0f, 0.82f, 0.28f, 1.0f};
    const Vec4 white{0.96f, 0.98f, 1.0f, 1.0f};
    const Vec4 bg{0.04f, 0.08f, 0.15f, 0.68f};

    // Hero Title Banner
    float titleY = winH * 0.16f;
    hud.textCentered(cx, titleY, 6.2f, cyan, "AETHERWING");
    hud.line(cx - 210, titleY + 58, cx + 210, titleY + 58, 2.0f, {0.25f, 0.94f, 1.0f, 0.7f});
    hud.textCentered(cx, titleY + 70, 2.0f, gold, "SKYBOUND ARCHIPELAGO STARFIGHTER");

    // Sector Preview Badge
    char secBuf[96];
    std::snprintf(secBuf, sizeof(secBuf), "SECTOR: %s  [1/2/3 OR T]", world.activePalette.name);
    hud.textCentered(cx, titleY + 104, 1.5f, {0.82f, 0.92f, 1.0f, 0.9f}, secBuf);

    // Primary Play Buttons
    float by1 = winH * 0.58f;
    float by2 = winH * 0.67f;
    hud.panel(cx - 180, by1, 360, 48, titleHover == 0 ? Vec4{0.10f, 0.34f, 0.48f, 0.88f} : bg, cyan);
    hud.textCentered(cx, by1 + 16, 2.2f, white, "[ENTER] LAUNCH ASSAULT");

    hud.panel(cx - 180, by2, 360, 44, titleHover == 1 ? Vec4{0.22f, 0.28f, 0.18f, 0.88f} : bg, gold);
    hud.textCentered(cx, by2 + 15, 1.9f, gold, "[F] FREE FLIGHT TOUR");

    // Compact Controls Strip at bottom (generous padding so text stays cleanly inside brackets)
    float py = winH - 118.0f;
    float pw = std::min(920.0f, float(winW) - 48.0f);
    hud.panel(cx - pw * 0.5f, py, pw, 92, {0.03f, 0.06f, 0.12f, 0.78f}, {0.25f, 0.92f, 1.0f, 0.85f});
    hud.textCentered(cx, py + 14, 1.38f, cyan, "FLIGHT: MOUSE OR ARROWS/WASD   |   SHIFT: AFTERBURNER   |   SPACE: BARREL ROLL DEFLECT");
    hud.textCentered(cx, py + 40, 1.38f, white, "LMB / Z: PLASMA CANNONS   |   HOLD RMB / X: MULTI-LOCK SWARM MISSILES   |   C: CAMERA");
    char sysBuf[128];
    std::snprintf(sysBuf, sizeof(sysBuf), "HIGH SCORE: %d   |   [F1] QUALITY: %s   |   [M] MUSIC   |   [F11] FULLSCREEN   |   FPS: %.0f",
                  highScore, qualityName(), currentFps);
    hud.textCentered(cx, py + 66, 1.28f, {0.72f, 0.86f, 0.95f, 0.92f}, sysBuf);
}

void Game::renderFlightHUD() {
    const Vec4 cyan{0.25f, 0.95f, 1.0f, 0.92f};
    const Vec4 gold{1.0f, 0.84f, 0.25f, 0.95f};
    const Vec4 red{1.0f, 0.25f, 0.30f, 0.95f};
    const Vec4 magenta{1.0f, 0.28f, 0.68f, 0.98f};
    const Vec4 panelBg{0.03f, 0.07f, 0.13f, 0.62f};

    float cx = winW * 0.5f;
    float cy = winH * 0.5f;

    // 1. Central Diegetic Reticle + Cursor-Steer Indicator
    Vec2 aimPt{cx, cy};
    float tmpD = 0.0f;
    projectToScreen(player.pos + player.orient.forward() * 140.0f, aimPt, tmpD);

    // Subtle artificial horizon brackets around nose aim point
    hud.arcOutline(aimPt.x, aimPt.y, 24.0f, -0.55f, 0.55f, 8, 2.0f, {0.25f, 0.95f, 1.0f, 0.72f});
    hud.arcOutline(aimPt.x, aimPt.y, 24.0f, PI - 0.55f, PI + 0.55f, 8, 2.0f, {0.25f, 0.95f, 1.0f, 0.72f});
    hud.rect(aimPt.x - 2.0f, aimPt.y - 2.0f, 4.0f, 4.0f, cyan);

    if (input.usingMouseSteer && state == GameState::Playing) {
        float mx = cx + input.steerX * (winW * 0.44f);
        float my = cy + input.steerY * (winH * 0.44f);
        hud.line(aimPt.x, aimPt.y, mx, my, 1.2f, {0.25f, 0.95f, 1.0f, 0.28f});
        hud.circleOutline(mx, my, 8.0f, 12, 1.6f, {0.95f, 0.98f, 1.0f, 0.78f});
    }

    // Lock-on cone circle when holding Right-Click / K / X
    if (lockingActive) {
        float rLock = std::min(winW, winH) * 0.22f;
        hud.circleOutline(cx, cy, rLock, 32, 2.0f, {1.0f, 0.85f, 0.25f, 0.65f});
        char lkBuf[48];
        std::snprintf(lkBuf, sizeof(lkBuf), "SWARM LOCK %d/%d", int(lockedIds.size()), player.maxMissileLocks);
        hud.textCentered(cx, cy + rLock + 12.0f, 1.6f, gold, lkBuf);
    }

    // 2. 3D-Projected Enemy Target Brackets, Lead Pips & Off-Screen Edge Chevrons
    float bossTotalHp = 0.0f, bossMaxHp = 0.0f;
    for (const auto& e : enemies) {
        if (!e.active || e.type == EnemyType::BossHull) continue;
        if (e.type == EnemyType::BossCore) {
            bossTotalHp += std::max(0.0f, e.hp);
            bossMaxHp += e.maxHp;
        }
        int lockCount = int(std::count(lockedIds.begin(), lockedIds.end(), e.id));
        Vec2 sp;
        float depth = 0.0f;
        bool onScreen = projectToScreen(e.pos, sp, depth);
        float dist = (e.pos - player.pos).len();
        Vec4 col = (e.type == EnemyType::BossCore) ? magenta : (e.type == EnemyType::Lancer ? gold : red);

        if (onScreen && depth > 0.1f) {
            float sz = clampf(5200.0f / std::max(dist, 40.0f), 14.0f, 34.0f);
            // Corner target bracket
            hud.rectOutline(sp.x - sz, sp.y - sz, sz * 2, sz * 2, 1.8f, col);
            // Tiny HP bar above target
            hud.bar(sp.x - sz, sp.y - sz - 8.0f, sz * 2, 5.0f, e.hp / std::max(1.0f, e.maxHp), col, {0, 0, 0, 0.55f});

            // Predictive Lead Pip for close/mid range targets!
            if (dist < 460.0f && e.type != EnemyType::BossCore) {
                Vec2 lp; float ld = 0.0f;
                float tFl = dist / (player.piercingBolts ? 540.0f : 420.0f);
                if (projectToScreen(e.pos + e.vel * (tFl * 0.78f), lp, ld)) {
                    hud.circleOutline(lp.x, lp.y, 6.0f, 8, 1.5f, {1.0f, 0.92f, 0.45f, 0.75f});
                    hud.line(sp.x, sp.y, lp.x, lp.y, 1.0f, {1.0f, 0.92f, 0.45f, 0.35f});
                }
            }

            // Rotating lock-on diamond!
            if (lockCount > 0) {
                hud.diamondOutline(sp.x, sp.y, sz + 9.0f, 2.4f, gold);
                char lb[16];
                std::snprintf(lb, sizeof(lb), "X%d", lockCount);
                hud.text(sp.x + sz + 5.0f, sp.y - 6.0f, 1.4f, gold, lb);
            }
        } else {
            // Off-screen directional indicator clamped to screen margin!
            Vec3 toE = normalize(e.pos - camPos);
            Vec3 camF = normalize(camLook - camPos);
            Vec3 camR = normalize(cross(camF, camUp));
            Vec3 camU = cross(camR, camF);
            float sx = dot(toE, camR);
            float sy = -dot(toE, camU);
            float ang = std::atan2(sy, sx);
            float ex = cx + std::cos(ang) * (winW * 0.44f);
            float ey = cy + std::sin(ang) * (winH * 0.42f);
            hud.diamondOutline(ex, ey, 7.0f, 2.0f, {col.x, col.y, col.z, 0.8f});
        }
    }

    // 3. Top Telemetry Bar (Score, Combo, Wave / Boss Bar, Sector & FPS)
    char buf[128];
    hud.panel(18, 16, 265, 68, panelBg, cyan);
    std::snprintf(buf, sizeof(buf), "SCORE %07d", score);
    hud.text(30, 26, 2.0f, {1, 1, 1, 1}, buf);
    std::snprintf(buf, sizeof(buf), "COMBO X%d   KILLS %d", combo, kills);
    hud.text(30, 54, 1.45f, combo > 1 ? gold : cyan, buf);

    // Center Top: Boss HP bar or Wave / Sector Status
    if (bossMaxHp > 1.0f) {
        hud.panel(cx - 240, 16, 480, 58, panelBg, magenta);
        hud.textCentered(cx, 23, 1.6f, magenta, "AEGIS SKY-DREADNOUGHT REACTOR CORES");
        hud.bar(cx - 220, 46, 440, 18, bossTotalHp / bossMaxHp, magenta, {0.10f, 0.04f, 0.10f, 0.8f});
    } else {
        hud.panel(cx - 185, 16, 370, 52, panelBg, cyan);
        if (mode == GameMode::Assault) {
            int rem = 0;
            for (const auto& e : enemies) if (e.active && e.type != EnemyType::BossHull) rem++;
            std::snprintf(buf, sizeof(buf), "WAVE %d  |  HOSTILES: %d", wave, rem);
            hud.textCentered(cx, 25, 1.75f, {1, 1, 1, 1}, buf);
        } else {
            std::snprintf(buf, sizeof(buf), "FREE FLIGHT  |  RINGS: %d", ringsCleared);
            hud.textCentered(cx, 25, 1.75f, gold, buf);
        }
        hud.textCentered(cx, 48, 1.3f, cyan, world.activePalette.name);
    }

    // Top-Right System Pill (FPS / Quality / Audio)
    hud.panel(winW - 220, 16, 202, 46, panelBg, {0.4f, 0.85f, 1.0f, 0.7f});
    std::snprintf(buf, sizeof(buf), "%.0f FPS  [%s]", currentFps, qualityName());
    hud.textRight(winW - 30, 24, 1.45f, cyan, buf);
    hud.textRight(winW - 30, 44, 1.2f, {0.75f, 0.86f, 0.95f, 0.85f}, "[F1] QUAL  [P] PAUSE");

    // 4. Bottom-Left Aircraft Vitals (Hull, Shield, Afterburner, Speed/Alt, Roll)
    float blY = winH - 134.0f;
    hud.panel(18, blY, 290, 116, panelBg, cyan);
    hud.text(30, blY + 12, 1.35f, {0.32f, 0.92f, 1.0f, 1.0f}, "SHIELD");
    hud.bar(106, blY + 10, 188, 16, player.shield / player.maxShield, {0.22f, 0.88f, 1.0f, 0.95f}, {0.04f, 0.12f, 0.18f, 0.8f});

    hud.text(30, blY + 36, 1.35f, {0.36f, 1.0f, 0.56f, 1.0f}, "HULL");
    Vec4 hullCol = (player.hull < 30.0f) ? red : Vec4{0.30f, 0.95f, 0.52f, 0.95f};
    hud.bar(106, blY + 34, 188, 16, player.hull / player.maxHull, hullCol, {0.06f, 0.14f, 0.08f, 0.8f});

    hud.text(30, blY + 60, 1.35f, gold, "BOOST");
    hud.bar(106, blY + 58, 188, 16, player.boost / player.maxBoost, gold, {0.16f, 0.12f, 0.04f, 0.8f});

    std::snprintf(buf, sizeof(buf), "SPD %3.0f   ALT %3.0f   %s",
                  player.speed * 3.6f, player.pos.y,
                  player.rollCooldown <= 0.0f ? "ROLL RDY" : "ROLLING");
    hud.text(30, blY + 88, 1.35f, {0.9f, 0.96f, 1.0f, 0.95f}, buf);

    // 5. Bottom-Right 360 Tactical Radar Minimap!
    float radR = 64.0f;
    float rcx = winW - radR - 24.0f;
    float rcy = winH - radR - 24.0f;
    hud.rect(rcx - radR - 6, rcy - radR - 6, radR * 2 + 12, radR * 2 + 12, {0.02f, 0.06f, 0.11f, 0.65f});
    hud.circleOutline(rcx, rcy, radR, 28, 2.0f, {0.25f, 0.92f, 1.0f, 0.78f});
    hud.circleOutline(rcx, rcy, radR * 0.5f, 20, 1.0f, {0.25f, 0.92f, 1.0f, 0.25f});
    hud.line(rcx - radR, rcy, rcx + radR, rcy, 1.0f, {0.25f, 0.92f, 1.0f, 0.20f});
    hud.line(rcx, rcy - radR, rcx, rcy + radR, 1.0f, {0.25f, 0.92f, 1.0f, 0.20f});

    Vec3 pFwd = player.orient.forward();
    float hYaw = std::atan2(-pFwd.x, -pFwd.z);
    float ch = std::cos(hYaw), sh = std::sin(hYaw);
    auto worldToRadar = [&](const Vec3& wp, float& rx, float& ry) {
        float dx = (wp.x - player.pos.x) / 920.0f;
        float dz = (wp.z - player.pos.z) / 920.0f;
        float lx = dx * ch - dz * sh;
        float lz = dx * sh + dz * ch;
        float d = std::sqrt(lx * lx + lz * lz);
        if (d > 0.94f) { lx = lx / d * 0.94f; lz = lz / d * 0.94f; }
        rx = rcx + lx * radR;
        ry = rcy + lz * radR;
    };

    // Ring gates on radar (gold dots)
    for (const auto& g : world.ringGates) {
        float rx, ry; worldToRadar(g.pos, rx, ry);
        hud.rect(rx - 2.0f, ry - 2.0f, 4.0f, 4.0f, {1.0f, 0.85f, 0.25f, 0.75f});
    }
    // Enemies on radar
    for (const auto& e : enemies) {
        if (!e.active || e.type == EnemyType::BossCore) continue;
        float rx, ry; worldToRadar(e.pos, rx, ry);
        if (e.type == EnemyType::BossHull) {
            hud.diamondOutline(rx, ry, 6.0f, 2.2f, magenta);
        } else {
            hud.rect(rx - 2.5f, ry - 2.5f, 5.0f, 5.0f, e.type == EnemyType::Lancer ? gold : red);
        }
    }
    // Player center chevron
    hud.diamondOutline(rcx, rcy, 4.0f, 2.0f, cyan);

    // 6. Center Event Callout Banner
    if (callout.timer > 0.0f) {
        float a = clampf(callout.timer * 1.5f, 0.0f, 1.0f);
        Vec4 cc = callout.color;
        cc.w *= a;
        hud.textCentered(cx, winH * 0.24f, 2.2f, cc, callout.text.c_str());
    }
}

void Game::renderUpgradeUI() {
    float cx = winW * 0.5f;
    hud.rect(0, 0, float(winW), float(winH), {0.02f, 0.05f, 0.10f, 0.56f});
    hud.textCentered(cx, winH * 0.24f, 3.2f, {1.0f, 0.86f, 0.28f, 1.0f}, "WAVE CLEARED! SELECT UPGRADE");
    hud.textCentered(cx, winH * 0.31f, 1.6f, {0.75f, 0.92f, 1.0f, 0.95f}, "PRESS [1] [2] [3] OR CLICK A MODULE");

    float cardW = std::min(290.0f, (winW - 140.0f) / 3.0f);
    float cardH = 190.0f;
    float gap = 24.0f;
    float startX = cx - (cardW * 3 + gap * 2) * 0.5f;
    float startY = winH * 0.42f;

    for (int i = 0; i < 3; ++i) {
        float x = startX + i * (cardW + gap);
        bool hov = (draftHover == i);
        Vec4 accent = hov ? Vec4{1.0f, 0.85f, 0.28f, 1.0f} : Vec4{0.25f, 0.92f, 1.0f, 0.95f};
        Vec4 bg = hov ? Vec4{0.08f, 0.22f, 0.34f, 0.92f} : Vec4{0.04f, 0.09f, 0.17f, 0.88f};
        hud.panel(x, startY, cardW, cardH, bg, accent);

        char keyLbl[16];
        std::snprintf(keyLbl, sizeof(keyLbl), "[ %d ]", i + 1);
        hud.textCentered(x + cardW * 0.5f, startY + 22, 2.2f, accent, keyLbl);
        hud.textCentered(x + cardW * 0.5f, startY + 68, 1.75f, {1, 1, 1, 1}, draftOptions[i].name);

        // Simple 2-line wrap for description
        std::string desc = draftOptions[i].desc;
        size_t amp = desc.find('&');
        if (amp != std::string::npos) {
            hud.textCentered(x + cardW * 0.5f, startY + 114, 1.25f, {0.78f, 0.92f, 1.0f, 0.95f}, desc.substr(0, amp).c_str());
            hud.textCentered(x + cardW * 0.5f, startY + 138, 1.25f, {0.78f, 0.92f, 1.0f, 0.95f}, desc.substr(amp).c_str());
        } else {
            hud.textCentered(x + cardW * 0.5f, startY + 122, 1.25f, {0.78f, 0.92f, 1.0f, 0.95f}, desc.c_str());
        }
    }
}

void Game::renderPauseUI() {
    float cx = winW * 0.5f, cy = winH * 0.5f;
    hud.rect(0, 0, float(winW), float(winH), {0.02f, 0.04f, 0.09f, 0.62f});
    hud.panel(cx - 230, cy - 125, 460, 250, {0.04f, 0.08f, 0.15f, 0.92f}, {0.25f, 0.95f, 1.0f, 1.0f});
    hud.textCentered(cx, cy - 95, 3.2f, {0.25f, 0.95f, 1.0f, 1.0f}, "PAUSED");
    hud.textCentered(cx, cy - 30, 1.8f, {1, 1, 1, 1}, "[ESC / P] RESUME FLIGHT");
    hud.textCentered(cx, cy + 8,  1.8f, {1.0f, 0.85f, 0.30f, 1.0f}, "[R] RESTART SORTIE");
    hud.textCentered(cx, cy + 46, 1.8f, {0.8f, 0.88f, 1.0f, 1.0f}, "[F1] CYCLE QUALITY  |  [M] MUSIC");
    hud.textCentered(cx, cy + 84, 1.8f, {1.0f, 0.42f, 0.42f, 1.0f}, "[Q] RETURN TO TITLE");
}

void Game::renderGameOverUI() {
    float cx = winW * 0.5f, cy = winH * 0.5f;
    hud.rect(0, 0, float(winW), float(winH), {0.06f, 0.02f, 0.04f, 0.55f});
    hud.panel(cx - 260, cy - 140, 520, 280, {0.06f, 0.05f, 0.10f, 0.90f}, {1.0f, 0.30f, 0.35f, 1.0f});
    hud.textCentered(cx, cy - 110, 3.4f, {1.0f, 0.30f, 0.35f, 1.0f}, "AIRCRAFT LOST");
    char buf[96];
    std::snprintf(buf, sizeof(buf), "FINAL SCORE: %d", score);
    hud.textCentered(cx, cy - 42, 2.2f, {1, 1, 1, 1}, buf);
    std::snprintf(buf, sizeof(buf), "HIGH SCORE: %d   |   WAVE %d   |   KILLS %d", highScore, wave, kills);
    hud.textCentered(cx, cy + 2, 1.55f, {1.0f, 0.86f, 0.32f, 1.0f}, buf);
    hud.textCentered(cx, cy + 56, 1.9f, {0.25f, 0.95f, 1.0f, 1.0f}, "[ENTER / CLICK] FLY AGAIN");
    hud.textCentered(cx, cy + 94, 1.6f, {0.75f, 0.85f, 0.95f, 0.9f}, "[ESC] TITLE SCREEN");
}
