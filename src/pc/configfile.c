// configfile.c - handles loading and saving the configuration options
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "platform.h"
#include "configfile.h"
#include "cliopts.h"
#include "gfx/gfx_screen_config.h"
#include "gfx/gfx_window_manager_api.h"
#include "controller/controller_api.h"
#include "fs/fs.h"
#include "mods/mods.h"
#include "crash_handler.h"
#include "debuglog.h"
#include "djui/djui_hud_utils.h"
#include "pc/platform.h"
#include "game/save_file.h"
#include "pc/network/network_player.h"
#include "pc/pc_main.h"
#include "game/characters.h"
#include "game/player_palette.h"

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

enum ConfigOptionType {
    CONFIG_TYPE_BOOL,
    CONFIG_TYPE_UINT,
    CONFIG_TYPE_FLOAT,
    CONFIG_TYPE_BIND,
    CONFIG_TYPE_STRING,
    CONFIG_TYPE_U64,
    CONFIG_TYPE_COLOR,
};

struct ConfigOption {
    const char *name;
    enum ConfigOptionType type;
    union {
        bool *boolValue;
        unsigned int *uintValue;
        float* floatValue;
        char* stringValue;
        u64* u64Value;
        u8 (*colorValue)[3];
    };
    int maxStringLength;
};

struct FunctionConfigOption {
    const char *name;
    void (*read)(char**, int);
    void (*write)(FILE*);
};

/*
 *Config options and default values
 */
char configSaveNames[4][MAX_SAVE_NAME_STRING] = {
    "SM64",
    "SM64",
    "SM64",
    "SM64"
};

// Video/audio stuff
ConfigWindow configWindow       = {
    .x = WAPI_WIN_CENTERPOS,
    .y = WAPI_WIN_CENTERPOS,
    .w = DESIRED_SCREEN_WIDTH,
    .h = DESIRED_SCREEN_HEIGHT,
    .vsync = 1,
    .reset = false,
    .fullscreen = false,
    .exiting_fullscreen = false,
    .settings_changed = false,
    .msaa = 0,
};

ConfigStick configStick = { 0 };

// display settings
unsigned int configFiltering                      = 2; // 0 = Nearest, 1 = Bilinear, 2 = Trilinear
bool         configShowFPS                        = false;
enum RefreshRateMode configFramerateMode          = RRM_AUTO;
unsigned int configFrameLimit                     = 60;
unsigned int configInterpolationMode              = 1;
unsigned int configDrawDistance                   = 4;
// sound settings
unsigned int configMasterVolume                   = 80; // 0 - MAX_VOLUME
unsigned int configMusicVolume                    = MAX_VOLUME;
unsigned int configSfxVolume                      = MAX_VOLUME;
unsigned int configEnvVolume                      = MAX_VOLUME;
bool         configFadeoutDistantSounds           = false;
bool         configMuteFocusLoss                  = false;
// control binds
unsigned int configKeyA[MAX_BINDS]                = { 0x0026,     0x1000,     0x1101     };
unsigned int configKeyB[MAX_BINDS]                = { 0x0033,     0x1001,     0x1103     };
unsigned int configKeyX[MAX_BINDS]                = { 0x0017,     0x1002,     VK_INVALID };
unsigned int configKeyY[MAX_BINDS]                = { 0x0032,     0x1003,     VK_INVALID };
unsigned int configKeyStart[MAX_BINDS]            = { 0x0039,     0x1006,     VK_INVALID };
unsigned int configKeyL[MAX_BINDS]                = { 0x002A,     0x1009,     0x1104     };
unsigned int configKeyR[MAX_BINDS]                = { 0x0036,     0x100A,     0x101B     };
unsigned int configKeyZ[MAX_BINDS]                = { 0x0025,     0x1007,     0x101A     };
unsigned int configKeyCUp[MAX_BINDS]              = { 0x0148,     VK_INVALID, VK_INVALID };
unsigned int configKeyCDown[MAX_BINDS]            = { 0x0150,     VK_INVALID, VK_INVALID };
unsigned int configKeyCLeft[MAX_BINDS]            = { 0x014B,     VK_INVALID, VK_INVALID };
unsigned int configKeyCRight[MAX_BINDS]           = { 0x014D,     VK_INVALID, VK_INVALID };
unsigned int configKeyStickUp[MAX_BINDS]          = { 0x0011,     VK_INVALID, VK_INVALID };
unsigned int configKeyStickDown[MAX_BINDS]        = { 0x001F,     VK_INVALID, VK_INVALID };
unsigned int configKeyStickLeft[MAX_BINDS]        = { 0x001E,     VK_INVALID, VK_INVALID };
unsigned int configKeyStickRight[MAX_BINDS]       = { 0x0020,     VK_INVALID, VK_INVALID };
unsigned int configKeyDUp[MAX_BINDS]              = { 0x0147,     0x100b,     VK_INVALID };
unsigned int configKeyDDown[MAX_BINDS]            = { 0x014f,     0x100c,     VK_INVALID };
unsigned int configKeyDLeft[MAX_BINDS]            = { 0x0153,     0x100d,     VK_INVALID };
unsigned int configKeyDRight[MAX_BINDS]           = { 0x0151,     0x100e,     VK_INVALID };
unsigned int configStickDeadzone                  = 16;
unsigned int configRumbleStrength                 = 50;
unsigned int configGamepadNumber                  = 0;
bool         configBackgroundGamepad              = true;
bool         configDisableGamepads                = false;
bool         configSmoothScrolling                = false;
// free camera settings
bool         configEnableFreeCamera               = false;
bool         configFreeCameraAnalog               = false;
bool         configFreeCameraLCentering           = false;
bool         configFreeCameraDPadBehavior         = false;
bool         configFreeCameraHasCollision         = true;
bool         configFreeCameraMouse                = false;
unsigned int configFreeCameraXSens                = 50;
unsigned int configFreeCameraYSens                = 50;
unsigned int configFreeCameraAggr                 = 0;
unsigned int configFreeCameraPan                  = 0;
unsigned int configFreeCameraDegrade              = 50; // 0 - 100%
// romhack camera settings
unsigned int configEnableRomhackCamera            = 0; // 0 for automatic, 1 for force on, 2 for force off
bool         configRomhackCameraBowserFights      = false;
bool         configRomhackCameraHasCollision      = true;
bool         configRomhackCameraHasCentering      = false;
bool         configRomhackCameraDPadBehavior      = false;
bool         configRomhackCameraSlowFall          = true;

// common camera settings
bool         configCameraInvertX                  = false;
bool         configCameraInvertY                  = true;
bool         configCameraToxicGas                 = true;
// debug
bool         configLuaProfiler                    = false;
bool         configDebugPrint                     = false;
bool         configDebugInfo                      = false;
bool         configDebugError                     = false;
#ifdef DEVELOPMENT
bool         configCtxProfiler                    = false;
#endif
// player settings
unsigned int configPlayerModel                    = 0;
struct PlayerPalette configPlayerPalette          = { { { 0x00, 0x00, 0xff }, { 0xff, 0x00, 0x00 }, { 0xff, 0xff, 0xff }, { 0x72, 0x1c, 0x0e }, { 0x73, 0x06, 0x00 }, { 0xfe, 0xc1, 0x79 }, { 0xff, 0x00, 0x00 }, { 0xff, 0x00, 0x00 } } };
struct PlayerPalette configPlayerPalettes[5]      = { 0 };
bool configPlayerPaletteCustomEnabled[5]          = { false };
struct PlayerPalette configPlayerPaletteCustom[5] = { 0 };
char configPlayerPalettePresetMario[MAX_CONFIG_STRING]   = "Mario";
char configPlayerPalettePresetLuigi[MAX_CONFIG_STRING]   = "Luigi";
char configPlayerPalettePresetToad[MAX_CONFIG_STRING]    = "Toad";
char configPlayerPalettePresetWaluigi[MAX_CONFIG_STRING] = "Waluigi";
char configPlayerPalettePresetWario[MAX_CONFIG_STRING]   = "Wario";
// coop settings
unsigned int configAmountOfPlayers                = 1;
bool         configBubbleDeath                    = true;
unsigned int configPlayerInteraction              = 1;
unsigned int configPlayerKnockbackStrength        = 25;
unsigned int configStayInLevelAfterStar           = 0;
bool         configNametags                       = true;
bool         configModDevMode                     = false;
unsigned int configBouncyLevelBounds              = 0;
bool         configSkipIntro                      = 0;
bool         configPauseAnywhere                  = false;
bool         configMenuStaffRoll                  = false;
unsigned int configMenuLevel                      = 0;
unsigned int configMenuSound                      = 0;
bool         configMenuRandom                     = false;
bool         configMenuDemos                      = false;
char         configLanguage[MAX_CONFIG_STRING]    = "";
bool         configForce4By3                      = false;
unsigned int configPvpType                        = PLAYER_PVP_CLASSIC;
// DJUI settings
unsigned int configDjuiTheme                      = DJUI_THEME_DARK;
#ifdef HANDHELD
bool         configDjuiThemeCenter                = false;
#else
bool         configDjuiThemeCenter                = true;
#endif
bool         configDjuiThemeGradients             = true;
unsigned int configDjuiThemeFont                  = FONT_NORMAL;
unsigned int configDjuiScale                      = 0;
// other
unsigned int configRulesVersion                   = 0;
bool         configCompressOnStartup              = false;
bool         configSkipPackGeneration             = false;

// secrets
bool configExCoopTheme = false;

static char* configfile_get_character_preset_name(enum CharacterType c) {
    switch (c) {
        case CT_MARIO:   return configPlayerPalettePresetMario;
        case CT_LUIGI:   return configPlayerPalettePresetLuigi;
        case CT_TOAD:    return configPlayerPalettePresetToad;
        case CT_WALUIGI: return configPlayerPalettePresetWaluigi;
        case CT_WARIO:   return configPlayerPalettePresetWario;
        default:         return configPlayerPalettePresetMario;
    }
}

void configfile_set_character_palette_preset(unsigned int characterIndex, const char* presetName) {
    if (presetName == NULL || presetName[0] == '\0') { return; }
    if (characterIndex >= CT_MAX) { return; }

    char* dst = configfile_get_character_preset_name((enum CharacterType)characterIndex);
    snprintf(dst, MAX_CONFIG_STRING, "%s", presetName);

    configPlayerPaletteCustomEnabled[characterIndex] = false;

    struct PlayerPalette palette = DEFAULT_MARIO_PALETTE;
    if (player_palette_get_preset(dst, &palette)) {
        configPlayerPalettes[characterIndex] = palette;
        configfile_sync_player_palette();
    }
}

void configfile_reset_character_palette(unsigned int characterIndex) {
    if (characterIndex >= CT_MAX) { return; }
    configfile_set_character_palette_preset(characterIndex, gCharacters[characterIndex].name);
}

void configfile_reset_all_character_palettes(void) {
    for (unsigned int c = 0; c < CT_MAX; c++) {
        configfile_reset_character_palette(c);
    }
    configfile_sync_player_palette();
}

void configfile_sync_player_palette(void) {
    u8 modelIndex = (u8)configPlayerModel;
    if (modelIndex >= CT_MAX) { modelIndex = CT_MARIO; }
    configPlayerPalette = configPlayerPalettes[modelIndex];
}

void configfile_init_player_palettes(void) {
    player_palettes_reset();
    player_palettes_read(sys_exe_path_dir(), true);
    player_palettes_read(fs_get_write_path(PALETTES_DIRECTORY), false);

    for (int c = 0; c < CT_MAX; c++) {
        struct PlayerPalette palette = DEFAULT_MARIO_PALETTE;

        char* presetName = configfile_get_character_preset_name((enum CharacterType)c);
        if (presetName == NULL || presetName[0] == '\0') {
            presetName = gCharacters[c].name;
            snprintf(configfile_get_character_preset_name((enum CharacterType)c), MAX_CONFIG_STRING, "%s", presetName);
        }

        if (!player_palette_get_preset(presetName, &palette)) {
            if (player_palette_get_preset(gCharacters[c].name, &palette)) {
                snprintf(configfile_get_character_preset_name((enum CharacterType)c), MAX_CONFIG_STRING, "%s", gCharacters[c].name);
            }
        }

        configPlayerPalettes[c] = palette;

        if (configPlayerPaletteCustomEnabled[c]) {
            configPlayerPalettes[c] = configPlayerPaletteCustom[c];
        }
    }

    configfile_sync_player_palette();
}

static const struct ConfigOption options[] = {
    // window settings
    {.name = "fullscreen",                     .type = CONFIG_TYPE_BOOL, .boolValue = &configWindow.fullscreen},
    {.name = "window_x",                       .type = CONFIG_TYPE_UINT, .uintValue = &configWindow.x},
    {.name = "window_y",                       .type = CONFIG_TYPE_UINT, .uintValue = &configWindow.y},
    {.name = "window_w",                       .type = CONFIG_TYPE_UINT, .uintValue = &configWindow.w},
    {.name = "window_h",                       .type = CONFIG_TYPE_UINT, .uintValue = &configWindow.h},
    {.name = "vsync",                          .type = CONFIG_TYPE_BOOL, .boolValue = &configWindow.vsync},
    {.name = "msaa",                           .type = CONFIG_TYPE_UINT, .uintValue = &configWindow.msaa},
    // display settings
    {.name = "texture_filtering",              .type = CONFIG_TYPE_UINT, .uintValue = &configFiltering},
    {.name = "show_fps",                       .type = CONFIG_TYPE_BOOL, .boolValue = &configShowFPS},
    {.name = "framerate_mode",                 .type = CONFIG_TYPE_UINT, .uintValue = &configFramerateMode},
    {.name = "frame_limit",                    .type = CONFIG_TYPE_UINT, .uintValue = &configFrameLimit},
    {.name = "interpolation_mode",             .type = CONFIG_TYPE_UINT, .uintValue = &configInterpolationMode},
    {.name = "coop_draw_distance",             .type = CONFIG_TYPE_UINT, .uintValue = &configDrawDistance},
    // sound settings
    {.name = "master_volume",                  .type = CONFIG_TYPE_UINT, .uintValue = &configMasterVolume},
    {.name = "music_volume",                   .type = CONFIG_TYPE_UINT, .uintValue = &configMusicVolume},
    {.name = "sfx_volume",                     .type = CONFIG_TYPE_UINT, .uintValue = &configSfxVolume},
    {.name = "env_volume",                     .type = CONFIG_TYPE_UINT, .uintValue = &configEnvVolume},
    {.name = "fade_distant_sounds",            .type = CONFIG_TYPE_BOOL, .boolValue = &configFadeoutDistantSounds},
    {.name = "mute_focus_loss",                .type = CONFIG_TYPE_BOOL, .boolValue = &configMuteFocusLoss},
    // control binds
    {.name = "key_a",                          .type = CONFIG_TYPE_BIND, .uintValue = configKeyA},
    {.name = "key_b",                          .type = CONFIG_TYPE_BIND, .uintValue = configKeyB},
    {.name = "key_x",                          .type = CONFIG_TYPE_BIND, .uintValue = configKeyX},
    {.name = "key_y",                          .type = CONFIG_TYPE_BIND, .uintValue = configKeyY},
    {.name = "key_start",                      .type = CONFIG_TYPE_BIND, .uintValue = configKeyStart},
    {.name = "key_l",                          .type = CONFIG_TYPE_BIND, .uintValue = configKeyL},
    {.name = "key_r",                          .type = CONFIG_TYPE_BIND, .uintValue = configKeyR},
    {.name = "key_z",                          .type = CONFIG_TYPE_BIND, .uintValue = configKeyZ},
    {.name = "key_cup",                        .type = CONFIG_TYPE_BIND, .uintValue = configKeyCUp},
    {.name = "key_cdown",                      .type = CONFIG_TYPE_BIND, .uintValue = configKeyCDown},
    {.name = "key_cleft",                      .type = CONFIG_TYPE_BIND, .uintValue = configKeyCLeft},
    {.name = "key_cright",                     .type = CONFIG_TYPE_BIND, .uintValue = configKeyCRight},
    {.name = "key_stickup",                    .type = CONFIG_TYPE_BIND, .uintValue = configKeyStickUp},
    {.name = "key_stickdown",                  .type = CONFIG_TYPE_BIND, .uintValue = configKeyStickDown},
    {.name = "key_stickleft",                  .type = CONFIG_TYPE_BIND, .uintValue = configKeyStickLeft},
    {.name = "key_stickright",                 .type = CONFIG_TYPE_BIND, .uintValue = configKeyStickRight},
    {.name = "key_dup",                        .type = CONFIG_TYPE_BIND, .uintValue = configKeyDUp},
    {.name = "key_ddown",                      .type = CONFIG_TYPE_BIND, .uintValue = configKeyDDown},
    {.name = "key_dleft",                      .type = CONFIG_TYPE_BIND, .uintValue = configKeyDLeft},
    {.name = "key_dright",                     .type = CONFIG_TYPE_BIND, .uintValue = configKeyDRight},
    {.name = "stick_deadzone",                 .type = CONFIG_TYPE_UINT, .uintValue = &configStickDeadzone},
    {.name = "rumble_strength",                .type = CONFIG_TYPE_UINT, .uintValue = &configRumbleStrength},
    {.name = "gamepad_number",                 .type = CONFIG_TYPE_UINT, .uintValue = &configGamepadNumber},
    {.name = "background_gamepad",             .type = CONFIG_TYPE_UINT, .boolValue = &configBackgroundGamepad},
#ifndef HANDHELD
    {.name = "disable_gamepads",               .type = CONFIG_TYPE_BOOL, .boolValue = &configDisableGamepads},
#endif
    {.name = "smooth_scrolling",               .type = CONFIG_TYPE_BOOL, .boolValue = &configSmoothScrolling},
    {.name = "stick_rotate_left",              .type = CONFIG_TYPE_BOOL, .boolValue = &configStick.rotateLeft},
    {.name = "stick_invert_left_x",            .type = CONFIG_TYPE_BOOL, .boolValue = &configStick.invertLeftX},
    {.name = "stick_invert_left_y",            .type = CONFIG_TYPE_BOOL, .boolValue = &configStick.invertLeftY},
    {.name = "stick_rotate_right",             .type = CONFIG_TYPE_BOOL, .boolValue = &configStick.rotateRight},
    {.name = "stick_invert_right_x",           .type = CONFIG_TYPE_BOOL, .boolValue = &configStick.invertRightX},
    {.name = "stick_invert_right_y",           .type = CONFIG_TYPE_BOOL, .boolValue = &configStick.invertRightY},
    // free camera settings
    {.name = "bettercam_enable",               .type = CONFIG_TYPE_BOOL, .boolValue = &configEnableFreeCamera},
    {.name = "bettercam_analog",               .type = CONFIG_TYPE_BOOL, .boolValue = &configFreeCameraAnalog},
    {.name = "bettercam_centering",            .type = CONFIG_TYPE_BOOL, .boolValue = &configFreeCameraLCentering},
    {.name = "bettercam_dpad",                 .type = CONFIG_TYPE_BOOL, .boolValue = &configFreeCameraDPadBehavior},
    {.name = "bettercam_collision",            .type = CONFIG_TYPE_BOOL, .boolValue = &configFreeCameraHasCollision},
    {.name = "bettercam_mouse_look",           .type = CONFIG_TYPE_BOOL, .boolValue = &configFreeCameraMouse},
    {.name = "bettercam_xsens",                .type = CONFIG_TYPE_UINT, .uintValue = &configFreeCameraXSens},
    {.name = "bettercam_ysens",                .type = CONFIG_TYPE_UINT, .uintValue = &configFreeCameraYSens},
    {.name = "bettercam_aggression",           .type = CONFIG_TYPE_UINT, .uintValue = &configFreeCameraAggr},
    {.name = "bettercam_pan_level",            .type = CONFIG_TYPE_UINT, .uintValue = &configFreeCameraPan},
    {.name = "bettercam_degrade",              .type = CONFIG_TYPE_UINT, .uintValue = &configFreeCameraDegrade},
    // romhack camera settings
    {.name = "romhackcam_enable",              .type = CONFIG_TYPE_UINT, .uintValue = &configEnableRomhackCamera},
    {.name = "romhackcam_bowser",              .type = CONFIG_TYPE_BOOL, .boolValue = &configRomhackCameraBowserFights},
    {.name = "romhackcam_collision",           .type = CONFIG_TYPE_BOOL, .boolValue = &configRomhackCameraHasCollision},
    {.name = "romhackcam_centering",           .type = CONFIG_TYPE_BOOL, .boolValue = &configRomhackCameraHasCentering},
    {.name = "romhackcam_dpad",                .type = CONFIG_TYPE_BOOL, .boolValue = &configRomhackCameraDPadBehavior},
    {.name = "romhackcam_slowfall",            .type = CONFIG_TYPE_BOOL, .boolValue = &configRomhackCameraSlowFall},
    // common camera settings
    {.name = "bettercam_invertx",              .type = CONFIG_TYPE_BOOL, .boolValue = &configCameraInvertX},
    {.name = "bettercam_inverty",              .type = CONFIG_TYPE_BOOL, .boolValue = &configCameraInvertY},
    {.name = "romhackcam_toxic_gas",           .type = CONFIG_TYPE_BOOL, .boolValue = &configCameraToxicGas},
    // debug
    {.name = "debug_offset",                   .type = CONFIG_TYPE_U64,  .u64Value    = &gPcDebug.bhvOffset},
    {.name = "debug_tags",                     .type = CONFIG_TYPE_U64,  .u64Value    = gPcDebug.tags},
    {.name = "lua_profiler",                   .type = CONFIG_TYPE_BOOL, .boolValue   = &configLuaProfiler},
    {.name = "debug_print",                    .type = CONFIG_TYPE_BOOL, .boolValue   = &configDebugPrint},
    {.name = "debug_info",                     .type = CONFIG_TYPE_BOOL, .boolValue   = &configDebugInfo},
    {.name = "debug_error",                    .type = CONFIG_TYPE_BOOL, .boolValue   = &configDebugError},
#ifdef DEVELOPMENT
    {.name = "ctx_profiler",                   .type = CONFIG_TYPE_BOOL, .boolValue   = &configCtxProfiler},
#endif
    // player settings
    {.name = "player_palette_preset_mario",    .type = CONFIG_TYPE_STRING, .stringValue = configPlayerPalettePresetMario,   .maxStringLength = MAX_CONFIG_STRING},
    {.name = "player_palette_preset_luigi",    .type = CONFIG_TYPE_STRING, .stringValue = configPlayerPalettePresetLuigi,   .maxStringLength = MAX_CONFIG_STRING},
    {.name = "player_palette_preset_toad",     .type = CONFIG_TYPE_STRING, .stringValue = configPlayerPalettePresetToad,    .maxStringLength = MAX_CONFIG_STRING},
    {.name = "player_palette_preset_waluigi",  .type = CONFIG_TYPE_STRING, .stringValue = configPlayerPalettePresetWaluigi, .maxStringLength = MAX_CONFIG_STRING},
    {.name = "player_palette_preset_wario",    .type = CONFIG_TYPE_STRING, .stringValue = configPlayerPalettePresetWario,   .maxStringLength = MAX_CONFIG_STRING},

    {.name = "player_palette_custom_enabled_mario",   .type = CONFIG_TYPE_BOOL,  .boolValue  = &configPlayerPaletteCustomEnabled[CT_MARIO]},
    {.name = "player_palette_custom_enabled_luigi",   .type = CONFIG_TYPE_BOOL,  .boolValue  = &configPlayerPaletteCustomEnabled[CT_LUIGI]},
    {.name = "player_palette_custom_enabled_toad",    .type = CONFIG_TYPE_BOOL,  .boolValue  = &configPlayerPaletteCustomEnabled[CT_TOAD]},
    {.name = "player_palette_custom_enabled_waluigi", .type = CONFIG_TYPE_BOOL,  .boolValue  = &configPlayerPaletteCustomEnabled[CT_WALUIGI]},
    {.name = "player_palette_custom_enabled_wario",   .type = CONFIG_TYPE_BOOL,  .boolValue  = &configPlayerPaletteCustomEnabled[CT_WARIO]},

    {.name = "player_palette_custom_mario_pants",   .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_MARIO].parts[PANTS]},
    {.name = "player_palette_custom_mario_shirt",   .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_MARIO].parts[SHIRT]},
    {.name = "player_palette_custom_mario_gloves",  .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_MARIO].parts[GLOVES]},
    {.name = "player_palette_custom_mario_shoes",   .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_MARIO].parts[SHOES]},
    {.name = "player_palette_custom_mario_hair",    .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_MARIO].parts[HAIR]},
    {.name = "player_palette_custom_mario_skin",    .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_MARIO].parts[SKIN]},
    {.name = "player_palette_custom_mario_cap",     .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_MARIO].parts[CAP]},
    {.name = "player_palette_custom_mario_emblem",  .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_MARIO].parts[EMBLEM]},

    {.name = "player_palette_custom_luigi_pants",   .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_LUIGI].parts[PANTS]},
    {.name = "player_palette_custom_luigi_shirt",   .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_LUIGI].parts[SHIRT]},
    {.name = "player_palette_custom_luigi_gloves",  .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_LUIGI].parts[GLOVES]},
    {.name = "player_palette_custom_luigi_shoes",   .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_LUIGI].parts[SHOES]},
    {.name = "player_palette_custom_luigi_hair",    .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_LUIGI].parts[HAIR]},
    {.name = "player_palette_custom_luigi_skin",    .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_LUIGI].parts[SKIN]},
    {.name = "player_palette_custom_luigi_cap",     .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_LUIGI].parts[CAP]},
    {.name = "player_palette_custom_luigi_emblem",  .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_LUIGI].parts[EMBLEM]},

    {.name = "player_palette_custom_toad_pants",   .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_TOAD].parts[PANTS]},
    {.name = "player_palette_custom_toad_shirt",   .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_TOAD].parts[SHIRT]},
    {.name = "player_palette_custom_toad_gloves",  .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_TOAD].parts[GLOVES]},
    {.name = "player_palette_custom_toad_shoes",   .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_TOAD].parts[SHOES]},
    {.name = "player_palette_custom_toad_hair",    .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_TOAD].parts[HAIR]},
    {.name = "player_palette_custom_toad_skin",    .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_TOAD].parts[SKIN]},
    {.name = "player_palette_custom_toad_cap",     .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_TOAD].parts[CAP]},
    {.name = "player_palette_custom_toad_emblem",  .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_TOAD].parts[EMBLEM]},

    {.name = "player_palette_custom_waluigi_pants",   .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_WALUIGI].parts[PANTS]},
    {.name = "player_palette_custom_waluigi_shirt",   .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_WALUIGI].parts[SHIRT]},
    {.name = "player_palette_custom_waluigi_gloves",  .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_WALUIGI].parts[GLOVES]},
    {.name = "player_palette_custom_waluigi_shoes",   .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_WALUIGI].parts[SHOES]},
    {.name = "player_palette_custom_waluigi_hair",    .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_WALUIGI].parts[HAIR]},
    {.name = "player_palette_custom_waluigi_skin",    .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_WALUIGI].parts[SKIN]},
    {.name = "player_palette_custom_waluigi_cap",     .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_WALUIGI].parts[CAP]},
    {.name = "player_palette_custom_waluigi_emblem",  .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_WALUIGI].parts[EMBLEM]},

    {.name = "player_palette_custom_wario_pants",   .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_WARIO].parts[PANTS]},
    {.name = "player_palette_custom_wario_shirt",   .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_WARIO].parts[SHIRT]},
    {.name = "player_palette_custom_wario_gloves",  .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_WARIO].parts[GLOVES]},
    {.name = "player_palette_custom_wario_shoes",   .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_WARIO].parts[SHOES]},
    {.name = "player_palette_custom_wario_hair",    .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_WARIO].parts[HAIR]},
    {.name = "player_palette_custom_wario_skin",    .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_WARIO].parts[SKIN]},
    {.name = "player_palette_custom_wario_cap",     .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_WARIO].parts[CAP]},
    {.name = "player_palette_custom_wario_emblem",  .type = CONFIG_TYPE_COLOR, .colorValue = &configPlayerPaletteCustom[CT_WARIO].parts[EMBLEM]},

    {.name = "skip_intro",                     .type = CONFIG_TYPE_BOOL,   .boolValue   = &configSkipIntro},
    {.name = "pause_anywhere",                 .type = CONFIG_TYPE_BOOL,   .boolValue   = &configPauseAnywhere},
    {.name = "coop_menu_staff_roll",           .type = CONFIG_TYPE_BOOL,   .boolValue   = &configMenuStaffRoll},
    {.name = "coop_menu_level",                .type = CONFIG_TYPE_UINT,   .uintValue   = &configMenuLevel},
    {.name = "coop_menu_sound",                .type = CONFIG_TYPE_UINT,   .uintValue   = &configMenuSound},
    {.name = "coop_menu_random",               .type = CONFIG_TYPE_BOOL,   .boolValue   = &configMenuRandom},
    {.name = "player_pvp_mode",                .type = CONFIG_TYPE_UINT,   .uintValue   = &configPvpType},
    // {.name = "coop_menu_demos",                .type = CONFIG_TYPE_BOOL,   .boolValue   = &configMenuDemos},
    {.name = "language",                       .type = CONFIG_TYPE_STRING, .stringValue = (char*)&configLanguage, .maxStringLength = MAX_CONFIG_STRING},
    {.name = "force_4by3",                     .type = CONFIG_TYPE_BOOL,   .boolValue   = &configForce4By3},
    // DJUI settings
    {.name = "djui_theme",                     .type = CONFIG_TYPE_UINT,   .uintValue   = &configDjuiTheme},
    {.name = "djui_theme_center",              .type = CONFIG_TYPE_BOOL,   .boolValue   = &configDjuiThemeCenter},
    {.name = "djui_theme_gradients",           .type = CONFIG_TYPE_BOOL,   .boolValue   = &configDjuiThemeGradients},
    {.name = "djui_theme_font",                .type = CONFIG_TYPE_UINT,   .uintValue   = &configDjuiThemeFont},
    {.name = "djui_scale",                     .type = CONFIG_TYPE_UINT,   .uintValue   = &configDjuiScale},
    // other
    {.name = "rules_version",                  .type = CONFIG_TYPE_UINT,   .uintValue   = &configRulesVersion},
    {.name = "compress_on_startup",            .type = CONFIG_TYPE_BOOL,   .boolValue   = &configCompressOnStartup},
    {.name = "skip_pack_generation",           .type = CONFIG_TYPE_BOOL,   .boolValue   = &configSkipPackGeneration},
};

struct SecretConfigOption {
    const char *name;
    enum ConfigOptionType type;
    union {
        bool *boolValue;
        unsigned int *uintValue;
        float* floatValue;
        char* stringValue;
        u64* u64Value;
        u8 (*colorValue)[3];
    };
    int maxStringLength;
    bool inConfig;
};

static struct SecretConfigOption secret_options[] = {
    {.name = "ex_coop_theme", .type = CONFIG_TYPE_BOOL, .boolValue = &configExCoopTheme},
};

// FunctionConfigOption functions

struct QueuedFile {
    char* path;
    struct QueuedFile *next;
};

static struct QueuedFile *sQueuedEnableModsHead = NULL;

void enable_queued_mods(void) {
    while (sQueuedEnableModsHead) {
        struct QueuedFile *next = sQueuedEnableModsHead->next;
        mods_enable(sQueuedEnableModsHead->path);
        free(sQueuedEnableModsHead->path);
        free(sQueuedEnableModsHead);
        sQueuedEnableModsHead = next;
    }
}

static bool configfile_read_legacy_multiplayer_option(char** tokens, int numTokens) {
    if (tokens == NULL || numTokens < 2) { return false; }

    if (strcmp(tokens[0], "amount_of_players") == 0) {
        sscanf(tokens[1], "%u", &configAmountOfPlayers);
        return true;
    }
    if (strcmp(tokens[0], "bubble_death") == 0) {
        configBubbleDeath = (strcmp(tokens[1], "true") == 0);
        return true;
    }
    if (strcmp(tokens[0], "coop_host_port") == 0) {
        return true;
    }
    if (strcmp(tokens[0], "coop_host_save_slot") == 0) {
        return true;
    }
    if (strcmp(tokens[0], "coop_join_ip") == 0) {
        return true;
    }
    if (strcmp(tokens[0], "coop_join_port") == 0) {
        return true;
    }
    if (strcmp(tokens[0], "coop_network_system") == 0) {
        return true;
    }
    if (strcmp(tokens[0], "coop_player_interaction") == 0) {
        sscanf(tokens[1], "%u", &configPlayerInteraction);
        return true;
    }
    if (strcmp(tokens[0], "coop_player_knockback_strength") == 0) {
        sscanf(tokens[1], "%u", &configPlayerKnockbackStrength);
        return true;
    }
    if (strcmp(tokens[0], "coop_stay_in_level_after_star") == 0) {
        sscanf(tokens[1], "%u", &configStayInLevelAfterStar);
        return true;
    }
    if (strcmp(tokens[0], "coop_nametags") == 0) {
        configNametags = (strcmp(tokens[1], "true") == 0);
        return true;
    }
    if (strcmp(tokens[0], "coop_mod_dev_mode") == 0) {
        configModDevMode = (strcmp(tokens[1], "true") == 0);
        return true;
    }
    if (strcmp(tokens[0], "coop_bouncy_bounds") == 0) {
        sscanf(tokens[1], "%u", &configBouncyLevelBounds);
        return true;
    }
    if (strcmp(tokens[0], "coopnet_ip") == 0) {
        return true;
    }
    if (strcmp(tokens[0], "coopnet_port") == 0) {
        return true;
    }
    if (strcmp(tokens[0], "coopnet_password") == 0) {
        return true;
    }
    if (strcmp(tokens[0], "coopnet_dest") == 0) {
        return true;
    }

    if (strcmp(tokens[0], "ban:") == 0) {
        return true;
    }

    if (strcmp(tokens[0], "moderator:") == 0) {
        return true;
    }

    return false;
}

static void enable_mod_read(char** tokens, UNUSED int numTokens) {
    if (gCLIOpts.disableMods) { return; }

    char combined[256] = { 0 };
    for (int i = 1; i < numTokens; i++) {
        if (i != 1) { strncat(combined, " ", 255); }
        strncat(combined, tokens[i], 255);
    }

    struct QueuedFile* queued = malloc(sizeof(struct QueuedFile));
    queued->path = strdup(combined);
    queued->next = NULL;
    if (!sQueuedEnableModsHead) {
        sQueuedEnableModsHead = queued;
    } else {
        struct QueuedFile* tail = sQueuedEnableModsHead;
        while (tail->next) { tail = tail->next; }
        tail->next = queued;
    }
}

static void enable_mod(char* mod) {
    struct QueuedFile* queued = malloc(sizeof(struct QueuedFile));
    queued->path = mod;
    queued->next = NULL;
    if (!sQueuedEnableModsHead) {
        sQueuedEnableModsHead = queued;
    } else {
        struct QueuedFile* tail = sQueuedEnableModsHead;
        while (tail->next) { tail = tail->next; }
        tail->next = queued;
    }
}

static void enable_mod_write(FILE* file) {
    for (unsigned int i = 0; i < gLocalMods.entryCount; i++) {
        struct Mod* mod = gLocalMods.entries[i];
        if (mod == NULL) { continue; }
        if (!mod->enabled) { continue; }
        fprintf(file, "%s %s\n", "enable-mod:", mod->relativePath);
    }
}

static struct QueuedFile *sQueuedEnableDynosPacksHead = NULL;

void enable_queued_dynos_packs(void) {
    while (sQueuedEnableDynosPacksHead) {
        int packCount = dynos_pack_get_count();
        const char *path = sQueuedEnableDynosPacksHead->path;
        for (int i = 0; i < packCount; i++) {
            const char* pack = dynos_pack_get_name(i);
            if (!strcmp(path, pack)) {
                dynos_pack_set_enabled(i, true);
                break;
            }
        }

        struct QueuedFile *next = sQueuedEnableDynosPacksHead->next;
        free(sQueuedEnableDynosPacksHead->path);
        free(sQueuedEnableDynosPacksHead);
        sQueuedEnableDynosPacksHead = next;
    }
}

static void dynos_pack_read(char** tokens, int numTokens) {
    if (numTokens < 2) { return; }
    if (strcmp(tokens[numTokens-1], "false") == 0) { return; } // Only accept enabled packs. Default is disabled. (old coop config compatibility)
    char fullPackName[256] = { 0 };
    for (int i = 1; i < numTokens; i++) {
        if (i == numTokens - 1 && strcmp(tokens[i], "true") == 0) { break; } // old coop config compatibility
        if (i != 1) { strncat(fullPackName, " ", 255); }
        strncat(fullPackName, tokens[i], 255);
    }

    struct QueuedFile* queued = malloc(sizeof(struct QueuedFile));
    queued->path = strdup(fullPackName);
    queued->next = NULL;
    if (!sQueuedEnableDynosPacksHead) {
        sQueuedEnableDynosPacksHead = queued;
    } else {
        struct QueuedFile* tail = sQueuedEnableDynosPacksHead;
        while (tail->next) { tail = tail->next; }
        tail->next = queued;
    }
}

static void dynos_pack_write(FILE* file) {
    int packCount = dynos_pack_get_count();
    for (int i = 0; i < packCount; i++) {
        if (dynos_pack_get_enabled(i)) {
            const char* pack = dynos_pack_get_name(i);
            fprintf(file, "%s %s\n", "dynos-pack:", pack);
        }
    }
}

static void save_name_read(char** tokens, int numTokens) {
    if (numTokens < 2) { return; }
    char fullSaveName[MAX_SAVE_NAME_STRING] = { 0 };
    int index = 0;
    for (int i = 1; i < numTokens; i++) {
        if (i == 1) {
            index = atoi(tokens[i]);
        } else {
            if (i > 2) {
                strncat(fullSaveName, " ", MAX_SAVE_NAME_STRING - 1);
            }
            strncat(fullSaveName, tokens[i], MAX_SAVE_NAME_STRING - 1);
        }

    }
    snprintf(configSaveNames[index], MAX_SAVE_NAME_STRING, "%s", fullSaveName);
}

static void save_name_write(FILE* file) {
    for (int i = 0; i < NUM_SAVE_FILES; i++) {
        fprintf(file, "%s %d %s\n", "save-name:", i, configSaveNames[i]);
    }
}

static const struct FunctionConfigOption functionOptions[] = {
    { .name = "enable-mod:", .read = enable_mod_read, .write = enable_mod_write },
    { .name = "dynos-pack:", .read = dynos_pack_read, .write = dynos_pack_write },
    { .name = "save-name:",  .read = save_name_read,  .write = save_name_write  }
};

// Reads an entire line from a file (excluding the newline character) and returns an allocated string
// Returns NULL if no lines could be read from the file
static char *read_file_line(fs_file_t *file, bool* error) {
    char *buffer;
    size_t bufferSize = 8;
    size_t offset = 0; // offset in buffer to write
    *error = false;

    buffer = malloc(bufferSize);
    buffer[0] = '\0';
    while (1) {
        // Read a line from the file
        if (fs_readline(file, buffer + offset, bufferSize - offset) == NULL) {
            free(buffer);
            return NULL; // Nothing could be read.
        }
        offset = strlen(buffer);
        if (offset <= 0) {
            LOG_ERROR("Configfile offset <= 0");
            *error = true;
            return NULL;
        }


        // If a newline was found, remove the trailing newline and exit
        if (buffer[offset - 1] == '\n') {
            buffer[offset - 1] = '\0';
            break;
        }

        if (fs_eof(file)) // EOF was reached
            break;

        // If no newline or EOF was reached, then the whole line wasn't read.
        bufferSize *= 2; // Increase buffer size
        buffer = realloc(buffer, bufferSize);
        assert(buffer != NULL);
    }

    return buffer;
}

// Returns the position of the first non-whitespace character
static char *skip_whitespace(char *str) {
    while (isspace(*str))
        str++;
    return str;
}

// NULL-terminates the current whitespace-delimited word, and returns a pointer to the next word
static char *word_split(char *str) {
    // Precondition: str must not point to whitespace
    assert(!isspace(*str));

    // Find either the next whitespace char or end of string
    while (!isspace(*str) && *str != '\0')
        str++;
    if (*str == '\0') // End of string
        return str;

    // Terminate current word
    *(str++) = '\0';

    // Skip whitespace to next word
    return skip_whitespace(str);
}

// Splits a string into words, and stores the words into the 'tokens' array
// 'maxTokens' is the length of the 'tokens' array
// Returns the number of tokens parsed
static unsigned int tokenize_string(char *str, int maxTokens, char **tokens) {
    int count = 0;

    str = skip_whitespace(str);
    while (str[0] != '\0' && count < maxTokens) {
        tokens[count] = str;
        str = word_split(str);
        count++;
    }
    return count;
}

// Gets the config file path and caches it
const char *configfile_name(void) {
    return (gCLIOpts.configFile[0]) ? gCLIOpts.configFile : CONFIGFILE_DEFAULT;
}

const char *configfile_backup_name(void) {
    return CONFIGFILE_BACKUP;
}

// Loads the config file specified by 'filename'
static void configfile_load_internal(const char *filename, bool* error) {
    fs_file_t *file;
    char *line;
    unsigned int temp;
    *error = false;

#ifdef DEVELOPMENT
    printf("Loading configuration from '%s'\n", filename);
#endif

    file = fs_open(filename);
    if (file == NULL) {
        // Create a new config file and save defaults
        printf("Config file '%s' not found. Creating it.\n", filename);
        configfile_save(filename);
        return;
    }

    // Go through each line in the file
    while ((line = read_file_line(file, error)) != NULL && !*error) {
        char *p = line;
        char *tokens[20];
        int numTokens;

        // skip whitespace
        while (isspace(*p))
            p++;

        // skip comment or empty line
        if (!*p || *p == '#') {
            free(line);
            continue;
        }

        numTokens = tokenize_string(p, sizeof(tokens) / sizeof(tokens[0]), tokens);
        if (numTokens != 0) {
            if (numTokens >= 2) {
                const struct ConfigOption *option = NULL;

                // find functionOption
                for (unsigned int i = 0; i < ARRAY_LEN(functionOptions); i++) {
                    if (strcmp(tokens[0], functionOptions[i].name) == 0) {
                        functionOptions[i].read(tokens, numTokens);
                        goto NEXT_OPTION;
                    }
                }

                // find option
                for (unsigned int i = 0; i < ARRAY_LEN(options); i++) {
                    if (strcmp(tokens[0], options[i].name) == 0) {
                        option = &options[i];
                        break;
                    }
                }

                // legacy multiplayer options (read-only)
                if (option == NULL && configfile_read_legacy_multiplayer_option(tokens, numTokens)) {
                    goto NEXT_OPTION;
                }

                // secret options
                for (unsigned int i = 0; i < ARRAY_LEN(secret_options); i++) {
                    if (strcmp(tokens[0], secret_options[i].name) == 0) {
                        secret_options[i].inConfig = true;
                        option = (const struct ConfigOption *) &secret_options[i];
                        break;
                    }
                }

                if (option == NULL) {
#ifdef DEVELOPMENT
                    printf("unknown option '%s'\n", tokens[0]);
#endif
                } else {
                    switch (option->type) {
                        case CONFIG_TYPE_BOOL:
                            if (strcmp(tokens[1], "true") == 0)
                                *option->boolValue = true;
                            else
                                *option->boolValue = false;
                            break;
                        case CONFIG_TYPE_UINT:
                            sscanf(tokens[1], "%u", option->uintValue);
                            break;
                        case CONFIG_TYPE_BIND:
                            for (int i = 0; i < MAX_BINDS && i < numTokens - 1; ++i)
                                sscanf(tokens[i + 1], "%x", option->uintValue + i);
                            break;
                        case CONFIG_TYPE_FLOAT:
                            sscanf(tokens[1], "%f", option->floatValue);
                            break;
                        case CONFIG_TYPE_STRING:
                            memset(option->stringValue, '\0', option->maxStringLength);
                            snprintf(option->stringValue, option->maxStringLength, "%s", tokens[1]);
                            break;
                        case CONFIG_TYPE_U64:
                            sscanf(tokens[1], "%llu", option->u64Value);
                            break;
                        case CONFIG_TYPE_COLOR:
                            for (int i = 0; i < 3 && i < numTokens - 1; ++i) {
                                sscanf(tokens[i + 1], "%x", &temp);
                                (*option->colorValue)[i] = temp;
                            }
                            break;
                        default:
                            LOG_ERROR("Configfile read bad type '%d': %s", (int)option->type, line);
                            goto NEXT_OPTION;
                    }
#ifdef DEVELOPMENT
                    printf("option: '%s', value:", tokens[0]);
                    for (int i = 1; i < numTokens; ++i) printf(" '%s'", tokens[i]);
                    printf("\n");
#endif
                }
            } else {
#ifdef DEVELOPMENT
                printf("error: expected value\n");
#endif
            }
        }
NEXT_OPTION:
        free(line);
        line = NULL;
    }

    if (line) {
        free(line);
    }

    fs_close(file);

    if (configFramerateMode < 0 || configFramerateMode > RRM_MAX) { configFramerateMode = 0; }
    if (configFrameLimit < 30)   { configFrameLimit = 30; }
    if (configFrameLimit > 3000) { configFrameLimit = 3000; }

    gMasterVolume = (f32)configMasterVolume / 127.0f;

    if (configPlayerModel >= CT_MAX) { configPlayerModel = 0; }

    if (configDjuiTheme >= DJUI_THEME_MAX) { configDjuiTheme = 0; }
    if (configDjuiScale >= 5) { configDjuiScale = 0; }

    if (gCLIOpts.fullscreen == 1) {
        configWindow.fullscreen = true;
    } else if (gCLIOpts.fullscreen == 2) {
        configWindow.fullscreen = false;
    }
    if (gCLIOpts.width != 0) { configWindow.w = gCLIOpts.width; }
    if (gCLIOpts.height != 0) { configWindow.h = gCLIOpts.height; }

    for (int i = 0; i < gCLIOpts.enabledModsCount; i++) {
        enable_mod(gCLIOpts.enableMods[i]);
    }
    free(gCLIOpts.enableMods);

    configAmountOfPlayers = 1;
}

void configfile_load(void) {
    bool configReadError = false;
#ifdef DEVELOPMENT
    configfile_load_internal(configfile_name(), &configReadError);
#else
    configfile_load_internal(configfile_name(), &configReadError);
    if (configReadError) {
        configfile_load_internal(configfile_backup_name(), &configReadError);
    } else {
        configfile_save(configfile_backup_name());
    }
#endif
}

static void configfile_save_option(FILE *file, const struct ConfigOption *option, bool isSecret) {
    if (isSecret) {
        const struct SecretConfigOption *secret_option = (const struct SecretConfigOption *) option;
        if (!secret_option->inConfig) { return; }
    }
    switch (option->type) {
        case CONFIG_TYPE_BOOL:
            fprintf(file, "%s %s\n", option->name, *option->boolValue ? "true" : "false");
            break;
        case CONFIG_TYPE_UINT:
            fprintf(file, "%s %u\n", option->name, *option->uintValue);
            break;
        case CONFIG_TYPE_FLOAT:
            fprintf(file, "%s %f\n", option->name, *option->floatValue);
            break;
        case CONFIG_TYPE_BIND:
            fprintf(file, "%s ", option->name);
            for (int i = 0; i < MAX_BINDS; ++i)
                fprintf(file, "%04x ", option->uintValue[i]);
            fprintf(file, "\n");
            break;
        case CONFIG_TYPE_STRING:
            fprintf(file, "%s %s\n", option->name, option->stringValue);
            break;
        case CONFIG_TYPE_U64:
            fprintf(file, "%s %llu\n", option->name, *option->u64Value);
            break;
        case CONFIG_TYPE_COLOR:
            fprintf(file, "%s %02x %02x %02x\n", option->name, (*option->colorValue)[0], (*option->colorValue)[1], (*option->colorValue)[2]);
            break;
        default:
            LOG_ERROR("Configfile wrote bad type '%d': %s", (int)option->type, option->name);
            break;
    }
}

// Writes the config file to 'filename'
void configfile_save(const char *filename) {
    FILE *file;

    file = fopen(fs_get_write_path(filename), "w");
    if (file == NULL) {
        // error
        return;
    }

    printf("Saving configuration to '%s'\n", filename);

    for (unsigned int i = 0; i < ARRAY_LEN(options); i++) {
        const struct ConfigOption *option = &options[i];
        configfile_save_option(file, option, false);
    }

    for (unsigned int i = 0; i < ARRAY_LEN(secret_options); i++) {
        const struct ConfigOption *option = (const struct ConfigOption *) &secret_options[i];
        configfile_save_option(file, option, true);
    }

    // save function options
    for (unsigned int i = 0; i < ARRAY_LEN(functionOptions); i++) {
        functionOptions[i].write(file);
    }

    fclose(file);
}
