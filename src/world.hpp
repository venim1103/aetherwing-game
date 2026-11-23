#pragma once
#include "math3d.hpp"
#include "mesh.hpp"
#include <vector>

struct SectorPalette {
    const char* name;
    Vec3 sunDir;
    Vec3 sunColor;
    Vec3 skyZenith;
    Vec3 skyHorizon;
    Vec3 skyBelow;
    Vec3 groundBounce;
    Vec3 fogColor;
    float fogDensity;
    Vec3 waterDeep;
    Vec3 waterShallow;
    float starIntensity;
    float auroraIntensity;
    Vec3 crystalColor;
    Vec3 grassColor;
    Vec3 rockColor;
    Vec3 sandColor;
};

struct SeaIsland {
    Vec3 center;
    float radius;
    float peakHeight;
    float peakY = 0.0f; // Actual sampled summit Y for crystal spire collision
};

struct FloatingIsle {
    static constexpr int RIM_SIDES = 9;
    Vec3 center;
    float radius;
    float topHeight;
    float bottomDepth;
    float bobPhase;
    float bobAmp;
    float rimRadii[RIM_SIDES] = {};
};

struct StoneArch {
    Vec3 center;
    float span;
    float height;
    float yaw;
    float thickness = 6.5f;
    float depth = 7.5f;
    float bonusCooldown = 0.0f;
};

struct SkyRock {
    bool active = true;
    Vec3 pos;
    Vec3 orbitCenter;
    float orbitRadius = 90.0f;
    float orbitAngle = 0.0f;
    float orbitSpeed = 0.08f;
    float baseY = 140.0f;
    float bobPhase = 0.0f;
    float radius = 10.0f;
    Vec3 rotAxis{0, 1, 0};
    float rotAngle = 0.0f;
    float rotSpeed = 0.35f;
    int meshIdx = 0;
    float hp = 55.0f;
    float maxHp = 55.0f;
    float hitFlash = 0.0f;
};

enum class SurfaceKind {
    None,
    Terrain,
    FloatingIsle,
    StoneArch,
    RingFrame,
    SkyRock,
    Water
};

struct CollisionHit {
    bool hit = false;
    SurfaceKind kind = SurfaceKind::None;
    Vec3 normal{0, 1, 0};
    float penetration = 0.0f;
    int entityIndex = -1;
};

struct RingGate {
    Vec3 pos;
    Vec3 forward;
    float radius = 22.0f;
    float cooldown = 0.0f;
    float spinAngle = 0.0f;
    bool highlighted = false;
};

struct CloudInst {
    Vec3 pos;
    float scale;
    float yaw;
    int meshIdx;
};

class World {
public:
    static constexpr float ARENA_RADIUS = 1350.0f;

    std::vector<SeaIsland> seaIslands;
    std::vector<FloatingIsle> floatingIsles;
    std::vector<StoneArch> stoneArches;
    std::vector<SkyRock> skyRocks;
    std::vector<RingGate> ringGates;
    std::vector<CloudInst> clouds;

    Mesh terrainMesh;       // Combined baked static archipelago + pines + monoliths + arches
    Mesh floatingMesh;      // Combined baked floating sky-islands + crystal spires
    Mesh skyRockMeshes[3];  // Solid floating crystal-veined sky-rocks / asteroids
    Mesh ringFrameMesh;
    Mesh ringEnergyMesh;
    Mesh cloudMeshes[4];
    Mesh oceanMesh;

    int currentSector = 0;
    SectorPalette activePalette;
    SectorPalette targetPalette;

    void init(uint32_t seed = 2026);
    void destroy();
    void resetDynamicState();
    void setSector(int sectorIndex, bool instant = false);
    void update(float dt, float totalTime);

    // Exact wave surface height matching OCEAN_VERT shader
    static float sampleOceanHeight(float x, float z, float time);
    // Exact floating island vertical bobbing offset shared by rendering & collision
    static float getFloatingBob(float time);

    // Returns smooth terrain height above sea level at (x, z)
    float sampleSeaIslandHeight(float x, float z) const;
    Vec3 sampleSeaIslandNormal(float x, float z) const;

    // Exact collision check against all solid world structures (terrain, spires, floating isles, arches, ring frames, sky-rocks)
    bool checkSolidCollision(const Vec3& pos, float radius, float time, CollisionHit& outHit) const;
    // Continuous swept collision check along prevPos -> pos
    bool checkSolidSweep(const Vec3& prevPos, const Vec3& pos, float radius, float time, CollisionHit& outHit) const;
    // Predictive lookahead hazard check (includes water surface) for HUD proximity alerts
    bool checkLookaheadHazard(const Vec3& pos, const Vec3& vel, float lookaheadSec, float radius, float time,
                              CollisionHit& outHit, float& outTtc) const;
    // Checks if a position is inside any soft atmospheric cloud volume
    bool isInsideCloud(const Vec3& pos) const;

    static const SectorPalette& getSectorPalette(int idx);
};
