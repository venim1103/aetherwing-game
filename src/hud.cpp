#include "hud.hpp"
#include "shaders.hpp"
#include <cmath>
#include <cstring>
#include <cctype>

// Compact crisp 5x7 uppercase + digit + symbol bitmap font (zero external font files needed!)
// Each glyph is 7 rows of 5 bits (MSB = left pixel, bit 4..0)
struct Glyph5x7 {
    char ch;
    uint8_t rows[7];
};

static const Glyph5x7 GLYPHS[] = {
    {' ', {0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
    {'!', {0x04,0x04,0x04,0x04,0x04,0x00,0x04}},
    {'%', {0x19,0x19,0x02,0x04,0x08,0x13,0x13}},
    {'+', {0x00,0x04,0x04,0x1F,0x04,0x04,0x00}},
    {'-', {0x00,0x00,0x00,0x1F,0x00,0x00,0x00}},
    {'.', {0x00,0x00,0x00,0x00,0x00,0x06,0x06}},
    {'/', {0x01,0x02,0x02,0x04,0x08,0x08,0x10}},
    {':', {0x00,0x06,0x06,0x00,0x06,0x06,0x00}},
    {'(', {0x02,0x04,0x08,0x08,0x08,0x04,0x02}},
    {')', {0x08,0x04,0x02,0x02,0x02,0x04,0x08}},
    {'[', {0x0E,0x08,0x08,0x08,0x08,0x08,0x0E}},
    {']', {0x0E,0x02,0x02,0x02,0x02,0x02,0x0E}},
    {'|', {0x04,0x04,0x04,0x04,0x04,0x04,0x04}},
    {',', {0x00,0x00,0x00,0x00,0x06,0x04,0x08}},
    {'<', {0x02,0x04,0x08,0x10,0x08,0x04,0x02}},
    {'>', {0x08,0x04,0x02,0x01,0x02,0x04,0x08}},
    {'0', {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}},
    {'1', {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}},
    {'2', {0x0E,0x11,0x01,0x06,0x08,0x10,0x1F}},
    {'3', {0x1F,0x02,0x04,0x06,0x01,0x11,0x0E}},
    {'4', {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}},
    {'5', {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}},
    {'6', {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}},
    {'7', {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}},
    {'8', {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}},
    {'9', {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}},
    {'A', {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}},
    {'B', {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}},
    {'C', {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}},
    {'D', {0x1C,0x12,0x11,0x11,0x11,0x12,0x1C}},
    {'E', {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}},
    {'F', {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}},
    {'G', {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F}},
    {'H', {0x11,0x11,0x11,0x1F,0x11,0x11,0x11}},
    {'I', {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}},
    {'J', {0x07,0x02,0x02,0x02,0x02,0x12,0x0C}},
    {'K', {0x11,0x12,0x14,0x18,0x14,0x12,0x11}},
    {'L', {0x10,0x10,0x10,0x10,0x10,0x10,0x1F}},
    {'M', {0x11,0x1B,0x15,0x15,0x11,0x11,0x11}},
    {'N', {0x11,0x19,0x15,0x13,0x11,0x11,0x11}},
    {'O', {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}},
    {'P', {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}},
    {'Q', {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}},
    {'R', {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}},
    {'S', {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}},
    {'T', {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}},
    {'U', {0x11,0x11,0x11,0x11,0x11,0x11,0x0E}},
    {'V', {0x11,0x11,0x11,0x11,0x11,0x0A,0x04}},
    {'W', {0x11,0x11,0x11,0x15,0x15,0x1B,0x11}},
    {'X', {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}},
    {'Y', {0x11,0x11,0x0A,0x04,0x04,0x04,0x04}},
    {'Z', {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}},
    {'*', {0x00,0x0A,0x04,0x1F,0x04,0x0A,0x00}},
    {'_', {0x00,0x00,0x00,0x00,0x00,0x00,0x1F}},
    {'?', {0x0E,0x11,0x01,0x02,0x04,0x00,0x04}}
};

static int glyphIndexMap[128];

bool HudRenderer::init() {
    prog = glfn::compileProgram(shaders::HUD_VERT, shaders::HUD_FRAG, "HUD");
    if (!prog) return false;
    locScreen = glfn::GetUniformLocation(prog, "uScreenSize");
    locUseTex = glfn::GetUniformLocation(prog, "uUseTex");
    locFont   = glfn::GetUniformLocation(prog, "uFont");

    glfn::GenVertexArrays(1, &vao);
    glfn::GenBuffers(1, &vbo);
    glfn::BindVertexArray(vao);
    glfn::BindBuffer(GL_ARRAY_BUFFER, vbo);
    glfn::BufferData(GL_ARRAY_BUFFER, 65536 * sizeof(HudVertex), nullptr, GL_STREAM_DRAW);
    glfn::EnableVertexAttribArray(0);
    glfn::VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(HudVertex), (void*)offsetof(HudVertex, x));
    glfn::EnableVertexAttribArray(1);
    glfn::VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(HudVertex), (void*)offsetof(HudVertex, u));
    glfn::EnableVertexAttribArray(2);
    glfn::VertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(HudVertex), (void*)offsetof(HudVertex, r));
    glfn::BindVertexArray(0);

    // Pack font glyphs into 128x128 1-channel atlas (16x16 grid of 8x8 cells)
    for (int i = 0; i < 128; ++i) glyphIndexMap[i] = 0;
    std::vector<uint8_t> atlas(128 * 128, 0);
    const int numGlyphs = int(sizeof(GLYPHS) / sizeof(GLYPHS[0]));
    for (int g = 0; g < numGlyphs; ++g) {
        glyphIndexMap[int(GLYPHS[g].ch)] = g;
        int gx = (g % 16) * 8;
        int gy = (g / 16) * 8;
        for (int row = 0; row < 7; ++row) {
            uint8_t bits = GLYPHS[g].rows[row];
            for (int col = 0; col < 5; ++col) {
                if (bits & (1u << (4 - col))) {
                    atlas[(gy + row) * 128 + (gx + col + 1)] = 255;
                }
            }
        }
    }

    glGenTextures(1, &fontTex);
    glBindTexture(GL_TEXTURE_2D, fontTex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 128, 128, 0, GL_RED, GL_UNSIGNED_BYTE, atlas.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    batch.reserve(4096);
    return true;
}

void HudRenderer::destroy() {
    if (fontTex) { glDeleteTextures(1, &fontTex); fontTex = 0; }
    if (vbo) { glfn::DeleteBuffers(1, &vbo); vbo = 0; }
    if (vao) { glfn::DeleteVertexArrays(1, &vao); vao = 0; }
    if (prog) { glfn::DeleteProgram(prog); prog = 0; }
}

void HudRenderer::begin(int screenW, int screenH) {
    scrW = screenW;
    scrH = screenH;
    batch.clear();
    curTextured = false;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glfn::UseProgram(prog);
    glfn::Uniform2f(locScreen, float(scrW), float(scrH));
    glfn::ActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fontTex);
    glfn::Uniform1i(locFont, 0);
}

void HudRenderer::flushBatch(bool textured) {
    if (batch.empty()) return;
    glfn::Uniform1i(locUseTex, textured ? 1 : 0);
    glfn::BindVertexArray(vao);
    glfn::BindBuffer(GL_ARRAY_BUFFER, vbo);
    glfn::BufferSubData(GL_ARRAY_BUFFER, 0, GLsizeiptr(batch.size() * sizeof(HudVertex)), batch.data());
    glDrawArrays(GL_TRIANGLES, 0, GLsizei(batch.size()));
    batch.clear();
}

void HudRenderer::flush() {
    flushBatch(curTextured);
    glfn::BindVertexArray(0);
}

void HudRenderer::pushQuad(float x0, float y0, float x1, float y1,
                           float u0, float v0, float u1, float v1,
                           const Vec4& col, bool textured) {
    if (curTextured != textured || batch.size() + 6 > 65500) {
        flushBatch(curTextured);
        curTextured = textured;
    }
    HudVertex a{x0, y0, u0, v0, col.x, col.y, col.z, col.w};
    HudVertex b{x1, y0, u1, v0, col.x, col.y, col.z, col.w};
    HudVertex c{x1, y1, u1, v1, col.x, col.y, col.z, col.w};
    HudVertex d{x0, y1, u0, v1, col.x, col.y, col.z, col.w};
    batch.push_back(a); batch.push_back(b); batch.push_back(c);
    batch.push_back(a); batch.push_back(c); batch.push_back(d);
}

void HudRenderer::rect(float x, float y, float w, float h, const Vec4& col) {
    pushQuad(x, y, x + w, y + h, 0, 0, 0, 0, col, false);
}

void HudRenderer::rectOutline(float x, float y, float w, float h, float t, const Vec4& col) {
    rect(x, y, w, t, col);
    rect(x, y + h - t, w, t, col);
    rect(x, y + t, t, h - 2 * t, col);
    rect(x + w - t, y + t, t, h - 2 * t, col);
}

void HudRenderer::line(float x0, float y0, float x1, float y1, float thick, const Vec4& col) {
    float dx = x1 - x0, dy = y1 - y0;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.001f) return;
    float nx = (-dy / len) * (thick * 0.5f);
    float ny = ( dx / len) * (thick * 0.5f);

    if (curTextured || batch.size() + 6 > 65500) {
        flushBatch(curTextured);
        curTextured = false;
    }
    HudVertex a{x0 + nx, y0 + ny, 0, 0, col.x, col.y, col.z, col.w};
    HudVertex b{x1 + nx, y1 + ny, 0, 0, col.x, col.y, col.z, col.w};
    HudVertex c{x1 - nx, y1 - ny, 0, 0, col.x, col.y, col.z, col.w};
    HudVertex d{x0 - nx, y0 - ny, 0, 0, col.x, col.y, col.z, col.w};
    batch.push_back(a); batch.push_back(b); batch.push_back(c);
    batch.push_back(a); batch.push_back(c); batch.push_back(d);
}

void HudRenderer::circleOutline(float cx, float cy, float r, int segs, float thick, const Vec4& col) {
    arcOutline(cx, cy, r, 0.0f, TAU, segs, thick, col);
}

void HudRenderer::arcOutline(float cx, float cy, float r, float a0, float a1, int segs, float thick, const Vec4& col) {
    for (int i = 0; i < segs; ++i) {
        float t0 = lerpf(a0, a1, float(i) / segs);
        float t1 = lerpf(a0, a1, float(i + 1) / segs);
        line(cx + std::cos(t0) * r, cy + std::sin(t0) * r,
             cx + std::cos(t1) * r, cy + std::sin(t1) * r, thick, col);
    }
}

void HudRenderer::diamondOutline(float cx, float cy, float r, float thick, const Vec4& col) {
    line(cx, cy - r, cx + r, cy, thick, col);
    line(cx + r, cy, cx, cy + r, thick, col);
    line(cx, cy + r, cx - r, cy, thick, col);
    line(cx - r, cy, cx, cy - r, thick, col);
}

void HudRenderer::panel(float x, float y, float w, float h, const Vec4& bg, const Vec4& accent) {
    rect(x, y, w, h, bg);
    rectOutline(x, y, w, h, 1.5f, {accent.x, accent.y, accent.z, accent.w * 0.45f});
    // Sleek glowing corner brackets
    float c = std::min(14.0f, std::min(w, h) * 0.25f);
    float t = 2.5f;
    rect(x, y, c, t, accent); rect(x, y, t, c, accent);
    rect(x + w - c, y, c, t, accent); rect(x + w - t, y, t, c, accent);
    rect(x, y + h - t, c, t, accent); rect(x, y + h - c, t, c, accent);
    rect(x + w - c, y + h - t, c, t, accent); rect(x + w - t, y + h - c, t, c, accent);
}

void HudRenderer::bar(float x, float y, float w, float h, float fill01, const Vec4& fillCol, const Vec4& bgCol) {
    fill01 = clampf(fill01, 0.0f, 1.0f);
    rect(x, y, w, h, bgCol);
    if (fill01 > 0.001f) {
        rect(x + 2, y + 2, (w - 4) * fill01, h - 4, fillCol);
    }
    rectOutline(x, y, w, h, 1.5f, {fillCol.x, fillCol.y, fillCol.z, 0.7f});
}

float HudRenderer::measureText(float scale, const char* str) const {
    if (!str) return 0.0f;
    size_t len = std::strlen(str);
    return float(len) * 6.5f * scale;
}

void HudRenderer::text(float x, float y, float scale, const Vec4& col, const char* str) {
    if (!str) return;
    float cx = x;
    const float cw = 8.0f * scale;
    const float ch = 8.0f * scale;
    const float adv = 6.5f * scale;
    const float cellUV = 8.0f / 128.0f;

    // Subtle dark shadow for razor-sharp readability against sunlit sky
    Vec4 shadow{0.02f, 0.04f, 0.08f, col.w * 0.75f};
    for (int pass = 0; pass < 2; ++pass) {
        cx = x + (pass == 0 ? scale * 0.85f : 0.0f);
        float cy = y + (pass == 0 ? scale * 0.85f : 0.0f);
        const Vec4& c = (pass == 0) ? shadow : col;
        for (const char* p = str; *p; ++p) {
            unsigned char chCode = static_cast<unsigned char>(std::toupper(static_cast<unsigned char>(*p)));
            int g = (chCode < 128) ? glyphIndexMap[chCode] : 0;
            if (g != 0) {
                float u0 = float(g % 16) * cellUV;
                float v0 = float(g / 16) * cellUV;
                pushQuad(cx, cy, cx + cw, cy + ch, u0, v0, u0 + cellUV, v0 + cellUV, c, true);
            }
            cx += adv;
        }
    }
}

void HudRenderer::textCentered(float cx, float y, float scale, const Vec4& col, const char* str) {
    float w = measureText(scale, str);
    text(cx - w * 0.5f, y, scale, col, str);
}

void HudRenderer::textRight(float rx, float y, float scale, const Vec4& col, const char* str) {
    float w = measureText(scale, str);
    text(rx - w, y, scale, col, str);
}
