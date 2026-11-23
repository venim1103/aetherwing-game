#pragma once
#include "math3d.hpp"
#include "gl_loader.hpp"
#include "mesh.hpp"
#include "world.hpp"
#include "hud.hpp"
#include "audio.hpp"
#include <SDL3/SDL.h>
#include <vector>
#include <string>

enum class GameState {
    Title,
    Playing,
    UpgradeDraft,
    Paused,
    GameOver
};

enum class GameMode {
    Assault,
    FreeFlight
};

enum class QualityPreset {
    Fast = 0,
    Balanced = 1,
    High = 2
};

enum class EnemyType {
    Dart,
    Lancer,
    BossCore,
    BossHull
};

struct UpgradeOption {
    int id;
    const char* name;
    const char* desc;
};

struct Player {
    Vec3 pos{0, 62, 160};
    Vec3 vel{0, 0, -82};
    Quat orient = Quat::identity();
    float speed = 82.0f;
    float throttle = 0.55f;
    float visualBank = 0.0f;
    float pitchInput = 0.0f;
    float yawInput = 0.0f;

    float hull = 100.0f, maxHull = 100.0f;
    float shield = 100.0f, maxShield = 100.0f;
    float shieldRegenDelay = 0.0f;
    float shieldRegenRate = 14.0f;

    float boost = 100.0f, maxBoost = 100.0f;
    float boostRecharge = 20.0f;
    float boostSmooth = 0.0f;
    bool boosting = false;

    float rollTimer = 0.0f;
    float rollCooldown = 0.0f;
    float rollDir = 1.0f;

    float fireCooldown = 0.0f;
    float fireRate = 0.105f;
    float cannonDamage = 18.0f;
    bool piercingBolts = false;
    bool vampiricHull = false;
    int maxMissileLocks = 4;
    float missileDamage = 58.0f;
    float missileCooldown = 0.0f;

    float invulnTimer = 0.0f;
    float damageFlash = 0.0f;
    float shieldFlash = 0.0f;
    float waveSkimTimer = 0.0f;
    int wingToggle = 0;

    // Wingtip vapor trail histories
    static constexpr int TRAIL_LEN = 38;
    Vec3 trailL[TRAIL_LEN];
    Vec3 trailR[TRAIL_LEN];
    int trailCount = 0;
    float trailSampleAccum = 0.0f;
};

struct Enemy {
    bool active = false;
    int id = 0;
    EnemyType type = EnemyType::Dart;
    Vec3 pos;
    Vec3 vel;
    Quat orient = Quat::identity();
    float hp = 40.0f, maxHp = 40.0f;
    float radius = 5.0f;
    float fireTimer = 1.2f;
    float aiTimer = 0.0f;
    float aiPhase = 0.0f;
    float hitFlash = 0.0f;
    int parentBossIdx = -1; // For BossCore
    Vec3 localOffset;       // Offset relative to BossHull
};

struct Projectile {
    bool active = false;
    bool fromPlayer = true;
    bool piercing = false;
    Vec3 pos;
    Vec3 vel;
    Vec3 color;
    float damage = 15.0f;
    float life = 2.0f;
    float radius = 1.6f;
};

struct Missile {
    bool active = false;
    Vec3 pos;
    Vec3 vel;
    int targetId = -1;
    float life = 5.5f;
    float damage = 58.0f;
    static constexpr int TRAIL_LEN = 18;
    Vec3 trail[TRAIL_LEN];
    int trailCount = 0;
    float trailAccum = 0.0f;
};

struct Particle {
    bool active = false;
    Vec3 pos;
    Vec3 vel;
    Vec3 color;
    float size = 1.0f;
    float life = 0.0f;
    float maxLife = 1.0f;
    float drag = 1.5f;
};

struct Debris {
    bool active = false;
    Vec3 pos;
    Vec3 vel;
    Vec3 rotAxis;
    float rotAngle = 0.0f;
    float rotSpeed = 4.0f;
    float scale = 1.0f;
    Vec3 color;
    float life = 2.0f;
};

struct Shockwave {
    bool active = false;
    Vec3 pos;
    Quat orient = Quat::identity();
    Vec3 color;
    float radius = 1.0f;
    float maxRadius = 24.0f;
    float life = 0.0f;
    float maxLife = 0.55f;
};

struct Callout {
    std::string text;
    Vec4 color{0.35f, 0.95f, 1.0f, 1.0f};
    float timer = 0.0f;
    float maxTimer = 2.0f;
};

struct InputState {
    float mouseX = 640.0f, mouseY = 360.0f;
    float steerX = 0.0f, steerY = 0.0f; // [-1..1] normalized cursor steer
    bool usingMouseSteer = false;
    bool mouseLeft = false;
    bool mouseRight = false;
    bool mouseRightPrev = false;
    bool mouseClicked = false;
    float wheelDelta = 0.0f;

    bool keyW = false, keyS = false, keyA = false, keyD = false;
    bool keyUp = false, keyDown = false, keyLeft = false, keyRight = false;
    bool keyShift = false, keySpace = false, keyQ = false, keyE = false;
    bool keyFire = false, keyMissile = false;
    bool spacePressed = false, qPressed = false, ePressed = false;
};

class Game {
public:
    bool init(int winW, int winH, bool isSoftwareGL, QualityPreset initialQuality);
    void destroy();

    void handleEvent(const SDL_Event& ev);
    void update(float dt);
    void render();

    void startNewRun(GameMode mode);
    void setQuality(QualityPreset q);
    QualityPreset getQuality() const { return quality; }
    const char* qualityName() const;
    void setWindowSize(int w, int h);
    void forceBossDemo(int sectorIdx); // Used for automated visual verification/demos
    void setTitleDemoCamera(int sectorIdx);
    bool shouldQuit() const { return quitRequested; }
    void setFpsDisplay(float fps) { currentFps = fps; }

private:
    bool initShaders();
    bool initFramebuffers(int w, int h);
    void destroyFramebuffers();

    void updateTitleAutopilot(float dt);
    void updatePlayer(float dt);
    void updateCamera(float dt);
    void updateEnemies(float dt);
    void updateWeaponsAndProjectiles(float dt);
    void updateEffects(float dt);
    void updateWaves(float dt);

    void spawnWave(int waveNum);
    void spawnEnemy(EnemyType type, const Vec3& pos);
    void spawnBoss();
    void damageEnemy(Enemy& e, float dmg, const Vec3& hitPos);
    void destroyEnemy(Enemy& e);
    void damagePlayer(float dmg);

    void spawnExplosion(const Vec3& pos, float scale, const Vec3& col, bool big);
    void spawnParticle(const Vec3& pos, const Vec3& vel, const Vec3& col, float size, float life, float drag = 1.6f);
    void spawnShockwave(const Vec3& pos, const Quat& orient, const Vec3& col, float maxR, float dur);
    void addCallout(const std::string& msg, const Vec4& col, float dur = 2.0f);
    void prepareUpgradeDraft();
    void applyUpgrade(int slotIndex);

    // Rendering helpers
    void renderScene3D();
    void renderSky();
    void renderOcean();
    void renderMesh(const Mesh& m, const Mat4& model, const Vec3& tint = {1,1,1}, const Vec3& emissiveAdd = {0,0,0});
    void renderFxBatch();
    void renderPostProcess();
    void renderHudOverlay();
    void renderTitleUI();
    void renderFlightHUD();
    void renderUpgradeUI();
    void renderPauseUI();
    void renderGameOverUI();

    bool projectToScreen(const Vec3& worldPos, Vec2& outScreen, float& outDepth) const;
    void loadHighScore();
    void saveHighScore();

    // Window & Quality
    int winW = 1280, winH = 720;
    int fboW = 0, fboH = 0;
    int bloomW = 0, bloomH = 0;
    bool isSoftwareGL = false;
    QualityPreset quality = QualityPreset::Balanced;
    bool quitRequested = false;
    float currentFps = 60.0f;

    // Shaders
    GLuint progMesh = 0;
    GLuint progOcean = 0;
    GLuint progSky = 0;
    GLuint progFx = 0;
    GLuint progBloomExtract = 0;
    GLuint progBlur = 0;
    GLuint progPost = 0;
    GLuint progPostFast = 0;
    GLuint emptyVao = 0;

    // Cached mesh shader uniform locations (fast per-draw!)
    GLint mLocViewProj = -1, mLocModel = -1, mLocCamPos = -1;
    GLint mLocSunDir = -1, mLocSunColor = -1, mLocSkyZenith = -1, mLocSkyHorizon = -1;
    GLint mLocGroundBounce = -1, mLocFogColor = -1, mLocFogDensity = -1;
    GLint mLocTint = -1, mLocEmissiveAdd = -1, mLocTime = -1;
    GLint mLocPointPos = -1, mLocPointColor = -1, mLocPointRadius = -1;

    // Dynamic FX batch VAO/VBO (particles + ribbons)
    struct FxVert { float x, y, z, r, g, b, a; };
    GLuint fxVao = 0, fxVbo = 0;
    std::vector<FxVert> fxVerts;

    // Framebuffers for HDR scene + bloom
    GLuint sceneFbo = 0, sceneColorTex = 0, sceneDepthRbo = 0;
    GLuint bloomFbo[2] = {0, 0};
    GLuint bloomTex[2] = {0, 0};

    // Meshes
    Mesh meshPlayer;
    Mesh meshThruster;
    Mesh meshDart;
    Mesh meshLancer;
    Mesh meshBossHull;
    Mesh meshBossCore;
    Mesh meshDebris;
    Mesh meshBolt;
    Mesh meshMissile;
    Mesh meshShockRing;

    // Subsystems
    World world;
    HudRenderer hud;
    AudioEngine audio;
    Rng rng{133742};

    // Game State
    GameState state = GameState::Title;
    GameMode mode = GameMode::Assault;
    Player player;
    InputState input;

    // Camera
    Vec3 camPos{0, 74, 195};
    Vec3 camLook{0, 62, 110};
    Vec3 camUp{0, 1, 0};
    float camFovDeg = 65.0f;
    float shakeAmt = 0.0f;
    int camMode = 0; // 0 = Dynamic Chase, 1 = Close Action, 2 = Cockpit Nose
    Mat4 curView, curProj, curViewProj;

    // Dynamic Point Light (explosions/muzzle flash)
    Vec3 dynLightPos{0, 0, 0};
    Vec3 dynLightColor{1, 0.7f, 0.3f};
    float dynLightRadius = 0.0f;

    // Pools
    static constexpr int MAX_ENEMIES = 64;
    static constexpr int MAX_PROJECTILES = 512;
    static constexpr int MAX_MISSILES = 48;
    static constexpr int MAX_PARTICLES = 1200;
    static constexpr int MAX_DEBRIS = 180;
    static constexpr int MAX_SHOCKWAVES = 32;

    Enemy enemies[MAX_ENEMIES];
    Projectile projectiles[MAX_PROJECTILES];
    Missile missiles[MAX_MISSILES];
    Particle particles[MAX_PARTICLES];
    Debris debris[MAX_DEBRIS];
    Shockwave shockwaves[MAX_SHOCKWAVES];

    // Lock-on state
    std::vector<int> lockedIds;
    float lockScanTimer = 0.0f;
    bool lockingActive = false;

    // Progression & Scoring
    int nextEnemyId = 1;
    int wave = 1;
    float waveBannerTimer = 0.0f;
    float waveClearDelay = 0.0f;
    int score = 0;
    int highScore = 0;
    int kills = 0;
    int ringsCleared = 0;
    int combo = 1;
    float comboTimer = 0.0f;
    float totalTime = 0.0f;
    float hitstopTimer = 0.0f;
    Callout callout;

    // Upgrade draft
    UpgradeOption draftOptions[3] = {};
    int draftHover = -1;
    int titleHover = -1;
};
