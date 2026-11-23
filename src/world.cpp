#include "world.hpp"
#include <cmath>

const SectorPalette& World::getSectorPalette(int idx) {
    static const SectorPalette palettes[3] = {
        // 0: AZURE ARCHIPELAGO (Vivid Ghibli sunlit cerulean & turquoise paradise)
        {
            "AZURE ARCHIPELAGO",
            normalize({0.58f, 0.38f, -0.72f}),
            {1.22f, 1.00f, 0.78f},
            {0.05f, 0.26f, 0.74f},
            {0.46f, 0.76f, 0.96f},
            {0.10f, 0.34f, 0.56f},
            {0.12f, 0.28f, 0.36f},
            {0.42f, 0.72f, 0.92f},
            0.00038f,
            {0.01f, 0.14f, 0.34f},
            {0.04f, 0.52f, 0.62f},
            0.05f,
            0.0f,
            {0.16f, 0.94f, 1.00f},
            {0.18f, 0.58f, 0.28f},
            {0.44f, 0.42f, 0.40f},
            {0.86f, 0.78f, 0.54f}
        },
        // 1: EMBER CALDERA (Rich fiery sunset & volcanic twilight)
        {
            "EMBER CALDERA",
            normalize({-0.60f, 0.22f, -0.77f}),
            {1.45f, 0.64f, 0.26f},
            {0.12f, 0.06f, 0.28f},
            {0.92f, 0.36f, 0.20f},
            {0.26f, 0.09f, 0.14f},
            {0.28f, 0.12f, 0.12f},
            {0.78f, 0.34f, 0.24f},
            0.00044f,
            {0.08f, 0.03f, 0.12f},
            {0.36f, 0.14f, 0.10f},
            0.42f,
            0.0f,
            {1.00f, 0.52f, 0.10f},
            {0.32f, 0.40f, 0.18f},
            {0.26f, 0.20f, 0.22f},
            {0.66f, 0.46f, 0.30f}
        },
        // 2: ASTRAL CITADEL (Deep velvet midnight & bioluminescent aurora)
        {
            "ASTRAL CITADEL",
            normalize({0.35f, 0.48f, -0.80f}),
            {0.58f, 0.72f, 1.15f},
            {0.01f, 0.02f, 0.09f},
            {0.07f, 0.16f, 0.34f},
            {0.01f, 0.03f, 0.10f},
            {0.04f, 0.10f, 0.20f},
            {0.05f, 0.12f, 0.26f},
            0.00042f,
            {0.005f, 0.025f, 0.09f},
            {0.03f, 0.22f, 0.36f},
            1.0f,
            1.0f,
            {0.22f, 1.00f, 0.88f},
            {0.08f, 0.36f, 0.36f},
            {0.18f, 0.20f, 0.30f},
            {0.32f, 0.38f, 0.48f}
        }
    };
    return palettes[((idx % 3) + 3) % 3];
}

static void addPineTree(MeshBuilder& mb, const Vec3& base, float scale, Rng& rng) {
    Vec3 trunk{0.36f, 0.24f, 0.14f};
    float g = rng.range(0.85f, 1.15f);
    Vec3 leaf1{0.12f * g, 0.44f * g, 0.24f * g};
    Vec3 leaf2{0.16f * g, 0.54f * g, 0.28f * g};
    mb.addPrism(base, 0.55f * scale, 0.38f * scale, 2.8f * scale, 5, trunk, 0.0f, 0.1f, false, false);
    mb.addCone(base + Vec3{0, 2.2f * scale, 0}, 2.9f * scale, 5.2f * scale, 6, leaf1, 0.0f, 0.1f);
    mb.addCone(base + Vec3{0, 5.0f * scale, 0}, 2.2f * scale, 4.4f * scale, 6, leaf2, 0.0f, 0.12f);
}

static void addCrystalSpire(MeshBuilder& mb, const Vec3& base, float radius, float height, const Vec3& col) {
    mb.addPrism(base, radius, radius * 0.55f, height * 0.72f, 5, col, 0.95f, 0.90f, false, false);
    mb.addCone(base + Vec3{0, height * 0.72f, 0}, radius * 0.55f, height * 0.28f, 5, col * 1.15f, 1.45f, 0.95f);
}

static void addStoneArch(MeshBuilder& mb, const Vec3& center, float span, float height, float yaw) {
    const int segs = 10;
    const Vec3 rock{0.52f, 0.49f, 0.44f};
    const Vec3 trim{0.85f, 0.74f, 0.42f};
    float cy = std::cos(yaw), sy = std::sin(yaw);
    auto rotY = [&](const Vec3& p) {
        return center + Vec3{p.x * cy - p.z * sy, p.y, p.x * sy + p.z * cy};
    };
    for (int i = 0; i < segs; ++i) {
        float t0 = PI * float(i) / segs;
        float t1 = PI * float(i + 1) / segs;
        Vec3 c0{std::cos(t0) * span, std::sin(t0) * height, 0};
        Vec3 c1{std::cos(t1) * span, std::sin(t1) * height, 0};
        Vec3 dir = normalize(c1 - c0);
        Vec3 perp{-dir.y, dir.x, 0};
        float th = 6.5f;
        float dp = 7.5f;
        Vec3 a0 = rotY(c0 + perp * th + Vec3{0, 0, -dp});
        Vec3 b0 = rotY(c0 - perp * th + Vec3{0, 0, -dp});
        Vec3 c0p = rotY(c0 - perp * th + Vec3{0, 0,  dp});
        Vec3 d0 = rotY(c0 + perp * th + Vec3{0, 0,  dp});
        Vec3 a1 = rotY(c1 + perp * th + Vec3{0, 0, -dp});
        Vec3 b1 = rotY(c1 - perp * th + Vec3{0, 0, -dp});
        Vec3 c1p = rotY(c1 - perp * th + Vec3{0, 0,  dp});
        Vec3 d1 = rotY(c1 + perp * th + Vec3{0, 0,  dp});
        Vec3 col = (i == 4 || i == 5) ? trim : rock;
        mb.addQuad(a0, a1, d1, d0, col, 0.0f, 0.25f);
        mb.addQuad(b0, c0p, c1p, b1, col * 0.85f, 0.0f, 0.20f);
        mb.addQuad(a0, b0, b1, a1, col * 0.92f, 0.0f, 0.22f);
        mb.addQuad(d0, d1, c1p, c0p, col * 0.92f, 0.0f, 0.22f);
    }
}

void World::init(uint32_t seed) {
    destroy();
    Rng rng(seed);
    currentSector = 0;
    activePalette = getSectorPalette(0);
    targetPalette = activePalette;

    // 1. Place 10 sculpted Sea Islands around the archipelago (keep central spawn channel clear!)
    seaIslands.clear();
    const struct { float x, z, r, h; } presets[10] = {
        { -260, -340, 145, 78 },
        {  320, -290, 130, 68 },
        { -440,  160, 165, 92 },
        {  410,  220, 155, 84 },
        {   40, -680, 180, 112 },
        {  -90,  580, 170, 96 },
        { -780, -480, 190, 108 },
        {  790, -440, 185, 102 },
        { -720,  620, 175, 95 },
        {  760,  590, 190, 110 }
    };
    for (const auto& p : presets) {
        seaIslands.push_back({{p.x, 0.0f, p.z}, p.r, p.h, 0.0f});
    }
    for (auto& isl : seaIslands) {
        isl.peakY = sampleSeaIslandHeight(isl.center.x, isl.center.z);
    }

    // Build combined low-poly faceted Sea Island geometry
    MeshBuilder terr;
    const Vec3 sandCol {0.86f, 0.79f, 0.58f};
    const Vec3 grassCol{0.22f, 0.58f, 0.30f};
    const Vec3 rockCol {0.48f, 0.45f, 0.42f};
    const Vec3 peakCol {0.72f, 0.74f, 0.76f};
    const Vec3 cyanCrys{0.18f, 0.92f, 1.00f};

    for (size_t idx = 0; idx < seaIslands.size(); ++idx) {
        const auto& isl = seaIslands[idx];
        const int rings = 6;
        const int segs = 14;
        std::vector<Vec3> grid((rings + 1) * segs);
        for (int r = 0; r <= rings; ++r) {
            float fr = float(r) / rings;
            for (int s = 0; s < segs; ++s) {
                float ang = TAU * float(s) / segs;
                float warp = 0.80f + 0.36f * fbm2D(std::cos(ang) * 1.4f + idx * 3.1f, std::sin(ang) * 1.4f + fr * 1.2f, 3);
                float rad = isl.radius * fr * warp;
                float px = isl.center.x + std::cos(ang) * rad;
                float pz = isl.center.z + std::sin(ang) * rad;
                float h = sampleSeaIslandHeight(px, pz);
                if (r == rings) h = -10.0f;
                grid[r * segs + s] = {px, h, pz};
            }
        }
        auto colorForY = [&](float y) -> Vec3 {
            if (y < 6.0f) return sandCol;
            if (y < isl.peakHeight * 0.48f) return grassCol;
            if (y < isl.peakHeight * 0.82f) return rockCol;
            return peakCol;
        };
        Vec3 peak{isl.center.x, sampleSeaIslandHeight(isl.center.x, isl.center.z), isl.center.z};
        for (int s = 0; s < segs; ++s) {
            int s1 = (s + 1) % segs;
            Vec3 a = grid[1 * segs + s];
            Vec3 b = grid[1 * segs + s1];
            terr.addTri(peak, b, a, colorForY((peak.y + a.y + b.y) / 3.0f), 0.0f, 0.14f);
            for (int r = 1; r < rings; ++r) {
                Vec3 v00 = grid[r * segs + s];
                Vec3 v01 = grid[r * segs + s1];
                Vec3 v10 = grid[(r + 1) * segs + s];
                Vec3 v11 = grid[(r + 1) * segs + s1];
                float avgY = (v00.y + v01.y + v10.y + v11.y) * 0.25f;
                Vec3 c = colorForY(avgY) * rng.range(0.92f, 1.07f);
                terr.addQuad(v00, v01, v11, v10, c, 0.0f, 0.14f);
            }
        }

        // Decorate island slopes with stylized pine groves & glowing crystal obelisks!
        for (int t = 0; t < 11; ++t) {
            float ang = rng.range(0, TAU);
            float dist = rng.range(isl.radius * 0.18f, isl.radius * 0.62f);
            float tx = isl.center.x + std::cos(ang) * dist;
            float tz = isl.center.z + std::sin(ang) * dist;
            float ty = sampleSeaIslandHeight(tx, tz);
            if (ty > 7.5f && ty < isl.peakHeight * 0.65f) {
                addPineTree(terr, {tx, ty - 0.8f, tz}, rng.range(1.6f, 2.8f), rng);
            }
        }
        // Ancient crystal obelisk near summit
        float py = sampleSeaIslandHeight(isl.center.x, isl.center.z);
        addCrystalSpire(terr, {isl.center.x, py - 2.0f, isl.center.z}, 5.5f, 26.0f, cyanCrys);
    }

    // Add 3 giant sculptural coastal stone arches spanning between waves (stored for exact collision & stunt detection!)
    stoneArches.clear();
    stoneArches.push_back({{   0, -4, -180}, 62.0f, 54.0f,  0.15f, 6.5f, 7.5f, 0.0f});
    stoneArches.push_back({{-360, -4,  -80}, 58.0f, 52.0f,  1.10f, 6.5f, 7.5f, 0.0f});
    stoneArches.push_back({{ 360, -4,  -60}, 60.0f, 56.0f, -0.85f, 6.5f, 7.5f, 0.0f});
    for (const auto& a : stoneArches) {
        addStoneArch(terr, a.center, a.span, a.height, a.yaw);
    }

    terrainMesh = terr.upload();

    // 2. Build 8 Floating Sky-Islands suspended high in the clouds!
    floatingIsles.clear();
    const struct { float x, y, z, r, topH, botD; } flPresets[8] = {
        {   0, 128, -460, 62, 16, 68 },
        {-340, 148, -140, 56, 14, 62 },
        { 360, 136,  -90, 58, 15, 65 },
        {-220, 162,  340, 64, 16, 70 },
        { 260, 152,  380, 60, 15, 64 },
        {   0, 178,  140, 52, 14, 58 },
        {-560, 158, -320, 66, 16, 72 },
        { 580, 165,  240, 62, 15, 66 }
    };
    MeshBuilder fl;
    for (int i = 0; i < 8; ++i) {
        const auto& p = flPresets[i];
        FloatingIsle fi;
        fi.center = {p.x, p.y, p.z};
        fi.radius = p.r;
        fi.topHeight = p.topH;
        fi.bottomDepth = p.botD;
        fi.bobPhase = rng.range(0, TAU);
        fi.bobAmp = rng.range(1.5f, 3.2f);

        const int sides = FloatingIsle::RIM_SIDES;
        std::vector<Vec3> rim(sides);
        for (int s = 0; s < sides; ++s) {
            float a = TAU * float(s) / sides;
            float rr = p.r * rng.range(0.84f, 1.12f);
            fi.rimRadii[s] = rr;
            rim[s] = {p.x + std::cos(a) * rr, p.y, p.z + std::sin(a) * rr};
        }
        floatingIsles.push_back(fi);
        Vec3 topCenter{p.x, p.y + p.topH, p.z};
        Vec3 botTip{p.x, p.y - p.botD, p.z};
        for (int s = 0; s < sides; ++s) {
            int s1 = (s + 1) % sides;
            // Emerald grassy plateau top
            fl.addTri(topCenter, rim[s1], rim[s], grassCol * rng.range(0.92f, 1.08f), 0.0f, 0.15f);
            // Jagged rock cliff underside converging to crystal stalactite root
            Vec3 mid0 = lerp(rim[s], botTip, 0.42f) + Vec3{0, rng.range(-3.0f, 3.0f), 0};
            Vec3 mid1 = lerp(rim[s1], botTip, 0.42f) + Vec3{0, rng.range(-3.0f, 3.0f), 0};
            fl.addQuad(rim[s], rim[s1], mid1, mid0, rockCol * rng.range(0.85f, 1.05f), 0.0f, 0.22f);
            fl.addTri(mid0, mid1, botTip, (s % 2 == 0) ? cyanCrys * 0.75f : rockCol * 0.75f,
                      (s % 2 == 0) ? 0.65f : 0.0f, 0.45f);
        }
        // Glowing levitation halo ring beneath the island
        fl.addTorus({p.x, p.y - p.botD * 0.55f, p.z}, p.r * 0.65f, 1.4f, 14, 5, cyanCrys, 1.4f, 0.8f, 1);
        // Crown crystal spire & pine trees on top!
        addCrystalSpire(fl, topCenter - Vec3{0, 2.0f, 0}, 6.0f, 32.0f, cyanCrys);
        for (int t = 0; t < 5; ++t) {
            float a = TAU * float(t) / 5.0f + 0.4f;
            float d = p.r * 0.48f;
            addPineTree(fl, {p.x + std::cos(a) * d, p.y + p.topH * 0.42f, p.z + std::sin(a) * d}, 2.2f, rng);
        }
    }
    floatingMesh = fl.upload();

    // 3. Place 10 Ancient Energy Ring Gates forming thrilling flightlines!
    ringGates.clear();
    const struct { Vec3 p; Vec3 fwd; } gateDefs[10] = {
        {{   0,  58, -110}, { 0.0f,  0.05f, -1.0f}},
        {{ -90,  86, -300}, {-0.4f,  0.12f, -0.9f}},
        {{  95, 118, -420}, { 0.7f,  0.08f, -0.7f}},
        {{ 280,  92, -210}, { 0.4f, -0.10f,  0.9f}},
        {{ 250,  64,  110}, {-0.2f,  0.15f,  0.95f}},
        {{  90, 125,  290}, {-0.8f,  0.05f,  0.6f}},
        {{-180, 105,  260}, {-0.6f, -0.10f, -0.8f}},
        {{-290,  72,   20}, { 0.2f,  0.14f, -0.95f}},
        {{   0, 172,  -40}, { 0.0f,  0.0f,  -1.0f}},
        {{-420, 142, -240}, { 0.7f,  0.06f, -0.7f}}
    };
    for (int i = 0; i < 10; ++i) {
        RingGate rg;
        rg.pos = gateDefs[i].p;
        rg.forward = normalize(gateDefs[i].fwd);
        rg.radius = 23.5f;
        rg.spinAngle = i * 0.7f;
        ringGates.push_back(rg);
    }
    ringFrameMesh = meshes::buildRingGateFrame();
    ringEnergyMesh = meshes::buildRingGateEnergy();

    // 3b. Solid Orbiting Sky-Rocks / Crystal-Veined Aether Asteroids around Floating Isles!
    for (int i = 0; i < 3; ++i) skyRockMeshes[i] = meshes::buildSkyRock(1200 + i * 137);
    skyRocks.clear();
    for (size_t i = 0; i < floatingIsles.size(); ++i) {
        const auto& fi = floatingIsles[i];
        int count = (i < 6) ? 2 : 3;
        for (int k = 0; k < count; ++k) {
            SkyRock sr;
            sr.active = true;
            sr.orbitCenter = fi.center;
            sr.orbitRadius = fi.radius + rng.range(38.0f, 76.0f);
            sr.orbitAngle = TAU * float(k) / float(count) + rng.range(0.0f, 1.4f);
            sr.orbitSpeed = (k % 2 == 0 ? 1.0f : -1.0f) * rng.range(0.05f, 0.11f);
            sr.baseY = fi.center.y + rng.range(-18.0f, 32.0f);
            sr.bobPhase = rng.range(0.0f, TAU);
            sr.radius = rng.range(7.5f, 14.5f);
            sr.rotAxis = rng.unitSphere();
            sr.rotAngle = rng.range(0.0f, TAU);
            sr.rotSpeed = rng.range(0.22f, 0.65f);
            sr.meshIdx = int((i + k) % 3);
            sr.maxHp = sr.radius * 5.0f;
            sr.hp = sr.maxHp;
            sr.pos = {
                sr.orbitCenter.x + std::cos(sr.orbitAngle) * sr.orbitRadius,
                sr.baseY,
                sr.orbitCenter.z + std::sin(sr.orbitAngle) * sr.orbitRadius
            };
            // Ensure rock never overlaps a Ring Gate flight opening
            bool tooClose = false;
            for (const auto& g : ringGates) {
                if ((sr.pos - g.pos).len() < g.radius + sr.radius + 32.0f) { tooClose = true; break; }
            }
            if (tooClose) sr.baseY += 42.0f;
            sr.pos.y = sr.baseY;
            skyRocks.push_back(sr);
        }
    }

    // 4. Stylized Soft 3D Clouds
    for (int i = 0; i < 4; ++i) cloudMeshes[i] = meshes::buildCloudCluster(700 + i * 97);
    clouds.clear();
    for (int i = 0; i < 34; ++i) {
        float ang = rng.range(0, TAU);
        float dist = rng.range(180.0f, 1550.0f);
        clouds.push_back({
            {std::cos(ang) * dist, rng.range(175.0f, 340.0f), std::sin(ang) * dist},
            rng.range(1.6f, 3.6f),
            rng.range(0.0f, TAU),
            i % 4
        });
    }

    // 5. Animated Ocean Grid (48x48 grid = ~4.6k tris, silky smooth on both GPU and llvmpipe!)
    oceanMesh = meshes::buildOceanGrid(2400.0f, 48);
}

void World::destroy() {
    terrainMesh.destroy();
    floatingMesh.destroy();
    for (auto& r : skyRockMeshes) r.destroy();
    ringFrameMesh.destroy();
    ringEnergyMesh.destroy();
    for (auto& c : cloudMeshes) c.destroy();
    oceanMesh.destroy();
}

void World::resetDynamicState() {
    for (auto& g : ringGates) g.cooldown = 0.0f;
    for (auto& a : stoneArches) a.bonusCooldown = 0.0f;
    for (auto& sr : skyRocks) {
        sr.active = true;
        sr.hp = sr.maxHp;
        sr.hitFlash = 0.0f;
    }
}

void World::setSector(int sectorIndex, bool instant) {
    currentSector = ((sectorIndex % 3) + 3) % 3;
    targetPalette = getSectorPalette(currentSector);
    if (instant) activePalette = targetPalette;
}

void World::update(float dt, float totalTime) {
    // Smooth atmospheric crossfade when transitioning sectors
    float k = 1.0f - std::exp(-1.4f * dt);
    activePalette.name          = targetPalette.name;
    activePalette.sunDir        = normalize(lerp(activePalette.sunDir, targetPalette.sunDir, k));
    activePalette.sunColor      = lerp(activePalette.sunColor, targetPalette.sunColor, k);
    activePalette.skyZenith     = lerp(activePalette.skyZenith, targetPalette.skyZenith, k);
    activePalette.skyHorizon    = lerp(activePalette.skyHorizon, targetPalette.skyHorizon, k);
    activePalette.skyBelow      = lerp(activePalette.skyBelow, targetPalette.skyBelow, k);
    activePalette.groundBounce  = lerp(activePalette.groundBounce, targetPalette.groundBounce, k);
    activePalette.fogColor      = lerp(activePalette.fogColor, targetPalette.fogColor, k);
    activePalette.fogDensity    = lerpf(activePalette.fogDensity, targetPalette.fogDensity, k);
    activePalette.waterDeep     = lerp(activePalette.waterDeep, targetPalette.waterDeep, k);
    activePalette.waterShallow  = lerp(activePalette.waterShallow, targetPalette.waterShallow, k);
    activePalette.starIntensity = lerpf(activePalette.starIntensity, targetPalette.starIntensity, k);
    activePalette.auroraIntensity = lerpf(activePalette.auroraIntensity, targetPalette.auroraIntensity, k);

    for (auto& g : ringGates) {
        g.spinAngle += dt * 1.4f;
        if (g.cooldown > 0.0f) g.cooldown = std::max(0.0f, g.cooldown - dt);
    }
    for (auto& a : stoneArches) {
        if (a.bonusCooldown > 0.0f) a.bonusCooldown = std::max(0.0f, a.bonusCooldown - dt);
    }
    for (auto& sr : skyRocks) {
        if (!sr.active) continue;
        sr.orbitAngle += sr.orbitSpeed * dt;
        sr.rotAngle += sr.rotSpeed * dt;
        if (sr.hitFlash > 0.0f) sr.hitFlash = std::max(0.0f, sr.hitFlash - dt * 5.0f);
        sr.pos = {
            sr.orbitCenter.x + std::cos(sr.orbitAngle) * sr.orbitRadius,
            sr.baseY + std::sin(totalTime * 0.9f + sr.bobPhase) * 3.5f,
            sr.orbitCenter.z + std::sin(sr.orbitAngle) * sr.orbitRadius
        };
    }
    for (auto& c : clouds) {
        c.pos.x += dt * 4.5f;
        if (c.pos.x > 1650.0f) c.pos.x = -1650.0f;
    }
}

float World::sampleOceanHeight(float x, float z, float time) {
    float w1 = std::sin(x * 0.042f + time * 1.35f + z * 0.022f) * 0.78f;
    float w2 = std::cos(z * 0.058f - time * 1.05f + x * 0.028f) * 0.48f;
    return w1 + w2;
}

float World::getFloatingBob(float time) {
    return std::sin(time * 0.7f) * 2.2f;
}

float World::sampleSeaIslandHeight(float x, float z) const {
    float maxH = -6.0f;
    for (const auto& isl : seaIslands) {
        float dx = x - isl.center.x;
        float dz = z - isl.center.z;
        float d = std::sqrt(dx * dx + dz * dz);
        if (d < isl.radius * 1.12f) {
            float t = clampf(1.0f - d / isl.radius, 0.0f, 1.0f);
            float ridge = fbm2D(x * 0.015f, z * 0.015f, 3);
            // Terraced volcanic / archipelago profile
            float h = std::pow(t, 1.35f) * isl.peakHeight * (0.72f + 0.55f * ridge);
            // Soft plateau terrace step
            float terrace = std::floor(h / 18.0f) * 18.0f + smoothstepf(0.0f, 18.0f, std::fmod(h, 18.0f)) * 18.0f;
            h = lerpf(h, terrace, 0.28f);
            if (h > maxH) maxH = h;
        }
    }
    return maxH;
}

Vec3 World::sampleSeaIslandNormal(float x, float z) const {
    const float eps = 2.5f;
    float hx = sampleSeaIslandHeight(x + eps, z) - sampleSeaIslandHeight(x - eps, z);
    float hz = sampleSeaIslandHeight(x, z + eps) - sampleSeaIslandHeight(x, z - eps);
    return normalize({-hx, 2.0f * eps, -hz});
}

bool World::checkSolidCollision(const Vec3& pos, float radius, float time, CollisionHit& outHit) const {
    outHit = CollisionHit{};
    float bestPen = -1.0f;
    auto record = [&](SurfaceKind kind, const Vec3& normal, float pen, int idx = -1) {
        if (pen > bestPen) {
            bestPen = pen;
            outHit.hit = true;
            outHit.kind = kind;
            outHit.normal = normal;
            outHit.penetration = pen;
            outHit.entityIndex = idx;
        }
    };

    // 1. Sea Islands terrain + summit crystal obelisks
    float gh = sampleSeaIslandHeight(pos.x, pos.z);
    if (gh > -0.5f && pos.y < gh + radius) {
        Vec3 n = sampleSeaIslandNormal(pos.x, pos.z);
        record(SurfaceKind::Terrain, n, (gh + radius) - pos.y);
    }
    for (const auto& isl : seaIslands) {
        float dx = pos.x - isl.center.x;
        float dz = pos.z - isl.center.z;
        float dXZSq = dx * dx + dz * dz;
        if (dXZSq > (12.0f + radius) * (12.0f + radius)) continue;
        float baseY = isl.peakY - 2.0f;
        float topY = baseY + 26.0f;
        if (pos.y >= baseY - radius && pos.y <= topY + radius) {
            float u = clampf((pos.y - baseY) / 26.0f, 0.0f, 1.0f);
            float spireR = lerpf(5.6f, 0.4f, u);
            float dXZ = std::sqrt(dXZSq);
            if (dXZ < spireR + radius) {
                Vec3 n = (dXZ > 0.01f) ? normalize({dx / dXZ, 0.25f, dz / dXZ}) : Vec3{0, 1, 0};
                record(SurfaceKind::Terrain, n, (spireR + radius) - dXZ);
            }
        }
    }

    // 2. Floating Sky-Islands (exact 9-sided rim + top plateau + stalactite underside + crown crystal spire + synced bob!)
    const float flBob = getFloatingBob(time);
    for (size_t i = 0; i < floatingIsles.size(); ++i) {
        const auto& fi = floatingIsles[i];
        float cy = fi.center.y + flBob;
        float dx = pos.x - fi.center.x;
        float dz = pos.z - fi.center.z;
        float dy = pos.y - cy;
        float maxR = fi.radius * 1.15f + radius;
        if (dx * dx + dz * dz > maxR * maxR) continue;
        if (dy < -fi.bottomDepth - radius || dy > fi.topHeight + 32.0f + radius) continue;

        float dXZ = std::sqrt(dx * dx + dz * dz);

        // Interpolate exact rim radius at this angle
        float ang = std::atan2(dz, dx);
        if (ang < 0.0f) ang += TAU;
        float fIdx = (ang / TAU) * float(FloatingIsle::RIM_SIDES);
        int s0 = int(std::floor(fIdx)) % FloatingIsle::RIM_SIDES;
        int s1 = (s0 + 1) % FloatingIsle::RIM_SIDES;
        float frac = fIdx - std::floor(fIdx);
        float rimR = lerpf(fi.rimRadii[s0], fi.rimRadii[s1], frac);

        // Check crown crystal spire on top (height 32m, base radius 6.0m)
        float spireBase = fi.topHeight - 2.0f;
        float spireTop = spireBase + 32.0f;
        if (dy >= spireBase - radius && dy <= spireTop + radius) {
            float u = clampf((dy - spireBase) / 32.0f, 0.0f, 1.0f);
            float spireR = lerpf(6.2f, 0.5f, u);
            if (dXZ < spireR + radius) {
                Vec3 n = (dXZ > 0.01f) ? normalize({dx / dXZ, 0.25f, dz / dXZ}) : Vec3{0, 1, 0};
                record(SurfaceKind::FloatingIsle, n, (spireR + radius) - dXZ, int(i));
            }
        }

        // Check double-cone island body (top plateau y in [0, topH], underside stalactite y in [-botD, 0])
        if (dy >= -fi.bottomDepth - radius && dy <= fi.topHeight + radius) {
            if (dy >= 0.0f) {
                // Upper grassy plateau cone: surface y_surf(r) = topHeight * (1 - r / rimR)
                float surfY = fi.topHeight * (1.0f - dXZ / std::max(rimR, 1.0f));
                if (dXZ <= rimR + radius && dy <= surfY + radius) {
                    float penY = (surfY + radius) - dy;
                    float radialAtY = rimR * clampf(1.0f - dy / std::max(fi.topHeight, 1.0f), 0.0f, 1.0f);
                    float penR = (radialAtY + radius) - dXZ;
                    if (penY > 0.0f && penR > 0.0f) {
                        Vec3 n;
                        if (dXZ > 0.01f) {
                            float slope = fi.topHeight / std::max(rimR, 1.0f);
                            n = normalize({(dx / dXZ) * slope, 1.0f, (dz / dXZ) * slope});
                        } else {
                            n = {0, 1, 0};
                        }
                        record(SurfaceKind::FloatingIsle, n, std::min(penY, penR), int(i));
                    }
                }
            } else {
                // Lower jagged stalactite underside cone: surface y_surf(r) = -bottomDepth * (1 - r / rimR)
                float surfY = -fi.bottomDepth * (1.0f - dXZ / std::max(rimR, 1.0f));
                if (dXZ <= rimR + radius && dy >= surfY - radius) {
                    float penY = dy - (surfY - radius);
                    float radialAtY = rimR * clampf(1.0f + dy / std::max(fi.bottomDepth, 1.0f), 0.0f, 1.0f);
                    float penR = (radialAtY + radius) - dXZ;
                    if (penY > 0.0f && penR > 0.0f) {
                        Vec3 n;
                        if (dXZ > 0.01f) {
                            float slope = fi.bottomDepth / std::max(rimR, 1.0f);
                            n = normalize({(dx / dXZ) * slope, -1.0f, (dz / dXZ) * slope});
                        } else {
                            n = {0, -1, 0};
                        }
                        record(SurfaceKind::FloatingIsle, n, std::min(penY, penR), int(i));
                    }
                }
            }
        }
    }

    // 3. Coastal Stone Arches (exact semicircular arch ring collision!)
    for (size_t i = 0; i < stoneArches.size(); ++i) {
        const auto& a = stoneArches[i];
        Vec3 rel = pos - a.center;
        float cy = std::cos(-a.yaw), sy = std::sin(-a.yaw);
        Vec3 loc{rel.x * cy - rel.z * sy, rel.y, rel.x * sy + rel.z * cy};
        if (std::abs(loc.z) > a.depth + radius + 2.0f) continue;
        if (loc.y < -6.0f || loc.y > a.height + a.thickness + radius + 4.0f) continue;
        if (std::abs(loc.x) > a.span + a.thickness + radius + 4.0f) continue;

        // Closest parameter t on arch centerline (span*cos(t), height*sin(t)), t in [0, PI]
        float t = std::atan2(std::max(0.0f, loc.y) / a.height, loc.x / a.span);
        t = clampf(t, 0.0f, PI);
        for (int it = 0; it < 2; ++it) {
            float ct = std::cos(t), st = std::sin(t);
            float ex = a.span * ct - loc.x;
            float ey = a.height * st - loc.y;
            float dx = -a.span * st;
            float dy =  a.height * ct;
            float d2x = -a.span * ct;
            float d2y = -a.height * st;
            float f1 = ex * dx + ey * dy;
            float f2 = dx * dx + dy * dy + ex * d2x + ey * d2y;
            if (std::abs(f2) > 1e-4f) t = clampf(t - f1 / f2, 0.0f, PI);
        }
        Vec3 cp{a.span * std::cos(t), a.height * std::sin(t), clampf(loc.z, -a.depth, a.depth)};
        Vec3 diff = loc - cp;
        float dist = diff.len();
        float hitR = a.thickness + radius;
        if (dist < hitR) {
            Vec3 locN = (dist > 0.001f) ? (diff / dist) : Vec3{0, 1, 0};
            float cyW = std::cos(a.yaw), syW = std::sin(a.yaw);
            Vec3 worldN = normalize({locN.x * cyW - locN.z * syW, locN.y, locN.x * syW + locN.z * cyW});
            record(SurfaceKind::StoneArch, worldN, hitR - dist, int(i));
        }
    }

    // 4. Ring Gate Solid Stone Torus Frames (open center is safe; outer stone frame is solid!)
    for (size_t i = 0; i < ringGates.size(); ++i) {
        const auto& g = ringGates[i];
        Vec3 d = pos - g.pos;
        if (d.lenSq() > (30.0f + radius) * (30.0f + radius)) continue;
        float axial = dot(d, g.forward);
        if (std::abs(axial) > 4.5f + radius) continue;
        Vec3 radVec = d - g.forward * axial;
        float radDist = radVec.len();
        float majorR = 24.0f;
        float tubeR = 2.6f;
        Vec3 ringCenterPt = (radDist > 0.001f) ? (radVec / radDist) * majorR : Vec3{majorR, 0, 0};
        Vec3 fromTube = d - ringCenterPt;
        float tubeDist = fromTube.len();
        if (tubeDist < tubeR + radius) {
            Vec3 n = (tubeDist > 0.001f) ? (fromTube / tubeDist) : normalize(d);
            record(SurfaceKind::RingFrame, n, (tubeR + radius) - tubeDist, int(i));
        }
    }

    // 5. Solid Floating Sky-Rocks / Aether Asteroids
    for (size_t i = 0; i < skyRocks.size(); ++i) {
        const auto& sr = skyRocks[i];
        if (!sr.active) continue;
        Vec3 d = pos - sr.pos;
        float hitR = sr.radius + radius;
        float dSq = d.lenSq();
        if (dSq < hitR * hitR) {
            float dist = std::sqrt(dSq);
            Vec3 n = (dist > 0.001f) ? (d / dist) : Vec3{0, 1, 0};
            record(SurfaceKind::SkyRock, n, hitR - dist, int(i));
        }
    }

    return outHit.hit;
}

bool World::checkSolidSweep(const Vec3& prevPos, const Vec3& pos, float radius, float time, CollisionHit& outHit) const {
    Vec3 delta = pos - prevPos;
    float dist = delta.len();
    int steps = std::max(1, std::min(6, int(std::ceil(dist / std::max(radius * 0.85f, 1.5f)))));
    for (int s = 1; s <= steps; ++s) {
        float u = float(s) / float(steps);
        Vec3 p = lerp(prevPos, pos, u);
        if (checkSolidCollision(p, radius, time, outHit)) {
            return true;
        }
    }
    return false;
}

bool World::checkLookaheadHazard(const Vec3& pos, const Vec3& vel, float lookaheadSec, float radius, float time,
                                 CollisionHit& outHit, float& outTtc) const {
    outHit = CollisionHit{};
    outTtc = lookaheadSec;
    const int samples = 8;
    for (int i = 1; i <= samples; ++i) {
        float t = lookaheadSec * (float(i) / float(samples));
        Vec3 p = pos + vel * t;
        float waveY = sampleOceanHeight(p.x, p.z, time + t);
        if (p.y <= waveY + 2.2f && vel.y < -3.0f) {
            outHit.hit = true;
            outHit.kind = SurfaceKind::Water;
            outHit.normal = {0, 1, 0};
            outHit.penetration = (waveY + 2.2f) - p.y;
            outTtc = t;
            return true;
        }
        if (checkSolidCollision(p, radius + 1.5f, time + t, outHit)) {
            outTtc = t;
            return true;
        }
    }
    return false;
}

bool World::isInsideCloud(const Vec3& pos) const {
    for (const auto& c : clouds) {
        Vec3 d = pos - c.pos;
        float rx = 26.0f * c.scale;
        float ry = 11.5f * c.scale;
        float normSq = (d.x * d.x + d.z * d.z) / (rx * rx) + (d.y * d.y) / (ry * ry);
        if (normSq < 1.0f) return true;
    }
    return false;
}
