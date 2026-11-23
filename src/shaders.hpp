#pragma once

namespace shaders {

// -----------------------------------------------------------------------------
// 1. Stylized 3D Lit Mesh Shader (ships, islands, arches, clouds, bosses)
// -----------------------------------------------------------------------------
inline const char* MESH_VERT = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;
layout(location = 2) in vec3 aColor;
layout(location = 3) in vec2 aMat;

uniform mat4 uViewProj;
uniform mat4 uModel;

out vec3 vWorldPos;
out vec3 vNormal;
out vec3 vColor;
out vec2 vMat;

void main() {
    vec4 wp = uModel * vec4(aPos, 1.0);
    vWorldPos = wp.xyz;
    vNormal = mat3(uModel) * aNorm;
    vColor = aColor;
    vMat = aMat;
    gl_Position = uViewProj * wp;
}
)GLSL";

inline const char* MESH_FRAG = R"GLSL(
#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
in vec3 vColor;
in vec2 vMat;

uniform vec3 uCamPos;
uniform vec3 uSunDir;
uniform vec3 uSunColor;
uniform vec3 uSkyZenith;
uniform vec3 uSkyHorizon;
uniform vec3 uGroundBounce;
uniform vec3 uFogColor;
uniform float uFogDensity;
uniform vec3 uTint;
uniform vec3 uEmissiveAdd;
uniform float uTime;
uniform vec3 uPointPos;
uniform vec3 uPointColor;
uniform float uPointRadius;

out vec4 FragColor;

void main() {
    vec3 N = normalize(vNormal);
    vec3 toCam = uCamPos - vWorldPos;
    float camDist = length(toCam);
    vec3 V = toCam / max(camDist, 0.001);
    vec3 L = normalize(uSunDir);

    float ndl = dot(N, L);
    float diff = clamp((ndl + 0.14) / 1.14, 0.0, 1.0);
    float hemi = clamp(N.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 ambient = mix(uGroundBounce, uSkyZenith * 0.62 + uSkyHorizon * 0.38, hemi);

    // Fast specular highlight (avoid expensive pow when gloss is low)
    float gloss = vMat.y;
    float spec = 0.0;
    if (gloss > 0.18 && ndl > -0.05) {
        vec3 H = normalize(L + V);
        float nh = max(dot(N, H), 0.0);
        float nh2 = nh * nh;
        float nh4 = nh2 * nh2;
        float nh8 = nh4 * nh4;
        float nh16 = nh8 * nh8;
        spec = mix(nh8, nh16 * nh16, gloss) * gloss * 0.90;
    }

    // Fast Fresnel rim light (f^3)
    float omv = 1.0 - max(dot(N, V), 0.0);
    float fresnel = omv * omv * omv;
    vec3 rimCol = mix(uSkyHorizon, uSunColor, max(dot(V, -L), 0.0) * 0.6 + 0.2);
    vec3 rim = rimCol * fresnel * (0.24 + gloss * 0.36);

    // Dynamic point light (explosions / muzzle glow)
    vec3 ptLit = vec3(0.0);
    if (uPointRadius > 0.1) {
        vec3 toPt = uPointPos - vWorldPos;
        float distSq = dot(toPt, toPt);
        float rSq = uPointRadius * uPointRadius;
        if (distSq < rSq) {
            float dist = sqrt(distSq);
            float att = 1.0 - dist / uPointRadius;
            att = att * att;
            float pnd = max(dot(N, toPt / max(dist, 0.001)), 0.15);
            ptLit = uPointColor * att * pnd * 1.45;
        }
    }

    vec3 baseCol = vColor * uTint;
    vec3 lit = baseCol * (ambient * 0.56 + uSunColor * diff * 0.96 + ptLit)
             + uSunColor * spec
             + rim * 0.48;

    vec3 col = lit + baseCol * vMat.x + uEmissiveAdd;

    // Distance + height aerial perspective fog
    float heightFactor = exp(-max(vWorldPos.y, 0.0) * 0.0042);
    float fogAmt = clamp(1.0 - exp(-camDist * uFogDensity * (0.58 + 0.58 * heightFactor)), 0.0, 0.92);

    float sv = max(dot(-V, L), 0.0);
    float sv2 = sv * sv;
    float sunAmt = sv2 * sv2 * sv2;
    vec3 fogC = mix(uFogColor, uSunColor * 1.1, sunAmt * 0.45);

    col = mix(col, fogC, fogAmt);
    FragColor = vec4(col, 1.0);
}
)GLSL";

// -----------------------------------------------------------------------------
// 2. Procedural Animated Ocean Shader (distance-filtered waves, zero moire!)
// -----------------------------------------------------------------------------
inline const char* OCEAN_VERT = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;

uniform mat4 uViewProj;
uniform vec2 uCenterXZ;
uniform float uTime;

out vec3 vWorldPos;
out float vWaveH;

void main() {
    vec3 wp = vec3(aPos.x + uCenterXZ.x, 0.0, aPos.z + uCenterXZ.y);
    float w1 = sin(wp.x * 0.042 + uTime * 1.35 + wp.z * 0.022) * 0.78;
    float w2 = cos(wp.z * 0.058 - uTime * 1.05 + wp.x * 0.028) * 0.48;
    wp.y = w1 + w2;
    vWorldPos = wp;
    vWaveH = wp.y;
    gl_Position = uViewProj * vec4(wp, 1.0);
}
)GLSL";

inline const char* OCEAN_FRAG = R"GLSL(
#version 330 core
in vec3 vWorldPos;
in float vWaveH;

uniform vec3 uCamPos;
uniform vec3 uSunDir;
uniform vec3 uSunColor;
uniform vec3 uSkyZenith;
uniform vec3 uSkyHorizon;
uniform vec3 uWaterDeep;
uniform vec3 uWaterShallow;
uniform vec3 uFogColor;
uniform float uFogDensity;
uniform float uTime;

out vec4 FragColor;

void main() {
    vec3 toCam = uCamPos - vWorldPos;
    float dist = length(toCam);
    vec3 V = toCam / max(dist, 0.001);
    vec3 L = normalize(uSunDir);

    // Distance-attenuated wave normal so close water sparkles and far ocean stays glassy-smooth!
    vec2 p = vWorldPos.xz;
    float farFade = exp(-dist * 0.0016);
    float detailFade = exp(-dist * 0.0045);

    float dx = cos(p.x * 0.042 + uTime * 1.35 + p.y * 0.022) * 0.055 * farFade
             + cos(p.x * 0.16 - p.y * 0.13 + uTime * 2.2) * 0.065 * detailFade;
    float dz = -sin(p.y * 0.058 - uTime * 1.05 + p.x * 0.028) * 0.055 * farFade
             - cos(p.x * 0.14 + p.y * 0.17 - uTime * 2.0) * 0.060 * detailFade;

    vec3 N = normalize(vec3(-dx * 1.8, 1.0, -dz * 1.8));

    float omv = 1.0 - max(dot(N, V), 0.0);
    float fresnel = clamp(omv * omv * omv * omv * 0.85 + 0.08, 0.0, 0.90);

    float crest = smoothstep(0.15, 1.25, vWaveH) * farFade;
    vec3 waterCol = mix(uWaterDeep, uWaterShallow, crest * 0.65 + 0.18);

    vec3 R = reflect(-V, N);
    vec3 skyRefl = mix(uSkyHorizon, uSkyZenith, clamp(R.y * 1.6, 0.0, 1.0));
    vec3 col = mix(waterCol, skyRefl, fresnel * 0.78);

    // Crisp sun glitter path across water
    vec3 H = normalize(L + V);
    float nh = max(dot(N, H), 0.0);
    float nh4 = (nh * nh) * (nh * nh);
    float nh16 = (nh4 * nh4) * (nh4 * nh4);
    float nh64 = (nh16 * nh16) * (nh16 * nh16);
    col += uSunColor * (nh16 * 0.35 + nh64 * nh64 * 2.2);

    // Gentle foam highlights near close wave crests
    float foam = smoothstep(0.85, 1.25, vWaveH) * detailFade * 0.35;
    col = mix(col, uSkyHorizon * 0.92 + vec3(0.12), foam);

    // Horizon fog blend
    float fogAmt = clamp(1.0 - exp(-dist * uFogDensity * 1.08), 0.0, 0.99);
    float sv = max(dot(-V, L), 0.0);
    float sv2 = sv * sv;
    vec3 fogC = mix(uFogColor, uSunColor * 1.05, sv2 * sv2 * sv2 * 0.45);
    col = mix(col, fogC, fogAmt);

    FragColor = vec4(col, 1.0);
}
)GLSL";

// -----------------------------------------------------------------------------
// 3. Procedural Sky Dome Shader
// -----------------------------------------------------------------------------
inline const char* SKY_VERT = R"GLSL(
#version 330 core
out vec2 vNDC;
void main() {
    vec2 pos[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    vNDC = pos[gl_VertexID];
    gl_Position = vec4(vNDC, 0.9999, 1.0);
}
)GLSL";

inline const char* SKY_FRAG = R"GLSL(
#version 330 core
in vec2 vNDC;

uniform vec3 uCamForward;
uniform vec3 uCamRight;
uniform vec3 uCamUp;
uniform float uTanHalfFov;
uniform float uAspect;
uniform vec3 uSunDir;
uniform vec3 uSunColor;
uniform vec3 uSkyZenith;
uniform vec3 uSkyHorizon;
uniform vec3 uSkyBelow;
uniform float uStarIntensity;
uniform float uAuroraIntensity;
uniform float uTime;

out vec4 FragColor;

float hash31(vec3 p) {
    p = fract(p * 0.1031);
    p += dot(p, p.yzx + 33.33);
    return fract((p.x + p.y) * p.z);
}

void main() {
    vec3 dir = normalize(uCamForward + uCamRight * (vNDC.x * uAspect * uTanHalfFov) + uCamUp * (vNDC.y * uTanHalfFov));
    float y = dir.y;

    vec3 col;
    if (y >= 0.0) {
        float t = sqrt(clamp(y, 0.0, 1.0));
        col = mix(uSkyHorizon, uSkyZenith, t);
        float haze = exp(-y * 10.0);
        col += uSkyHorizon * haze * 0.12;
    } else {
        float t = sqrt(clamp(-y, 0.0, 1.0));
        col = mix(uSkyHorizon, uSkyBelow, t);
    }

    // Glowing sun disc + soft halo (fast power chains)
    vec3 L = normalize(uSunDir);
    float sDot = max(dot(dir, L), 0.0);
    float s2 = sDot * sDot;
    float s8 = (s2 * s2) * (s2 * s2);
    float s64 = (s8 * s8) * (s8 * s8);
    float halo = s8 * 0.34 + s64 * 0.72;
    float disc = smoothstep(0.9984, 0.9994, sDot) * 2.6;
    col += uSunColor * (halo + disc);

    // Shimmering aurora ribbons in Astral Citadel sector
    if (uAuroraIntensity > 0.01 && y > 0.04) {
        float wave = sin(dir.x * 5.2 + uTime * 0.35 + sin(dir.z * 3.4 - uTime * 0.2)) * 0.5 + 0.5;
        float band = exp(-abs(y - 0.28 - wave * 0.14) * 10.0);
        vec3 aurCol = mix(vec3(0.06, 0.95, 0.80), vec3(0.68, 0.18, 0.98), wave);
        col += aurCol * band * uAuroraIntensity * 0.75;
    }

    // Crisp twinkling pinpoint starfield (round sub-cell stars, zero blocky grids!)
    if (uStarIntensity > 0.01 && y > 0.02) {
        vec3 scaled = dir * 135.0;
        vec3 cell = floor(scaled);
        vec3 local = fract(scaled) - 0.5;
        float h = hash31(cell);
        if (h > 0.968) {
            vec3 off = vec3(
                hash31(cell + vec3(17.1, 3.7, 9.2)),
                hash31(cell + vec3(5.3, 29.4, 1.8)),
                hash31(cell + vec3(11.9, 7.1, 23.6))
            ) - 0.5;
            vec3 delta = local - off * 0.55;
            float d2 = dot(delta, delta);
            float core = smoothstep(0.055, 0.004, d2);
            if (core > 0.001) {
                float twinkle = 0.70 + 0.30 * sin(uTime * 3.4 + h * 120.0);
                float sizeBoost = smoothstep(0.968, 0.998, h);
                float horizonFade = smoothstep(0.02, 0.24, y);
                vec3 starTint = mix(vec3(0.78, 0.90, 1.0), vec3(1.0, 0.94, 0.80), fract(h * 47.3));
                float starAmt = min(core * (0.55 + sizeBoost * 0.85) * twinkle * horizonFade * uStarIntensity * 1.15, 1.05);
                col += starTint * starAmt;
            }
        }
    }

    FragColor = vec4(col, 1.0);
}
)GLSL";

// -----------------------------------------------------------------------------
// 3b. Soft Luminous Atmospheric Cloud Shader (never dark rocky asteroids!)
// -----------------------------------------------------------------------------
inline const char* CLOUD_VERT = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;
layout(location = 2) in vec3 aColor;
layout(location = 3) in vec2 aMat;

uniform mat4 uViewProj;
uniform mat4 uModel;

out vec3 vWorldPos;
out vec3 vNormal;
out vec3 vColor;

void main() {
    vec4 wp = uModel * vec4(aPos, 1.0);
    vWorldPos = wp.xyz;
    vNormal = normalize(mat3(uModel) * aNorm);
    vColor = aColor;
    gl_Position = uViewProj * wp;
}
)GLSL";

inline const char* CLOUD_FRAG = R"GLSL(
#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
in vec3 vColor;

uniform vec3 uCamPos;
uniform vec3 uSunDir;
uniform vec3 uSunColor;
uniform vec3 uSkyZenith;
uniform vec3 uSkyHorizon;
uniform vec3 uFogColor;
uniform float uFogDensity;
uniform float uStarIntensity;

out vec4 FragColor;

void main() {
    vec3 N = normalize(vNormal);
    vec3 toCam = uCamPos - vWorldPos;
    float camDist = length(toCam);
    vec3 V = toCam / max(camDist, 0.001);
    vec3 L = normalize(uSunDir);

    // Soft wrapped volumetric scattering so clouds stay bright, fluffy, and airy
    float ndl = dot(N, L);
    float wrapDiff = clamp((ndl + 0.65) / 1.65, 0.0, 1.0);
    float topGrad = clamp(N.y * 0.45 + 0.55, 0.0, 1.0);

    // Warm/cool atmospheric base: luminous white in daylight, peach/gold at sunset, pale moonlit silver-blue at night
    float nightness = clamp(uStarIntensity, 0.0, 1.0);
    vec3 shadowCol = mix(uSkyHorizon * 0.85 + vec3(0.22, 0.24, 0.28), uSkyHorizon * 1.15 + vec3(0.08, 0.12, 0.20), nightness);
    vec3 litCol = mix(vec3(1.02, 1.01, 0.99) * mix(vec3(1.0), uSunColor, 0.42), uSkyHorizon * 1.45 + uSunColor * 0.55, nightness * 0.72);

    vec3 col = vColor * mix(shadowCol, litCol, wrapDiff * 0.72 + topGrad * 0.28);

    // Forward-scattered sun silver lining around cloud edges
    float backLit = max(dot(-V, L), 0.0);
    float b2 = backLit * backLit;
    float rim = 1.0 - max(dot(N, V), 0.0);
    col += uSunColor * (b2 * b2 * 0.32 + rim * rim * 0.16) * (1.0 - nightness * 0.45);

    // Gentle aerial perspective fog
    float fogAmt = clamp(1.0 - exp(-camDist * uFogDensity * 0.72), 0.0, 0.90);
    col = mix(col, uFogColor, fogAmt);

    // Soft translucent edges & smooth fade when flying right inside a cloud!
    float nearFade = smoothstep(10.0, 42.0, camDist);
    float edgeSoft = smoothstep(0.02, 0.48, max(dot(N, V), 0.0));
    float alpha = clamp(mix(0.45, 0.86, edgeSoft) * nearFade * (1.0 - fogAmt * 0.45), 0.0, 0.88);
    if (alpha < 0.02) discard;

    FragColor = vec4(col, alpha);
}
)GLSL";

// -----------------------------------------------------------------------------
// 4. Additive Unlit Particle / Ribbon Shader (with soft radial glow UV!)
// -----------------------------------------------------------------------------
inline const char* FX_VERT = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aUV;

uniform mat4 uViewProj;
out vec4 vColor;
out vec2 vUV;

void main() {
    vColor = aColor;
    vUV = aUV;
    gl_Position = uViewProj * vec4(aPos, 1.0);
}
)GLSL";

inline const char* FX_FRAG = R"GLSL(
#version 330 core
in vec4 vColor;
in vec2 vUV;
out vec4 FragColor;
void main() {
    float r2 = dot(vUV, vUV);
    if (r2 > 1.0) discard;
    float soft = 1.0 - r2;
    float falloff = soft * soft;
    FragColor = vec4(vColor.rgb * (0.55 + 0.65 * falloff), vColor.a * falloff);
}
)GLSL";

// -----------------------------------------------------------------------------
// 5. Post-Process Composite (Vivid Filmic ACES + Fast Software Path)
// -----------------------------------------------------------------------------
inline const char* POST_VERT = R"GLSL(
#version 330 core
out vec2 vUV;
void main() {
    vec2 pos[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    vUV = pos[gl_VertexID] * 0.5 + 0.5;
    gl_Position = vec4(pos[gl_VertexID], 0.0, 1.0);
}
)GLSL";

inline const char* BLOOM_EXTRACT_FRAG = R"GLSL(
#version 330 core
in vec2 vUV;
uniform sampler2D uScene;
out vec4 FragColor;
void main() {
    vec3 c = texture(uScene, vUV).rgb;
    float lum = dot(c, vec3(0.2126, 0.7152, 0.0722));
    float k = smoothstep(0.85, 1.45, lum);
    FragColor = vec4(c * k, 1.0);
}
)GLSL";

inline const char* BLUR_FRAG = R"GLSL(
#version 330 core
in vec2 vUV;
uniform sampler2D uTex;
uniform vec2 uDir;
out vec4 FragColor;
void main() {
    vec3 sum = texture(uTex, vUV).rgb * 0.227027;
    vec2 off1 = uDir * 1.384615;
    vec2 off2 = uDir * 3.230769;
    sum += texture(uTex, vUV + off1).rgb * 0.316216;
    sum += texture(uTex, vUV - off1).rgb * 0.316216;
    sum += texture(uTex, vUV + off2).rgb * 0.070270;
    sum += texture(uTex, vUV - off2).rgb * 0.070270;
    FragColor = vec4(sum, 1.0);
}
)GLSL";

// Ultra-fast blit/composite for FAST quality (zero pow/atan/sin per pixel!)
inline const char* POST_FAST_FRAG = R"GLSL(
#version 330 core
in vec2 vUV;
uniform sampler2D uScene;
uniform float uDamageFlash;
uniform float uShieldFlash;

out vec4 FragColor;

void main() {
    vec3 col = texture(uScene, vUV).rgb;
    // Fast vivid filmic curve without pow()
    col = (col * (2.25 * col + 0.08)) / (col * (2.05 * col + 0.42) + 0.08);
    if (uDamageFlash > 0.01) col = mix(col, vec3(1.0, 0.14, 0.12), uDamageFlash * 0.32);
    if (uShieldFlash > 0.01) col = mix(col, vec3(0.20, 0.85, 1.0), uShieldFlash * 0.24);
    FragColor = vec4(clamp(col, 0.0, 1.0), 1.0);
}
)GLSL";

// Full-featured vivid filmic composite + bloom + boost streaks (BALANCED / HIGH)
inline const char* POST_FRAG = R"GLSL(
#version 330 core
in vec2 vUV;
uniform sampler2D uScene;
uniform sampler2D uBloom;
uniform float uBloomAmt;
uniform float uBoostAmt;
uniform float uDamageFlash;
uniform float uShieldFlash;
uniform float uTime;

out vec4 FragColor;

vec3 filmic(vec3 x) {
    // Rich high-contrast ACES curve tuned for already-stylized sRGB-authored palette
    return clamp((x * (2.45 * x + 0.04)) / (x * (2.20 * x + 0.46) + 0.09), 0.0, 1.0);
}

void main() {
    vec2 centered = vUV - 0.5;
    float rSq = dot(centered, centered);

    vec3 col;
    if (uBoostAmt > 0.04) {
        vec2 shift = centered * (uBoostAmt * 0.009);
        col.r = texture(uScene, vUV + shift).r;
        col.g = texture(uScene, vUV).g;
        col.b = texture(uScene, vUV - shift).b;
    } else {
        col = texture(uScene, vUV).rgb;
    }

    if (uBloomAmt > 0.001) {
        col += texture(uBloom, vUV).rgb * uBloomAmt;
    }

    if (uBoostAmt > 0.06 && rSq > 0.11) {
        float ang = atan(centered.y, centered.x);
        float streak = sin(ang * 36.0 + uTime * 9.0) * sin(ang * 21.0 - uTime * 13.0);
        float edge = smoothstep(0.11, 0.44, rSq) * smoothstep(0.60, 0.95, streak);
        col += vec3(0.50, 0.88, 1.0) * edge * uBoostAmt * 0.45;
    }

    col = filmic(col * 1.12);

    // Subtle rich saturation boost + crisp vignette
    float luma = dot(col, vec3(0.299, 0.587, 0.114));
    col = clamp(mix(vec3(luma), col, 1.16), 0.0, 1.0);
    col *= clamp(1.06 - rSq * 0.52, 0.74, 1.0);

    if (uDamageFlash > 0.01) {
        float ef = smoothstep(0.06, 0.45, rSq) * uDamageFlash;
        col = mix(col, vec3(1.0, 0.14, 0.12), clamp(ef * 0.62, 0.0, 0.62));
    }
    if (uShieldFlash > 0.01) {
        float ef = smoothstep(0.08, 0.45, rSq) * uShieldFlash;
        col = mix(col, vec3(0.22, 0.86, 1.0), clamp(ef * 0.44, 0.0, 0.44));
    }

    FragColor = vec4(col, 1.0);
}
)GLSL";

// -----------------------------------------------------------------------------
// 6. Crisp 2D HUD & Bitmap Font Shader
// -----------------------------------------------------------------------------
inline const char* HUD_VERT = R"GLSL(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;

uniform vec2 uScreenSize;
out vec2 vUV;
out vec4 vColor;

void main() {
    vec2 ndc = (aPos / uScreenSize) * 2.0 - 1.0;
    ndc.y = -ndc.y;
    gl_Position = vec4(ndc, 0.0, 1.0);
    vUV = aUV;
    vColor = aColor;
}
)GLSL";

inline const char* HUD_FRAG = R"GLSL(
#version 330 core
in vec2 vUV;
in vec4 vColor;
uniform sampler2D uFont;
uniform int uUseTex;
out vec4 FragColor;

void main() {
    if (uUseTex != 0) {
        float a = texture(uFont, vUV).r;
        if (a < 0.1) discard;
        FragColor = vec4(vColor.rgb, vColor.a * a);
    } else {
        FragColor = vColor;
    }
}
)GLSL";

} // namespace shaders
