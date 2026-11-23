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
};

struct FloatingIsle {
    Vec3 center;
    float radius;
    float topHeight;
    float bottomDepth;
    float bobPhase;
    float bobAmp;
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
    std::vector<RingGate> ringGates;
    std::vector<CloudInst> clouds;

    Mesh terrainMesh;       // Combined baked static archipelago + pines + monoliths
    Mesh floatingMesh;      // Combined baked floating sky-islands + crystal spires
    Mesh ringFrameMesh;
    Mesh ringEnergyMesh;
    Mesh cloudMeshes[4];
    Mesh oceanMesh;

    int currentSector = 0;
    SectorPalette activePalette;
    SectorPalette targetPalette;

    void init(uint32_t seed = 2026);
    void destroy();
    void setSector(int sectorIndex, bool instant = false);
    void update(float dt);

    // Returns smooth terrain height above sea level at (x, z)
    float sampleSeaIslandHeight(float x, float z) const;
    // Checks collision with solid archipelago terrain or floating islands
    bool checkSolidCollision(const Vec3& pos, float radius, Vec3& outPushDir) const;

    static const SectorPalette& getSectorPalette(int idx);
};
