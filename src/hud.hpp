#pragma once
#include "math3d.hpp"
#include "gl_loader.hpp"
#include <vector>
#include <string>

struct HudVertex {
    float x, y;
    float u, v;
    float r, g, b, a;
};

class HudRenderer {
public:
    bool init();
    void destroy();

    void begin(int screenW, int screenH);
    void flush();

    // Primitives (screen pixels, origin top-left)
    void rect(float x, float y, float w, float h, const Vec4& col);
    void rectOutline(float x, float y, float w, float h, float thick, const Vec4& col);
    void line(float x0, float y0, float x1, float y1, float thick, const Vec4& col);
    void circleOutline(float cx, float cy, float r, int segs, float thick, const Vec4& col);
    void arcOutline(float cx, float cy, float r, float a0, float a1, int segs, float thick, const Vec4& col);
    void diamondOutline(float cx, float cy, float r, float thick, const Vec4& col);
    void panel(float x, float y, float w, float h, const Vec4& bg, const Vec4& accent);
    void bar(float x, float y, float w, float h, float fill01, const Vec4& fillCol, const Vec4& bgCol);

    // Crisp built-in 8x8 bitmap typography
    void text(float x, float y, float scale, const Vec4& col, const char* str);
    void textCentered(float cx, float y, float scale, const Vec4& col, const char* str);
    void textRight(float rx, float y, float scale, const Vec4& col, const char* str);
    float measureText(float scale, const char* str) const;

    int width() const { return scrW; }
    int height() const { return scrH; }

private:
    void flushBatch(bool textured);
    void pushQuad(float x0, float y0, float x1, float y1,
                  float u0, float v0, float u1, float v1,
                  const Vec4& col, bool textured);

    GLuint prog = 0;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint fontTex = 0;
    GLint locScreen = -1;
    GLint locUseTex = -1;
    GLint locFont = -1;

    int scrW = 1280, scrH = 720;
    bool curTextured = false;
    std::vector<HudVertex> batch;
};
