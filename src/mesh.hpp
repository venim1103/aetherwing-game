#pragma once
#include "math3d.hpp"
#include "gl_loader.hpp"
#include <vector>
#include <cstdint>

struct Vertex {
    Vec3 pos;
    Vec3 norm;
    Vec3 color;
    float emissive = 0.0f;
    float gloss = 0.2f;
};

struct Mesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    uint32_t indexCount = 0;
    uint32_t vertexCount = 0;

    void destroy();
    void draw() const;
};

struct MeshBuilder {
    std::vector<Vertex> verts;
    std::vector<uint32_t> indices;

    void clear() { verts.clear(); indices.clear(); }

    void addTri(const Vec3& a, const Vec3& b, const Vec3& c,
                const Vec3& col, float emissive = 0.0f, float gloss = 0.2f);
    void addTriColors(const Vec3& a, const Vec3& b, const Vec3& c,
                      const Vec3& ca, const Vec3& cb, const Vec3& cc,
                      float emissive = 0.0f, float gloss = 0.2f);
    void addQuad(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d,
                 const Vec3& col, float emissive = 0.0f, float gloss = 0.2f);
    // Adds a mirrored left/right pair across X=0 (correcting winding for the mirrored side)
    void addMirroredTri(const Vec3& a, const Vec3& b, const Vec3& c,
                        const Vec3& col, float emissive = 0.0f, float gloss = 0.2f);
    void addMirroredQuad(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d,
                         const Vec3& col, float emissive = 0.0f, float gloss = 0.2f);

    void addBox(const Vec3& center, const Vec3& halfSize, const Vec3& col,
                float emissive = 0.0f, float gloss = 0.2f);
    void addPrism(const Vec3& baseCenter, float radiusBottom, float radiusTop,
                  float height, int sides, const Vec3& col,
                  float emissive = 0.0f, float gloss = 0.2f, bool capBottom = true, bool capTop = true);
    void addCone(const Vec3& baseCenter, float radius, float height, int sides,
                 const Vec3& col, float emissive = 0.0f, float gloss = 0.2f);
    void addIcoSphere(const Vec3& center, float radius, int subdiv, const Vec3& col,
                      float emissive = 0.0f, float gloss = 0.2f, float jitter = 0.0f, uint32_t seed = 1);
    void addTorus(const Vec3& center, float majorR, float minorR, int segMajor, int segMinor,
                  const Vec3& col, float emissive = 0.0f, float gloss = 0.2f, int axis = 2);
    void addExtrudedCrossSection(const std::vector<Vec3>& ringA, const std::vector<Vec3>& ringB,
                                 const Vec3& col, float emissive = 0.0f, float gloss = 0.2f);

    Mesh upload() const;
};

namespace meshes {
    Mesh buildPlayerFighter();
    Mesh buildThrusterCone();
    Mesh buildEnemyDart();
    Mesh buildEnemyLancer();
    Mesh buildBossHull();
    Mesh buildBossCore();
    Mesh buildRingGateFrame();
    Mesh buildRingGateEnergy();
    Mesh buildCloudCluster(uint32_t seed);
    Mesh buildSkyRock(uint32_t seed);
    Mesh buildDebrisChunk();
    Mesh buildProjectileBolt();
    Mesh buildMissile();
    Mesh buildShockRing();
    Mesh buildOceanGrid(float halfExtent, int divisions);
}
