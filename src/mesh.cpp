#include "mesh.hpp"
#include <cmath>

void Mesh::destroy() {
    if (ebo) { glfn::DeleteBuffers(1, &ebo); ebo = 0; }
    if (vbo) { glfn::DeleteBuffers(1, &vbo); vbo = 0; }
    if (vao) { glfn::DeleteVertexArrays(1, &vao); vao = 0; }
    indexCount = 0;
    vertexCount = 0;
}

void Mesh::draw() const {
    if (!vao || indexCount == 0) return;
    glfn::BindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, GLsizei(indexCount), GL_UNSIGNED_INT, nullptr);
}

void MeshBuilder::addTri(const Vec3& a, const Vec3& b, const Vec3& c,
                         const Vec3& col, float emissive, float gloss) {
    Vec3 n = normalize(cross(b - a, c - a));
    uint32_t base = uint32_t(verts.size());
    verts.push_back({a, n, col, emissive, gloss});
    verts.push_back({b, n, col, emissive, gloss});
    verts.push_back({c, n, col, emissive, gloss});
    indices.push_back(base);
    indices.push_back(base + 1);
    indices.push_back(base + 2);
}

void MeshBuilder::addTriColors(const Vec3& a, const Vec3& b, const Vec3& c,
                               const Vec3& ca, const Vec3& cb, const Vec3& cc,
                               float emissive, float gloss) {
    Vec3 n = normalize(cross(b - a, c - a));
    uint32_t base = uint32_t(verts.size());
    verts.push_back({a, n, ca, emissive, gloss});
    verts.push_back({b, n, cb, emissive, gloss});
    verts.push_back({c, n, cc, emissive, gloss});
    indices.push_back(base);
    indices.push_back(base + 1);
    indices.push_back(base + 2);
}

void MeshBuilder::addQuad(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d,
                          const Vec3& col, float emissive, float gloss) {
    addTri(a, b, c, col, emissive, gloss);
    addTri(a, c, d, col, emissive, gloss);
}

void MeshBuilder::addMirroredTri(const Vec3& a, const Vec3& b, const Vec3& c,
                                 const Vec3& col, float emissive, float gloss) {
    addTri(a, b, c, col, emissive, gloss);
    addTri({-a.x, a.y, a.z}, {-c.x, c.y, c.z}, {-b.x, b.y, b.z}, col, emissive, gloss);
}

void MeshBuilder::addMirroredQuad(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d,
                                  const Vec3& col, float emissive, float gloss) {
    addQuad(a, b, c, d, col, emissive, gloss);
    addQuad({-a.x, a.y, a.z}, {-d.x, d.y, d.z}, {-c.x, c.y, c.z}, {-b.x, b.y, b.z}, col, emissive, gloss);
}

void MeshBuilder::addBox(const Vec3& c, const Vec3& h, const Vec3& col, float emissive, float gloss) {
    Vec3 p000{c.x - h.x, c.y - h.y, c.z - h.z};
    Vec3 p100{c.x + h.x, c.y - h.y, c.z - h.z};
    Vec3 p110{c.x + h.x, c.y + h.y, c.z - h.z};
    Vec3 p010{c.x - h.x, c.y + h.y, c.z - h.z};
    Vec3 p001{c.x - h.x, c.y - h.y, c.z + h.z};
    Vec3 p101{c.x + h.x, c.y - h.y, c.z + h.z};
    Vec3 p111{c.x + h.x, c.y + h.y, c.z + h.z};
    Vec3 p011{c.x - h.x, c.y + h.y, c.z + h.z};

    addQuad(p001, p101, p111, p011, col, emissive, gloss); // +Z
    addQuad(p100, p000, p010, p110, col, emissive, gloss); // -Z
    addQuad(p011, p111, p110, p010, col, emissive, gloss); // +Y
    addQuad(p000, p100, p101, p001, col, emissive, gloss); // -Y
    addQuad(p101, p100, p110, p111, col, emissive, gloss); // +X
    addQuad(p000, p001, p011, p010, col, emissive, gloss); // -X
}

void MeshBuilder::addPrism(const Vec3& baseCenter, float rBot, float rTop, float height, int sides,
                           const Vec3& col, float emissive, float gloss, bool capBot, bool capTop) {
    Vec3 topC = baseCenter + Vec3{0, height, 0};
    for (int i = 0; i < sides; ++i) {
        float a0 = TAU * float(i) / sides;
        float a1 = TAU * float(i + 1) / sides;
        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);
        Vec3 b0 = baseCenter + Vec3{c0 * rBot, 0, s0 * rBot};
        Vec3 b1 = baseCenter + Vec3{c1 * rBot, 0, s1 * rBot};
        Vec3 t0 = topC + Vec3{c0 * rTop, 0, s0 * rTop};
        Vec3 t1 = topC + Vec3{c1 * rTop, 0, s1 * rTop};
        addQuad(b0, t0, t1, b1, col, emissive, gloss);
        if (capBot && rBot > 1e-4f) addTri(baseCenter, b0, b1, col, emissive, gloss);
        if (capTop && rTop > 1e-4f) addTri(topC, t1, t0, col, emissive, gloss);
    }
}

void MeshBuilder::addCone(const Vec3& baseCenter, float radius, float height, int sides,
                          const Vec3& col, float emissive, float gloss) {
    Vec3 tip = baseCenter + Vec3{0, height, 0};
    for (int i = 0; i < sides; ++i) {
        float a0 = TAU * float(i) / sides;
        float a1 = TAU * float(i + 1) / sides;
        Vec3 b0 = baseCenter + Vec3{std::cos(a0) * radius, 0, std::sin(a0) * radius};
        Vec3 b1 = baseCenter + Vec3{std::cos(a1) * radius, 0, std::sin(a1) * radius};
        addTri(b0, tip, b1, col, emissive, gloss);
        addTri(baseCenter, b0, b1, col, emissive, gloss);
    }
}

void MeshBuilder::addIcoSphere(const Vec3& center, float radius, int subdiv, const Vec3& col,
                               float emissive, float gloss, float jitter, uint32_t seed) {
    const float t = (1.0f + std::sqrt(5.0f)) * 0.5f;
    std::vector<Vec3> pts = {
        normalize({-1, t, 0}), normalize({ 1, t, 0}), normalize({-1,-t, 0}), normalize({ 1,-t, 0}),
        normalize({ 0,-1, t}), normalize({ 0, 1, t}), normalize({ 0,-1,-t}), normalize({ 0, 1,-t}),
        normalize({ t, 0,-1}), normalize({ t, 0, 1}), normalize({-t, 0,-1}), normalize({-t, 0, 1})
    };
    struct Tri { int a, b, c; };
    std::vector<Tri> tris = {
        {0,11,5},{0,5,1},{0,1,7},{0,7,10},{0,10,11},
        {1,5,9},{5,11,4},{11,10,2},{10,7,6},{7,1,8},
        {3,9,4},{3,4,2},{3,2,6},{3,6,8},{3,8,9},
        {4,9,5},{2,4,11},{6,2,10},{8,6,7},{9,8,1}
    };
    for (int s = 0; s < subdiv; ++s) {
        std::vector<Tri> next;
        next.reserve(tris.size() * 4);
        for (const auto& tr : tris) {
            Vec3 ab = normalize(pts[tr.a] + pts[tr.b]);
            Vec3 bc = normalize(pts[tr.b] + pts[tr.c]);
            Vec3 ca = normalize(pts[tr.c] + pts[tr.a]);
            int iab = int(pts.size()); pts.push_back(ab);
            int ibc = int(pts.size()); pts.push_back(bc);
            int ica = int(pts.size()); pts.push_back(ca);
            next.push_back({tr.a, iab, ica});
            next.push_back({tr.b, ibc, iab});
            next.push_back({tr.c, ica, ibc});
            next.push_back({iab, ibc, ica});
        }
        tris.swap(next);
    }
    Rng rng(seed);
    std::vector<float> rads(pts.size(), radius);
    if (jitter > 0.0f) {
        for (size_t i = 0; i < pts.size(); ++i) {
            // deterministic per-direction noise so shared vertices match
            Vec3 p = pts[i];
            float n = std::sin(p.x * 7.3f + float(seed)) * std::cos(p.y * 6.1f - p.z * 5.7f);
            rads[i] = radius * (1.0f + n * jitter);
        }
    }
    for (const auto& tr : tris) {
        Vec3 a = center + pts[tr.a] * rads[tr.a];
        Vec3 b = center + pts[tr.b] * rads[tr.b];
        Vec3 c = center + pts[tr.c] * rads[tr.c];
        // Slight per-facet tonal variation for rich stylized look
        float shade = 0.94f + 0.08f * rng.next01();
        addTri(a, b, c, col * shade, emissive, gloss);
    }
}

void MeshBuilder::addTorus(const Vec3& center, float majorR, float minorR, int segMajor, int segMinor,
                           const Vec3& col, float emissive, float gloss, int axis) {
    auto eval = [&](float u, float v) -> Vec3 {
        float cu = std::cos(u), su = std::sin(u);
        float cv = std::cos(v), sv = std::sin(v);
        float r = majorR + minorR * cv;
        Vec3 p{r * cu, r * su, minorR * sv}; // axis=2 (around Z)
        if (axis == 1) p = {r * cu, minorR * sv, r * su}; // around Y
        if (axis == 0) p = {minorR * sv, r * cu, r * su}; // around X
        return center + p;
    };
    for (int i = 0; i < segMajor; ++i) {
        float u0 = TAU * float(i) / segMajor;
        float u1 = TAU * float(i + 1) / segMajor;
        for (int j = 0; j < segMinor; ++j) {
            float v0 = TAU * float(j) / segMinor;
            float v1 = TAU * float(j + 1) / segMinor;
            addQuad(eval(u0, v0), eval(u1, v0), eval(u1, v1), eval(u0, v1), col, emissive, gloss);
        }
    }
}

void MeshBuilder::addExtrudedCrossSection(const std::vector<Vec3>& ringA, const std::vector<Vec3>& ringB,
                                          const Vec3& col, float emissive, float gloss) {
    size_t n = ringA.size();
    for (size_t i = 0; i < n; ++i) {
        size_t j = (i + 1) % n;
        addQuad(ringA[i], ringA[j], ringB[j], ringB[i], col, emissive, gloss);
    }
}

Mesh MeshBuilder::upload() const {
    Mesh m;
    if (verts.empty() || indices.empty()) return m;
    m.vertexCount = uint32_t(verts.size());
    m.indexCount = uint32_t(indices.size());

    glfn::GenVertexArrays(1, &m.vao);
    glfn::GenBuffers(1, &m.vbo);
    glfn::GenBuffers(1, &m.ebo);

    glfn::BindVertexArray(m.vao);
    glfn::BindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glfn::BufferData(GL_ARRAY_BUFFER, GLsizeiptr(verts.size() * sizeof(Vertex)), verts.data(), GL_STATIC_DRAW);

    glfn::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
    glfn::BufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(indices.size() * sizeof(uint32_t)), indices.data(), GL_STATIC_DRAW);

    constexpr GLsizei stride = sizeof(Vertex);
    glfn::EnableVertexAttribArray(0);
    glfn::VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, pos));
    glfn::EnableVertexAttribArray(1);
    glfn::VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, norm));
    glfn::EnableVertexAttribArray(2);
    glfn::VertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, color));
    glfn::EnableVertexAttribArray(3);
    glfn::VertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, emissive));

    glfn::BindVertexArray(0);
    return m;
}

namespace meshes {

// -----------------------------------------------------------------------------
// VALKYRIE-IX Player Starfighter (faces -Z, wingspan ~8.4m, length ~7.5m)
// -----------------------------------------------------------------------------
Mesh buildPlayerFighter() {
    MeshBuilder mb;
    const Vec3 hullWhite {0.92f, 0.95f, 0.99f};
    const Vec3 hullSlate {0.26f, 0.31f, 0.39f};
    const Vec3 trimCyan  {0.10f, 0.85f, 1.00f};
    const Vec3 trimGold  {1.00f, 0.68f, 0.16f};
    const Vec3 glassCol  {0.08f, 0.18f, 0.32f};
    const Vec3 engGlow   {0.25f, 0.92f, 1.00f};

    // Sculpted fuselage cross-sections along Z (nose at -4.2, tail at +2.6)
    auto ring = [](float z, float w, float topY, float botY, float chineY) {
        return std::vector<Vec3>{
            { 0.0f, topY,   z},
            { w,    chineY, z},
            { w*0.7f, botY, z},
            {-w*0.7f, botY, z},
            {-w,    chineY, z}
        };
    };
    Vec3 noseTip{0.0f, -0.05f, -4.35f};
    auto r1 = ring(-2.6f, 0.45f, 0.25f, -0.22f, -0.02f);
    auto r2 = ring(-0.8f, 0.92f, 0.52f, -0.35f,  0.04f);
    auto r3 = ring( 1.2f, 1.05f, 0.48f, -0.36f,  0.02f);
    auto r4 = ring( 2.4f, 0.72f, 0.34f, -0.26f,  0.00f);

    // Nose cone triangles
    for (size_t i = 0; i < r1.size(); ++i) {
        size_t j = (i + 1) % r1.size();
        mb.addTri(noseTip, r1[j], r1[i], i == 0 || i == 4 ? trimGold : hullWhite, 0.0f, 0.55f);
    }
    mb.addExtrudedCrossSection(r1, r2, hullWhite, 0.0f, 0.50f);
    mb.addExtrudedCrossSection(r2, r3, hullWhite, 0.0f, 0.45f);
    mb.addExtrudedCrossSection(r3, r4, hullSlate, 0.0f, 0.35f);
    // Tail cap
    mb.addTri(r4[0], r4[1], r4[2], hullSlate, 0.0f, 0.2f);
    mb.addTri(r4[0], r4[2], r4[3], hullSlate, 0.0f, 0.2f);
    mb.addTri(r4[0], r4[3], r4[4], hullSlate, 0.0f, 0.2f);

    // Faceted dark-glass cockpit canopy
    Vec3 cFront{0.0f, 0.34f, -2.05f};
    Vec3 cMidTop{0.0f, 0.88f, -0.55f};
    Vec3 cRear{0.0f, 0.52f, 0.75f};
    Vec3 cSideF{0.46f, 0.38f, -1.15f};
    Vec3 cSideR{0.48f, 0.44f, 0.15f};
    mb.addMirroredTri(cFront, cSideF, cMidTop, glassCol, 0.08f, 0.95f);
    mb.addMirroredQuad(cSideF, cSideR, cRear, cMidTop, glassCol, 0.08f, 0.95f);

    // Main swept delta wings (mirrored L/R) with thickness & angled tips
    Vec3 wRootF{0.82f, 0.08f, -1.15f};
    Vec3 wRootR{0.92f, 0.05f,  1.55f};
    Vec3 wMidF {2.55f, 0.02f,  0.25f};
    Vec3 wMidR {2.50f,-0.02f,  1.65f};
    Vec3 wTipF {4.30f,-0.16f,  1.30f};
    Vec3 wTipR {4.15f,-0.18f,  2.15f};
    Vec3 thk   {0.0f, -0.15f,  0.0f};
    Vec3 thkTip{0.0f, -0.06f,  0.0f};

    // Inner wing (white top, slate bottom)
    mb.addMirroredQuad(wRootF, wRootR, wMidR, wMidF, hullWhite, 0.0f, 0.45f);
    mb.addMirroredQuad(wRootF + thk, wMidF + thk, wMidR + thk, wRootR + thk, hullSlate, 0.0f, 0.30f);
    mb.addMirroredQuad(wRootF, wMidF, wMidF + thk, wRootF + thk, trimCyan, 0.45f, 0.6f); // glowing leading edge strip!
    mb.addMirroredQuad(wMidR, wRootR, wRootR + thk, wMidR + thk, hullSlate, 0.0f, 0.3f);

    // Outer wing panel (accented with gold/cyan tip)
    mb.addMirroredQuad(wMidF, wMidR, wTipR, wTipF, hullSlate, 0.0f, 0.50f);
    mb.addMirroredQuad(wMidF + thk, wTipF + thkTip, wTipR + thkTip, wMidR + thk, hullSlate, 0.0f, 0.30f);
    mb.addMirroredQuad(wMidF, wTipF, wTipF + thkTip, wMidF + thk, trimGold, 0.35f, 0.6f);
    mb.addMirroredQuad(wTipF, wTipR, wTipR + thkTip, wTipF + thkTip, trimCyan, 1.4f, 0.7f); // wingtip nav glow

    // Upward canted winglets at wingtips
    Vec3 wlTopF{4.52f, 0.52f, 1.75f};
    Vec3 wlTopR{4.42f, 0.45f, 2.35f};
    mb.addMirroredQuad(wTipF, wTipR, wlTopR, wlTopF, trimGold, 0.12f, 0.55f);
    mb.addMirroredQuad(wTipR, wTipF, wlTopF, wlTopR, trimGold, 0.12f, 0.55f);

    // Twin angled vertical tail stabilizers
    Vec3 tBaseF{0.62f, 0.34f, 0.75f};
    Vec3 tBaseR{0.68f, 0.28f, 2.25f};
    Vec3 tTopF {1.25f, 1.48f, 1.75f};
    Vec3 tTopR {1.30f, 1.38f, 2.55f};
    mb.addMirroredQuad(tBaseF, tBaseR, tTopR, tTopF, hullWhite, 0.0f, 0.5f);
    mb.addMirroredQuad(tBaseR, tBaseF, tTopF, tTopR, hullSlate, 0.0f, 0.5f);
    // Cyan accent stripe on tail tip
    mb.addMirroredQuad(lerp(tBaseF, tTopF, 0.78f), lerp(tBaseR, tTopR, 0.78f), tTopR, tTopF, trimCyan, 0.6f, 0.6f);

    // Forward canards (small sleek stabilizer fins)
    Vec3 cnF{0.58f, 0.02f, -2.25f};
    Vec3 cnR{0.78f, 0.02f, -1.45f};
    Vec3 cnT{1.55f,-0.04f, -1.30f};
    mb.addMirroredTri(cnF, cnR, cnT, trimGold, 0.0f, 0.5f);
    mb.addMirroredTri(cnF, cnT, cnR, hullSlate, 0.0f, 0.4f);

    // Twin under-wing plasma cannon pods
    for (float sx : {-1.65f, 1.65f}) {
        mb.addBox({sx, -0.26f, 0.0f}, {0.14f, 0.13f, 0.95f}, hullSlate, 0.0f, 0.4f);
        mb.addBox({sx, -0.26f, -1.05f}, {0.07f, 0.07f, 0.25f}, trimCyan, 1.2f, 0.8f);
    }

    // Twin engine nacelles + glowing nozzles at Z = +2.45
    for (float sx : {-0.46f, 0.46f}) {
        mb.addBox({sx, -0.02f, 1.75f}, {0.28f, 0.26f, 0.85f}, hullSlate, 0.0f, 0.5f);
        mb.addBox({sx, -0.02f, 2.62f}, {0.20f, 0.18f, 0.06f}, engGlow, 2.2f, 0.9f);
    }

    return mb.upload();
}

// Glowing plasma afterburner plume cone (points along +Z from origin)
Mesh buildThrusterCone() {
    MeshBuilder mb;
    const int sides = 8;
    const float r0 = 0.21f;
    const float len = 2.6f;
    Vec3 tip{0, 0, len};
    for (int i = 0; i < sides; ++i) {
        float a0 = TAU * float(i) / sides;
        float a1 = TAU * float(i + 1) / sides;
        Vec3 b0{std::cos(a0) * r0, std::sin(a0) * r0, 0.0f};
        Vec3 b1{std::cos(a1) * r0, std::sin(a1) * r0, 0.0f};
        mb.addTriColors(b0, b1, tip,
                        {0.55f, 0.95f, 1.0f}, {0.55f, 0.95f, 1.0f}, {0.15f, 0.45f, 1.0f},
                        2.5f, 0.0f);
    }
    return mb.upload();
}

// -----------------------------------------------------------------------------
// CRIMSON DART Enemy Interceptor (fast agile delta drone)
// -----------------------------------------------------------------------------
Mesh buildEnemyDart() {
    MeshBuilder mb;
    const Vec3 darkHull{0.17f, 0.16f, 0.20f};
    const Vec3 crimson {0.95f, 0.18f, 0.22f};
    const Vec3 glowRed {1.00f, 0.30f, 0.12f};

    Vec3 nose{0, 0, -4.0f};
    Vec3 topM{0, 0.65f, 0.2f};
    Vec3 botM{0,-0.45f, 0.2f};
    Vec3 sideM{0.85f, 0.05f, 0.4f};
    Vec3 tail{0, 0.10f, 2.2f};

    mb.addMirroredTri(nose, sideM, topM, darkHull, 0.0f, 0.55f);
    mb.addMirroredTri(nose, botM, sideM, darkHull, 0.0f, 0.35f);
    mb.addMirroredTri(topM, sideM, tail, crimson, 0.05f, 0.55f);
    mb.addMirroredTri(botM, tail, sideM, darkHull, 0.0f, 0.35f);

    // Forward-swept jagged wings
    Vec3 w1{0.75f, 0.0f, 0.6f};
    Vec3 w2{0.65f, 0.0f, 1.8f};
    Vec3 w3{3.95f,-0.25f,-0.5f};
    Vec3 w4{3.25f,-0.15f, 1.2f};
    mb.addMirroredQuad(w1, w2, w4, w3, crimson, 0.08f, 0.50f);
    mb.addMirroredQuad(w1, w3, w4, w2, darkHull, 0.0f, 0.35f);

    // Glowing sensor eye & engine core
    mb.addBox({0, 0.22f, -1.6f}, {0.22f, 0.12f, 0.45f}, glowRed, 1.9f, 0.8f);
    mb.addBox({0, 0.06f, 2.25f}, {0.36f, 0.24f, 0.15f}, glowRed, 2.4f, 0.8f);
    return mb.upload();
}

// -----------------------------------------------------------------------------
// OBSIDIAN LANCER Heavy Gunship (twin-boom striker)
// -----------------------------------------------------------------------------
Mesh buildEnemyLancer() {
    MeshBuilder mb;
    const Vec3 armor  {0.20f, 0.22f, 0.28f};
    const Vec3 accent {1.00f, 0.58f, 0.10f};
    const Vec3 core   {1.00f, 0.38f, 0.08f};

    // Central armored pod
    mb.addBox({0, 0, 0}, {1.2f, 0.72f, 2.8f}, armor, 0.0f, 0.45f);
    mb.addBox({0, 0.45f, -0.6f}, {0.75f, 0.35f, 1.4f}, accent, 0.12f, 0.65f);
    mb.addBox({0, 0.15f, -2.85f}, {0.55f, 0.22f, 0.18f}, core, 2.0f, 0.9f);

    // Twin forward booms + heavy wings
    for (float sx : {-2.8f, 2.8f}) {
        mb.addBox({sx, -0.1f, -0.7f}, {0.48f, 0.42f, 3.6f}, armor, 0.0f, 0.45f);
        mb.addBox({sx,  0.35f, -0.9f}, {0.32f, 0.10f, 2.4f}, accent, 0.2f, 0.6f);
        mb.addBox({sx, -0.1f,  2.95f}, {0.34f, 0.30f, 0.14f}, core, 2.4f, 0.8f);
    }
    // Cross-wing bridge
    mb.addBox({0, -0.05f, 0.6f}, {5.4f, 0.16f, 1.25f}, armor, 0.0f, 0.4f);
    mb.addBox({0,  0.12f, 0.6f}, {4.9f, 0.06f, 0.45f}, accent, 0.3f, 0.6f);
    return mb.upload();
}

// -----------------------------------------------------------------------------
// AEGIS SKY-DREADNOUGHT BOSS (colossal ~64m floating sky-carrier)
// -----------------------------------------------------------------------------
Mesh buildBossHull() {
    MeshBuilder mb;
    const Vec3 steelDark {0.16f, 0.19f, 0.25f};
    const Vec3 steelMid  {0.29f, 0.34f, 0.42f};
    const Vec3 trimGold  {0.96f, 0.66f, 0.18f};
    const Vec3 trimRed   {0.95f, 0.20f, 0.26f};
    const Vec3 glowBlue  {0.20f, 0.88f, 1.00f};

    // Massive terraced main keel & prow
    mb.addBox({0,  0.0f,  0.0f}, {9.5f, 4.2f, 26.0f}, steelMid, 0.0f, 0.45f);
    mb.addBox({0, -4.6f,  2.0f}, {6.0f, 2.4f, 20.0f}, steelDark, 0.0f, 0.35f);
    // Tapered armored prow
    Vec3 pTip{0, 0.0f, -35.0f};
    Vec3 pTL{-9.5f, 4.2f, -26.0f}, pTR{9.5f, 4.2f, -26.0f};
    Vec3 pBL{-9.5f,-4.2f, -26.0f}, pBR{9.5f,-4.2f, -26.0f};
    mb.addTri(pTip, pTR, pTL, steelMid, 0.0f, 0.5f);
    mb.addTri(pTip, pBL, pBR, steelDark, 0.0f, 0.35f);
    mb.addTri(pTip, pTL, pBL, steelDark, 0.0f, 0.4f);
    mb.addTri(pTip, pBR, pTR, steelDark, 0.0f, 0.4f);

    // Towering command citadel bridge
    mb.addBox({0,  7.2f,  6.5f}, {5.2f, 3.6f, 9.5f}, steelDark, 0.0f, 0.45f);
    mb.addBox({0, 11.4f,  8.0f}, {3.2f, 1.8f, 5.5f}, steelMid, 0.0f, 0.50f);
    mb.addBox({0,  9.2f, -3.1f}, {4.2f, 0.85f, 0.4f}, trimRed, 1.8f, 0.9f); // glowing bridge visor!

    // Twin massive side hangar/wing sponsons (where 4 reactor cores sit!)
    for (float sx : {-17.0f, 17.0f}) {
        mb.addBox({sx, 0.5f, 2.0f}, {7.8f, 2.6f, 16.5f}, steelDark, 0.0f, 0.40f);
        mb.addBox({sx, 3.2f, 2.0f}, {6.4f, 0.5f, 14.5f}, trimGold, 0.08f, 0.55f);
        // Connecting struts
        mb.addBox({sx * 0.55f, 0.5f, 2.0f}, {4.5f, 1.8f, 11.0f}, steelMid, 0.0f, 0.35f);
        // Rear engine banks
        mb.addBox({sx, 0.5f, 18.8f}, {4.5f, 1.7f, 0.6f}, glowBlue, 2.2f, 0.9f);
    }
    // Central engine bank
    mb.addBox({0, 0.0f, 26.4f}, {6.8f, 2.6f, 0.7f}, glowBlue, 2.4f, 0.9f);

    // Dorsal armor stripes
    mb.addBox({0, 4.35f, -10.0f}, {2.4f, 0.25f, 13.0f}, trimRed, 0.15f, 0.5f);

    return mb.upload();
}

// Glowing Boss Reactor Core (destructible weak point!)
Mesh buildBossCore() {
    MeshBuilder mb;
    mb.addIcoSphere({0, 0, 0}, 3.6f, 1, {1.0f, 0.22f, 0.55f}, 1.9f, 0.9f);
    mb.addTorus({0, 0, 0}, 4.5f, 0.45f, 12, 6, {0.25f, 0.92f, 1.0f}, 1.2f, 0.7f, 1);
    return mb.upload();
}

// -----------------------------------------------------------------------------
// Ancient Octagonal Energy Ring Gate (outer stone+gold frame + inner rotating halo)
// -----------------------------------------------------------------------------
Mesh buildRingGateFrame() {
    MeshBuilder mb;
    const Vec3 stone{0.68f, 0.66f, 0.60f};
    const Vec3 gold {0.98f, 0.78f, 0.26f};
    mb.addTorus({0, 0, 0}, 24.0f, 2.4f, 12, 6, stone, 0.0f, 0.25f, 2);
    // 4 sculpted keystone pylons around the ring
    for (int i = 0; i < 4; ++i) {
        float a = i * (PI * 0.5f);
        Vec3 pos{std::cos(a) * 25.8f, std::sin(a) * 25.8f, 0.0f};
        mb.addBox(pos, {2.8f, 2.8f, 3.4f}, gold, 0.28f, 0.75f);
    }
    return mb.upload();
}

Mesh buildRingGateEnergy() {
    MeshBuilder mb;
    mb.addTorus({0, 0, 0}, 21.2f, 0.65f, 20, 6, {0.25f, 0.95f, 1.0f}, 2.2f, 0.9f, 2);
    return mb.upload();
}

// -----------------------------------------------------------------------------
// Stylized Soft Cumulus Cloud (smooth normals + billowy flat-bottom domes)
// -----------------------------------------------------------------------------
Mesh buildCloudCluster(uint32_t seed) {
    MeshBuilder mb;
    Rng rng(seed);

    const float t = (1.0f + std::sqrt(5.0f)) * 0.5f;
    std::vector<Vec3> basePts = {
        normalize({-1, t, 0}), normalize({ 1, t, 0}), normalize({-1,-t, 0}), normalize({ 1,-t, 0}),
        normalize({ 0,-1, t}), normalize({ 0, 1, t}), normalize({ 0,-1,-t}), normalize({ 0, 1,-t}),
        normalize({ t, 0,-1}), normalize({ t, 0, 1}), normalize({-t, 0,-1}), normalize({-t, 0, 1})
    };
    struct Tri { int a, b, c; };
    std::vector<Tri> baseTris = {
        {0,11,5},{0,5,1},{0,1,7},{0,7,10},{0,10,11},
        {1,5,9},{5,11,4},{11,10,2},{10,7,6},{7,1,8},
        {3,9,4},{3,4,2},{3,2,6},{3,6,8},{3,8,9},
        {4,9,5},{2,4,11},{6,2,10},{8,6,7},{9,8,1}
    };
    // Subdivide once into smooth 80-tri puff template
    {
        std::vector<Tri> next;
        next.reserve(baseTris.size() * 4);
        for (const auto& tr : baseTris) {
            Vec3 ab = normalize(basePts[tr.a] + basePts[tr.b]);
            Vec3 bc = normalize(basePts[tr.b] + basePts[tr.c]);
            Vec3 ca = normalize(basePts[tr.c] + basePts[tr.a]);
            int iab = int(basePts.size()); basePts.push_back(ab);
            int ibc = int(basePts.size()); basePts.push_back(bc);
            int ica = int(basePts.size()); basePts.push_back(ca);
            next.push_back({tr.a, iab, ica});
            next.push_back({tr.b, ibc, iab});
            next.push_back({tr.c, ica, ibc});
            next.push_back({iab, ibc, ica});
        }
        baseTris.swap(next);
    }

    auto addSmoothPuff = [&](const Vec3& off, float rx, float ryTop, float ryBot, float rz) {
        uint32_t vBase = uint32_t(mb.verts.size());
        for (const Vec3& u : basePts) {
            float ry = (u.y >= 0.0f) ? ryTop : ryBot;
            Vec3 pos{off.x + u.x * rx, off.y + u.y * ry, off.z + u.z * rz};
            Vec3 norm = normalize({u.x / rx, u.y / ry, u.z / rz});
            float hGrad = clampf(u.y * 0.5f + 0.5f, 0.0f, 1.0f);
            Vec3 col = lerp({0.90f, 0.94f, 0.99f}, {1.0f, 1.0f, 1.0f}, hGrad);
            mb.verts.push_back({pos, norm, col, 0.0f, 0.0f});
        }
        for (const auto& tr : baseTris) {
            mb.indices.push_back(vBase + uint32_t(tr.a));
            mb.indices.push_back(vBase + uint32_t(tr.b));
            mb.indices.push_back(vBase + uint32_t(tr.c));
        }
    };

    // Broad central cumulus crown + 4..5 billowy side lobes
    addSmoothPuff({0.0f, 2.0f, 0.0f}, rng.range(16.0f, 21.0f), rng.range(9.5f, 13.0f), 4.2f, rng.range(12.0f, 16.0f));
    int lobes = rng.rangeInt(4, 5);
    for (int i = 0; i < lobes; ++i) {
        float ang = TAU * float(i) / float(lobes) + rng.range(-0.35f, 0.35f);
        float dist = rng.range(11.0f, 20.0f);
        Vec3 off{std::cos(ang) * dist, rng.range(-1.5f, 3.2f), std::sin(ang) * dist * 0.72f};
        float r = rng.range(9.5f, 14.5f);
        addSmoothPuff(off, r * rng.range(0.95f, 1.25f), r * rng.range(0.55f, 0.75f), r * 0.28f, r * rng.range(0.85f, 1.15f));
    }
    return mb.upload();
}

// -----------------------------------------------------------------------------
// Solid Floating Sky-Rock / Crystal-Veined Aether Asteroid (unit radius ~1.0)
// -----------------------------------------------------------------------------
Mesh buildSkyRock(uint32_t seed) {
    MeshBuilder mb;
    Rng rng(seed);
    const Vec3 basalt{0.44f, 0.41f, 0.45f};
    const Vec3 crystal{0.20f, 0.94f, 1.00f};
    mb.addIcoSphere({0, 0, 0}, 0.94f, 1, basalt, 0.0f, 0.24f, 0.24f, seed);

    // Embedded glowing aether crystal spikes so sky-rocks read clearly even in Astral Citadel night!
    int spikes = 4;
    for (int i = 0; i < spikes; ++i) {
        Vec3 dir = normalize(rng.unitSphere());
        Vec3 base = dir * 0.52f;
        Vec3 tip = dir * 1.30f;
        Vec3 ref = (std::abs(dir.y) < 0.85f) ? Vec3{0, 1, 0} : Vec3{1, 0, 0};
        Vec3 u = normalize(cross(dir, ref));
        Vec3 v = cross(dir, u);
        float rad = 0.27f;
        const int sides = 5;
        for (int s = 0; s < sides; ++s) {
            float a0 = TAU * float(s) / sides;
            float a1 = TAU * float(s + 1) / sides;
            Vec3 b0 = base + (u * std::cos(a0) + v * std::sin(a0)) * rad;
            Vec3 b1 = base + (u * std::cos(a1) + v * std::sin(a1)) * rad;
            mb.addTri(b0, b1, tip, crystal, 1.45f, 0.88f);
        }
    }
    return mb.upload();
}

Mesh buildDebrisChunk() {
    MeshBuilder mb;
    mb.addIcoSphere({0, 0, 0}, 1.0f, 0, {0.85f, 0.85f, 0.88f}, 0.0f, 0.4f, 0.32f, 99);
    return mb.upload();
}

Mesh buildProjectileBolt() {
    MeshBuilder mb;
    // Elongated glowing diamond bolt along Z
    Vec3 f{0, 0, -2.4f}, b{0, 0, 2.4f};
    Vec3 r{0.26f, 0, 0}, l{-0.26f, 0, 0}, u{0, 0.26f, 0}, d{0, -0.26f, 0};
    Vec3 col{1.0f, 1.0f, 1.0f};
    mb.addTri(f, u, r, col, 2.6f, 0.5f); mb.addTri(f, r, d, col, 2.6f, 0.5f);
    mb.addTri(f, d, l, col, 2.6f, 0.5f); mb.addTri(f, l, u, col, 2.6f, 0.5f);
    mb.addTri(b, r, u, col, 2.6f, 0.5f); mb.addTri(b, d, r, col, 2.6f, 0.5f);
    mb.addTri(b, l, d, col, 2.6f, 0.5f); mb.addTri(b, u, l, col, 2.6f, 0.5f);
    return mb.upload();
}

Mesh buildMissile() {
    MeshBuilder mb;
    mb.addBox({0, 0, 0}, {0.22f, 0.22f, 1.4f}, {0.95f, 0.95f, 0.98f}, 0.1f, 0.6f);
    mb.addBox({0, 0, -1.55f}, {0.16f, 0.16f, 0.25f}, {1.0f, 0.35f, 0.12f}, 1.4f, 0.7f);
    mb.addBox({0, 0,  1.50f}, {0.26f, 0.26f, 0.18f}, {1.0f, 0.85f, 0.30f}, 2.6f, 0.9f);
    return mb.upload();
}

Mesh buildShockRing() {
    MeshBuilder mb;
    mb.addTorus({0, 0, 0}, 1.0f, 0.08f, 20, 5, {1.0f, 1.0f, 1.0f}, 2.4f, 0.5f, 2);
    return mb.upload();
}

// Ocean mesh: uses only vec3 position (location 0)
Mesh buildOceanGrid(float halfExtent, int div) {
    std::vector<Vec3> pos;
    std::vector<uint32_t> idx;
    pos.reserve((div + 1) * (div + 1));
    idx.reserve(div * div * 6);

    // Non-linear radial spacing: dense waves near player, huge horizon coverage!
    for (int z = 0; z <= div; ++z) {
        float tz = (float(z) / div) * 2.0f - 1.0f;
        float wz = tz * std::abs(tz) * 0.72f + tz * 0.28f;
        for (int x = 0; x <= div; ++x) {
            float tx = (float(x) / div) * 2.0f - 1.0f;
            float wx = tx * std::abs(tx) * 0.72f + tx * 0.28f;
            pos.push_back({wx * halfExtent, 0.0f, wz * halfExtent});
        }
    }
    int row = div + 1;
    for (int z = 0; z < div; ++z) {
        for (int x = 0; x < div; ++x) {
            uint32_t a = uint32_t(z * row + x);
            uint32_t b = a + 1;
            uint32_t c = a + row;
            uint32_t d = c + 1;
            idx.push_back(a); idx.push_back(c); idx.push_back(b);
            idx.push_back(b); idx.push_back(c); idx.push_back(d);
        }
    }

    Mesh m;
    m.vertexCount = uint32_t(pos.size());
    m.indexCount = uint32_t(idx.size());
    glfn::GenVertexArrays(1, &m.vao);
    glfn::GenBuffers(1, &m.vbo);
    glfn::GenBuffers(1, &m.ebo);
    glfn::BindVertexArray(m.vao);
    glfn::BindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glfn::BufferData(GL_ARRAY_BUFFER, GLsizeiptr(pos.size() * sizeof(Vec3)), pos.data(), GL_STATIC_DRAW);
    glfn::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
    glfn::BufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(idx.size() * sizeof(uint32_t)), idx.data(), GL_STATIC_DRAW);
    glfn::EnableVertexAttribArray(0);
    glfn::VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3), (void*)0);
    glfn::BindVertexArray(0);
    return m;
}

} // namespace meshes
