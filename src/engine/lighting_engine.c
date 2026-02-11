#include "lighting_engine.h"
#include "math_util.h"
 #include "game/display.h"
 #include <float.h>

#define LE_FLOAT_EPSILON 0.001f

struct LELight
{
    f32 posX;
    f32 posY;
    f32 posZ;
    u8 colorR;
    u8 colorG;
    u8 colorB;
    f32 radius;
    f32 intensity;
    bool added;
    bool useSurfaceNormals;
};

Color gLEAmbientColor = { 127, 127, 127 };
static struct LELight sLights[LE_MAX_LIGHTS] = { 0 };
static s16 sActiveLightIds[LE_MAX_LIGHTS] = { 0 };
static s16 sActiveLightCount = 0;
static u32 sLightRevision = 1;

static u32 sLightBoundsRevision = 0;
static bool sHasInfluentialLights = false;
static Vec3f sLightBoundsMin = { 0 };
static Vec3f sLightBoundsMax = { 0 };

static void le_recompute_light_bounds(void) {
    sHasInfluentialLights = false;
    sLightBoundsMin[0] = FLT_MAX;
    sLightBoundsMin[1] = FLT_MAX;
    sLightBoundsMin[2] = FLT_MAX;
    sLightBoundsMax[0] = -FLT_MAX;
    sLightBoundsMax[1] = -FLT_MAX;
    sLightBoundsMax[2] = -FLT_MAX;

    for (s16 i = 0; i < sActiveLightCount; i++) {
        struct LELight* light = &sLights[sActiveLightIds[i]];
        if (!light->added) { continue; }
        if (light->intensity <= 0.0f || light->radius <= 0.0f) { continue; }

        f32 minX = light->posX - light->radius;
        f32 minY = light->posY - light->radius;
        f32 minZ = light->posZ - light->radius;
        f32 maxX = light->posX + light->radius;
        f32 maxY = light->posY + light->radius;
        f32 maxZ = light->posZ + light->radius;

        if (minX < sLightBoundsMin[0]) { sLightBoundsMin[0] = minX; }
        if (minY < sLightBoundsMin[1]) { sLightBoundsMin[1] = minY; }
        if (minZ < sLightBoundsMin[2]) { sLightBoundsMin[2] = minZ; }
        if (maxX > sLightBoundsMax[0]) { sLightBoundsMax[0] = maxX; }
        if (maxY > sLightBoundsMax[1]) { sLightBoundsMax[1] = maxY; }
        if (maxZ > sLightBoundsMax[2]) { sLightBoundsMax[2] = maxZ; }
        sHasInfluentialLights = true;
    }

    sLightBoundsRevision = sLightRevision;
}

static inline bool le_pos_may_be_affected_by_lights(Vec3f pos) {
    if (sLightBoundsRevision != sLightRevision) {
        le_recompute_light_bounds();
    }
    if (!sHasInfluentialLights) { return false; }
    if (pos[0] < sLightBoundsMin[0] || pos[0] > sLightBoundsMax[0]) { return false; }
    if (pos[1] < sLightBoundsMin[1] || pos[1] > sLightBoundsMax[1]) { return false; }
    if (pos[2] < sLightBoundsMin[2] || pos[2] > sLightBoundsMax[2]) { return false; }
    return true;
}

static enum LEMode sMode = LE_MODE_AFFECT_ALL_SHADED_AND_COLORED;
static enum LEToneMapping sToneMapping = LE_TONE_MAPPING_WEIGHTED;
static bool sEnabled = false;

#define LE_CACHE_CELL_SIZE 64
#define LE_CACHE_SIZE 8192

struct LECachedSample {
    u32 frame;
    u32 revision;
    s32 qx;
    s32 qy;
    s32 qz;
    u32 normalPacked;
    u16 scalarQ;
    u16 pad;
    Vec3f color;
    f32 weight;
    bool valid;
};

static struct LECachedSample sLECache[LE_CACHE_SIZE] = { 0 };

static inline s32 le_quantize_pos(f32 v) {
    return (s32) floorf(v / (f32) LE_CACHE_CELL_SIZE);
}

static inline u16 le_quantize_scalar(f32 s) {
    s32 q = (s32) (s * 16.0f);
    if (q < 0) { q = 0; }
    if (q > 65535) { q = 65535; }
    return (u16) q;
}

static inline u32 le_pack_normal(Vec3f n) {
    if (n == NULL) { return 0; }
    f32 nx = clamp(n[0], -1.0f, 1.0f);
    f32 ny = clamp(n[1], -1.0f, 1.0f);
    f32 nz = clamp(n[2], -1.0f, 1.0f);
    u32 ix = (u32) (s32) ((nx * 127.0f) + 128.0f);
    u32 iy = (u32) (s32) ((ny * 127.0f) + 128.0f);
    u32 iz = (u32) (s32) ((nz * 127.0f) + 128.0f);
    if (ix > 255) { ix = 255; }
    if (iy > 255) { iy = 255; }
    if (iz > 255) { iz = 255; }
    return (ix) | (iy << 8) | (iz << 16);
}

static inline u32 le_hash_key(s32 qx, s32 qy, s32 qz, u32 normalPacked, u16 scalarQ) {
    u32 h = (u32) qx * 0x9E3779B1u;
    h ^= (u32) qy * 0x85EBCA77u;
    h ^= (u32) qz * 0xC2B2AE3Du;
    h ^= normalPacked * 0x27D4EB2Du;
    h ^= (u32) scalarQ * 0x165667B1u;
    return h;
}

static bool le_cache_get(Vec3f pos, Vec3f normal, f32 lightIntensityScalar, Vec3f outColor, f32* outWeight) {
    s32 qx = le_quantize_pos(pos[0]);
    s32 qy = le_quantize_pos(pos[1]);
    s32 qz = le_quantize_pos(pos[2]);
    u32 normalPacked = le_pack_normal(normal);
    u16 scalarQ = le_quantize_scalar(lightIntensityScalar);

    u32 idx = le_hash_key(qx, qy, qz, normalPacked, scalarQ) & (LE_CACHE_SIZE - 1);
    struct LECachedSample* e = &sLECache[idx];
    if (!e->valid) { return false; }
    if (e->frame != gGlobalTimer) { return false; }
    if (e->revision != sLightRevision) { return false; }
    if (e->qx != qx || e->qy != qy || e->qz != qz) { return false; }
    if (e->normalPacked != normalPacked) { return false; }
    if (e->scalarQ != scalarQ) { return false; }

    vec3f_copy(outColor, e->color);
    *outWeight = e->weight;
    return true;
}

static void le_cache_put(Vec3f pos, Vec3f normal, f32 lightIntensityScalar, Vec3f inColor, f32 inWeight) {
    s32 qx = le_quantize_pos(pos[0]);
    s32 qy = le_quantize_pos(pos[1]);
    s32 qz = le_quantize_pos(pos[2]);
    u32 normalPacked = le_pack_normal(normal);
    u16 scalarQ = le_quantize_scalar(lightIntensityScalar);

    u32 idx = le_hash_key(qx, qy, qz, normalPacked, scalarQ) & (LE_CACHE_SIZE - 1);
    struct LECachedSample* e = &sLECache[idx];
    e->frame = gGlobalTimer;
    e->revision = sLightRevision;
    e->qx = qx;
    e->qy = qy;
    e->qz = qz;
    e->normalPacked = normalPacked;
    e->scalarQ = scalarQ;
    vec3f_copy(e->color, inColor);
    e->weight = inWeight;
    e->valid = true;
}

static inline void color_set(Color color, u8 r, u8 g, u8 b) {
    color[0] = r;
    color[1] = g;
    color[2] = b;
}

static inline void color_copy(Color dest, Color src) {
    dest[0] = src[0];
    dest[1] = src[1];
    dest[2] = src[2];
}

bool le_is_enabled(void) {
    // this is needed because we don't want to make vanilla darker,
    // and we don't want to set the ambient color to { 255, 255, 255 }
    // because then no one could see the effect of their lights
    return sEnabled;
}

void le_set_mode(enum LEMode mode) {
    sMode = mode;
}

enum LEMode le_get_mode(void) {
    return sMode;
}

void le_set_tone_mapping(enum LEToneMapping toneMapping) {
    sToneMapping = toneMapping;
}

void le_get_ambient_color(VEC_OUT Color out) {
    color_copy(out, gLEAmbientColor);
}

void le_set_ambient_color(u8 r, u8 g, u8 b) {
    if (gLEAmbientColor[0] == r && gLEAmbientColor[1] == g && gLEAmbientColor[2] == b) {
        sEnabled = true;
        return;
    }
    color_set(gLEAmbientColor, r, g, b);
    sEnabled = true;
    sLightRevision++;
}

static inline void le_tone_map_total_weighted(Color out, Color inAmbient, Vec3f inColor, float weight) {
    out[0] = clamp((inAmbient[0] + inColor[0]) / weight, 0, 255);
    out[1] = clamp((inAmbient[1] + inColor[1]) / weight, 0, 255);
    out[2] = clamp((inAmbient[2] + inColor[2]) / weight, 0, 255);
}

static inline void le_tone_map_weighted(Color out, Color inAmbient, Vec3f inColor, float weight) {
    out[0] = clamp(inAmbient[0] + (inColor[0] / weight), 0, 255);
    out[1] = clamp(inAmbient[1] + (inColor[1] / weight), 0, 255);
    out[2] = clamp(inAmbient[2] + (inColor[2] / weight), 0, 255);
}

static inline void le_tone_map_clamp(Color out, Color inAmbient, Vec3f inColor) {
    out[0] = clamp(inAmbient[0] + inColor[0], 0, 255);
    out[1] = clamp(inAmbient[1] + inColor[1], 0, 255);
    out[2] = clamp(inAmbient[2] + inColor[2], 0, 255);
}

static inline void le_tone_map_reinhard(Color out, Color inAmbient, Vec3f inColor) {
    inColor[0] += inAmbient[0];
    inColor[1] += inAmbient[1];
    inColor[2] += inAmbient[2];

    out[0] = clamp((inColor[0] / (inColor[0] + 255.0f)) * 255.0f, 0, 255);
    out[1] = clamp((inColor[1] / (inColor[1] + 255.0f)) * 255.0f, 0, 255);
    out[2] = clamp((inColor[2] / (inColor[2] + 255.0f)) * 255.0f, 0, 255);
}

static inline void le_tone_map(Color out, Color inAmbient, Vec3f inColor, float weight) {
    switch (sToneMapping) {
        case LE_TONE_MAPPING_TOTAL_WEIGHTED: le_tone_map_total_weighted(out, inAmbient, inColor, weight); break;
        case LE_TONE_MAPPING_WEIGHTED:       le_tone_map_weighted(out, inAmbient, inColor, weight);       break;
        case LE_TONE_MAPPING_CLAMP:          le_tone_map_clamp(out, inAmbient, inColor);                  break;
        case LE_TONE_MAPPING_REINHARD:       le_tone_map_reinhard(out, inAmbient, inColor);               break;
    }
}

static inline void le_calculate_light_contribution(struct LELight* light, Vec3f pos, Vec3f normal, f32 lightIntensityScalar, Vec3f out_color, f32* weight) {
    // skip 'inactive' lights
    if (light->intensity <= 0 || light->radius <= 0) { return; }

    // vector to light
    f32 diffX = light->posX - pos[0];
    f32 diffY = light->posY - pos[1];
    f32 diffZ = light->posZ - pos[2];

    // squared distance check
    f32 dist2 = (diffX * diffX) + (diffY * diffY) + (diffZ * diffZ);
    f32 radius2 = light->radius * light->radius;
    if (dist2 > radius2 || dist2 <= 0) { return; }

    // attenuation & intensity
    f32 att = 1.0f - (dist2 / radius2);
    f32 brightness = att * light->intensity * lightIntensityScalar;

    // normalize diff
    f32 invLen = 1.0f / sqrtf(dist2);
    diffX *= invLen;
    diffY *= invLen;
    diffZ *= invLen;

    if (light->useSurfaceNormals && normal) {
        // lambert term
        f32 nl = (normal[0] * diffX) + (normal[1] * diffY) + (normal[2] * diffZ);
        if (nl <= 0.0f) { return; }

        // modulate by normal
        brightness *= nl;
    }

    // accumulate
    out_color[0] += light->colorR * brightness;
    out_color[1] += light->colorG * brightness;
    out_color[2] += light->colorB * brightness;
    *weight += brightness;
}

void le_calculate_vertex_lighting(Vtx_t* v, Vec3f pos, Color out) {
    if (!le_pos_may_be_affected_by_lights(pos)) {
        Color vtxAmbient = {
            v->cn[0] * (gLEAmbientColor[0] / 255.0f),
            v->cn[1] * (gLEAmbientColor[1] / 255.0f),
            v->cn[2] * (gLEAmbientColor[2] / 255.0f),
        };
        out[0] = vtxAmbient[0];
        out[1] = vtxAmbient[1];
        out[2] = vtxAmbient[2];
        return;
    }

    // clear color
    Vec3f color = { 0 };

    // accumulate lighting
    f32 weight = 1.0f;
    if (!le_cache_get(pos, NULL, 1.0f, color, &weight)) {
        vec3f_set(color, 0, 0, 0);
        weight = 1.0f;
        for (s16 i = 0; i < sActiveLightCount; i++) {
            struct LELight* light = &sLights[sActiveLightIds[i]];
            le_calculate_light_contribution(light, pos, NULL, 1.0f, color, &weight);
        }
        le_cache_put(pos, NULL, 1.0f, color, weight);
    }

    // tone map and output
    Color vtxAmbient = {
        v->cn[0] * (gLEAmbientColor[0] / 255.0f),
        v->cn[1] * (gLEAmbientColor[1] / 255.0f),
        v->cn[2] * (gLEAmbientColor[2] / 255.0f),
    };
    le_tone_map(out, vtxAmbient, color, weight);
}

void le_calculate_lighting_color(Vec3f pos, Color out, f32 lightIntensityScalar) {
    if (!le_pos_may_be_affected_by_lights(pos)) {
        color_copy(out, gLEAmbientColor);
        return;
    }

    // clear color
    Vec3f color = { 0 };

    // accumulate lighting
    f32 weight = 1.0f;
    if (!le_cache_get(pos, NULL, lightIntensityScalar, color, &weight)) {
        vec3f_set(color, 0, 0, 0);
        weight = 1.0f;
        for (s16 i = 0; i < sActiveLightCount; i++) {
            struct LELight* light = &sLights[sActiveLightIds[i]];
            le_calculate_light_contribution(light, pos, NULL, lightIntensityScalar, color, &weight);
        }
        le_cache_put(pos, NULL, lightIntensityScalar, color, weight);
    }

    // tone map and output
    le_tone_map(out, gLEAmbientColor, color, weight);
}

void le_calculate_lighting_color_with_normal(Vec3f pos, Vec3f normal, Color out, f32 lightIntensityScalar) {
    if (!le_pos_may_be_affected_by_lights(pos)) {
        color_copy(out, gLEAmbientColor);
        return;
    }

    // normalize normal
    if (normal) { vec3f_normalize(normal); }

    // clear color
    Vec3f color = { 0 };

    // accumulate lighting
    f32 weight = 1.0f;
    if (!le_cache_get(pos, normal, lightIntensityScalar, color, &weight)) {
        vec3f_set(color, 0, 0, 0);
        weight = 1.0f;
        for (s16 i = 0; i < sActiveLightCount; i++) {
            struct LELight* light = &sLights[sActiveLightIds[i]];
            le_calculate_light_contribution(light, pos, normal, lightIntensityScalar, color, &weight);
        }
        le_cache_put(pos, normal, lightIntensityScalar, color, weight);
    }

    // tone map and output
    le_tone_map(out, gLEAmbientColor, color, weight);
}

void le_calculate_lighting_dir(Vec3f pos, Vec3f out) {
    Vec3f lightingDir = { 0, 0, 0 };
    s16 count = 1;

    for (s16 i = 0; i < LE_MAX_LIGHTS; i++) {
        struct LELight* light = &sLights[i];
        if (!light->added) { continue; }

        f32 diffX = light->posX - pos[0];
        f32 diffY = light->posY - pos[1];
        f32 diffZ = light->posZ - pos[2];
        f32 dist = (diffX * diffX) + (diffY * diffY) + (diffZ * diffZ);
        f32 radius = light->radius * light->radius;
        if (dist > radius) { continue; }

        Vec3f dir = {
            pos[0] - light->posX,
            pos[1] - light->posY,
            pos[2] - light->posZ,
        };
        vec3f_normalize(dir);

        f32 intensity = (1 - (dist / radius)) * light->intensity;
        lightingDir[0] += dir[0] * intensity;
        lightingDir[1] += dir[1] * intensity;
        lightingDir[2] += dir[2] * intensity;

        count++;
    }

    out[0] = lightingDir[0] / (f32)(count);
    out[1] = lightingDir[1] / (f32)(count);
    out[2] = lightingDir[2] / (f32)(count);
    vec3f_normalize(out);
}

s16 le_add_light(f32 x, f32 y, f32 z, u8 r, u8 g, u8 b, f32 radius, f32 intensity) {
    struct LELight* newLight = NULL;
    s16 lightID = -1;

    for (s16 i = 0; i < LE_MAX_LIGHTS; i++) {
        struct LELight* light = &sLights[i];
        if (!light->added) {
            newLight = light;
            lightID = i;
            break;
        }
    }
    if (newLight == NULL) { return -1; }

    newLight->posX = x;
    newLight->posY = y;
    newLight->posZ = z;
    newLight->colorR = r;
    newLight->colorG = g;
    newLight->colorB = b;
    newLight->radius = radius;
    newLight->intensity = intensity;
    newLight->added = true;
    newLight->useSurfaceNormals = true;

    sEnabled = true;
    if (lightID >= 0 && sActiveLightCount < LE_MAX_LIGHTS) {
        sActiveLightIds[sActiveLightCount++] = lightID;
    }
    sLightRevision++;
    return lightID;
}

void le_remove_light(s16 id) {
    if (id < 0 || id >= LE_MAX_LIGHTS) { return; }

    memset(&sLights[id], 0, sizeof(struct LELight));

    for (s16 i = 0; i < sActiveLightCount; i++) {
        if (sActiveLightIds[i] == id) {
            sActiveLightCount--;
            sActiveLightIds[i] = sActiveLightIds[sActiveLightCount];
            break;
        }
    }
    sLightRevision++;
}

s16 le_get_light_count(void) {
    return sActiveLightCount;
}

bool le_light_exists(s16 id) {
    if (id < 0 || id >= LE_MAX_LIGHTS) { return false; }
    return sLights[id].added;
}

void le_get_light_pos(s16 id, VEC_OUT Vec3f out) {
    if (id < 0 || id >= LE_MAX_LIGHTS) { return; }

    struct LELight* light = &sLights[id];
    if (!light->added) { return; }
    vec3f_set(out, light->posX, light->posY, light->posZ);
}

void le_set_light_pos(s16 id, f32 x, f32 y, f32 z) {
    if (id < 0 || id >= LE_MAX_LIGHTS) { return; }

    struct LELight* light = &sLights[id];
    if (!light->added) { return; }
    if (fabsf(light->posX - x) <= LE_FLOAT_EPSILON && fabsf(light->posY - y) <= LE_FLOAT_EPSILON && fabsf(light->posZ - z) <= LE_FLOAT_EPSILON) {
        return;
    }
    light->posX = x;
    light->posY = y;
    light->posZ = z;
    sLightRevision++;
}

void le_get_light_color(s16 id, VEC_OUT Color out) {
    if (id < 0 || id >= LE_MAX_LIGHTS) { return; }

    struct LELight* light = &sLights[id];
    if (!light->added) { return; }
    color_set(out, light->colorR, light->colorG, light->colorB);
}

void le_set_light_color(s16 id, u8 r, u8 g, u8 b) {
    if (id < 0 || id >= LE_MAX_LIGHTS) { return; }

    struct LELight* light = &sLights[id];
    if (!light->added) { return; }
    if (light->colorR == r && light->colorG == g && light->colorB == b) { return; }
    light->colorR = r;
    light->colorG = g;
    light->colorB = b;
    sLightRevision++;
}

f32 le_get_light_radius(s16 id) {
    if (id < 0 || id >= LE_MAX_LIGHTS) { return 0.0f; }

    struct LELight* light = &sLights[id];
    if (!light->added) { return 0.0f; }
    return light->radius;
}

void le_set_light_radius(s16 id, f32 radius) {
    if (id < 0 || id >= LE_MAX_LIGHTS) { return; }

    struct LELight* light = &sLights[id];
    if (!light->added) { return; }
    if (fabsf(light->radius - radius) <= LE_FLOAT_EPSILON) { return; }
    light->radius = radius;
    sLightRevision++;
}

f32 le_get_light_intensity(s16 id) {
    if (id < 0 || id >= LE_MAX_LIGHTS) { return 0.0f; }

    struct LELight* light = &sLights[id];
    if (!light->added) { return 0.0f; }
    return light->intensity;
}

void le_set_light_intensity(s16 id, f32 intensity) {
    if (id < 0 || id >= LE_MAX_LIGHTS) { return; }

    struct LELight* light = &sLights[id];
    if (!light->added) { return; }
    if (fabsf(light->intensity - intensity) <= LE_FLOAT_EPSILON) { return; }
    light->intensity = intensity;
    sLightRevision++;
}

bool le_get_light_use_surface_normals(s16 id) {
    if (id < 0 || id >= LE_MAX_LIGHTS) { return false; }

    struct LELight* light = &sLights[id];
    if (!light->added) { return false; }
    return light->useSurfaceNormals;
}

void le_set_light_use_surface_normals(s16 id, bool useSurfaceNormals) {
    if (id < 0 || id >= LE_MAX_LIGHTS) { return; }

    struct LELight* light = &sLights[id];
    if (!light->added) { return; }
    if (light->useSurfaceNormals == useSurfaceNormals) { return; }
    light->useSurfaceNormals = useSurfaceNormals;
    sLightRevision++;
}

void le_clear(void) {
    memset(&sLights, 0, sizeof(struct LELight) * LE_MAX_LIGHTS);
    sActiveLightCount = 0;
    sLightRevision++;
    sLightBoundsRevision = 0;
    sHasInfluentialLights = false;

    gLEAmbientColor[0] = 127;
    gLEAmbientColor[1] = 127;
    gLEAmbientColor[2] = 127;
}

void le_shutdown(void) {
    sEnabled = false;
    sMode = LE_MODE_AFFECT_ALL_SHADED_AND_COLORED;
    sToneMapping = LE_TONE_MAPPING_WEIGHTED;
    le_clear();
}
