#include "sm64.h"
#include "djui.h"
#include "djui_panel.h"
#include "djui_panel_player.h"
#include "djui_panel_dynos.h"
#include "djui_panel_options.h"
#include "djui_panel_menu.h"
#include "djui_panel_confirm.h"
#include "djui_panel_mod_menu.h"
#include "djui_panel_mods.h"
#include "pc/mods/mods.h"
#include "pc/pc_main.h"
#include "pc/configfile.h"
#include "pc/lua/smlua_hooks.h"
#include "game/object_helpers.h"
#include "behavior_table.h"
#include "game/camera.h"
#include "game/ingame_menu.h"
#include "game/sound_init.h"
#include "game/level_update.h"
#include "data/dynos.c.h"

bool gDjuiPanelPauseCreated = false;

static void djui_panel_pause_quit(struct DjuiBase* caller);
static void djui_panel_pause_customize_create(struct DjuiBase* caller);
static void djui_panel_pause_mods_root_create(struct DjuiBase* caller);

static void djui_panel_pause_exit_to_main_menu(UNUSED struct DjuiBase* caller) {
    if (gMarioStates[0].action == ACT_PUSHING_DOOR || gMarioStates[0].action == ACT_PULLING_DOOR) { return; }

    extern s16 gPauseScreenMode;
    raise_background_noise(1);
    gCameraMovementFlags &= ~CAM_MOVE_PAUSE_SCREEN;
    gPauseScreenMode = 0;
    set_menu_mode(-1);
    set_play_mode(PLAY_MODE_NORMAL);

    djui_panel_shutdown();
    fade_into_special_warp(SPECIAL_WARP_GODDARD, 0);
}

static void djui_panel_pause_resume(UNUSED struct DjuiBase* caller) {
    djui_panel_shutdown();
}

static bool djui_panel_pause_has_dynos_packs(void) {
    int packCount = dynos_pack_get_count();
    for (int i = 0; i < packCount; i++) {
        if (dynos_pack_get_exists(i)) {
            return true;
        }
    }
    return false;
}

static int djui_panel_pause_count_mod_menu_mods(void) {
    struct Mod* addedMods[MAX_HOOKED_MOD_MENU_ELEMENTS] = { 0 };
    int modCount = 0;
    for (int i = 0; i < gHookedModMenuElementsCount; i++) {
        struct LuaHookedModMenuElement* hooked = &gHookedModMenuElements[i];
        bool shouldContinue = false;
        for (int j = 0; j < MAX_HOOKED_MOD_MENU_ELEMENTS; j++) {
            if (addedMods[j] == NULL) { break; }
            if (addedMods[j] == hooked->mod) {
                shouldContinue = true;
                break;
            }
        }
        if (shouldContinue) { continue; }
        addedMods[modCount++] = hooked->mod;
    }
    return modCount;
}

static void djui_panel_pause_customize_create(struct DjuiBase* caller) {
    struct DjuiThreePanel* panel = djui_panel_menu_create(DLANG(PLAYER, PLAYER), false);
    struct DjuiBase* body = djui_three_panel_get_body(panel);
    {
        bool dynosExists = djui_panel_pause_has_dynos_packs();

        struct DjuiRect* row = djui_rect_container_create(body, 64);
        {
            djui_button_left_create(&row->base, DLANG(PLAYER, MODEL), DJUI_BUTTON_STYLE_NORMAL, djui_panel_character_create);
            if (dynosExists) {
                djui_button_right_create(&row->base, DLANG(PAUSE, DYNOS_PACKS), DJUI_BUTTON_STYLE_NORMAL, djui_panel_dynos_create);
            }
        }

        djui_button_create(body, DLANG(MENU, BACK), DJUI_BUTTON_STYLE_BACK, djui_panel_menu_back);
    }

    djui_panel_add(caller, panel, NULL);
}

static void djui_panel_pause_mods_root_create(struct DjuiBase* caller) {
    struct DjuiThreePanel* panel = djui_panel_menu_create(DLANG(HOST_MODS, MODS), false);
    struct DjuiBase* body = djui_three_panel_get_body(panel);
    {
        djui_button_create(body, DLANG(HOST_MODS, MODS), DJUI_BUTTON_STYLE_NORMAL, djui_panel_mods_create);

        int modCount = djui_panel_pause_count_mod_menu_mods();
        if (modCount == 1) {
            struct LuaHookedModMenuElement* hooked = &gHookedModMenuElements[0];
            char buffer[256] = { 0 };
            if (gHookedModMenuElementsCount == 1 && gHookedModMenuElements[0].element == MOD_MENU_ELEMENT_BUTTON) {
                snprintf(buffer, 256, "%s - %s", hooked->mod->name, hooked->name);
                struct DjuiButton* button = djui_button_create(body, buffer, DJUI_BUTTON_STYLE_NORMAL, djui_panel_mod_menu_mod_button);
                button->base.tag = 0;
            } else {
                snprintf(buffer, 256, "%s", hooked->mod->name);
                struct DjuiButton* button = djui_button_create(body, buffer, DJUI_BUTTON_STYLE_NORMAL, djui_panel_mod_menu_mod_create);
                button->base.tag = hooked->mod->index;
            }
        } else if (modCount > 1) {
            djui_button_create(body, DLANG(PAUSE, MOD_MENU), DJUI_BUTTON_STYLE_NORMAL, djui_panel_mod_menu_create);
        }

        djui_button_create(body, DLANG(MENU, BACK), DJUI_BUTTON_STYLE_BACK, djui_panel_menu_back);
    }

    djui_panel_add(caller, panel, NULL);
}

void djui_panel_pause_quit_yes(UNUSED struct DjuiBase* caller) {
    game_exit();
}

static void djui_panel_pause_quit(struct DjuiBase* caller) {
    if (gMarioStates[0].action == ACT_PUSHING_DOOR || gMarioStates[0].action == ACT_PULLING_DOOR) { return; }

    djui_panel_confirm_create(caller,
                            DLANG(MAIN, QUIT_TITLE),
                            DLANG(MAIN, QUIT_CONFIRM),
                            djui_panel_pause_quit_yes);
}

void djui_panel_pause_create(struct DjuiBase* caller) {
    if (gDjuiPanelPauseCreated) { return; }
    if (gDjuiChatBoxFocus) { djui_chat_box_toggle(); }

    struct DjuiBase* defaultBase = NULL;
    struct DjuiThreePanel* panel = djui_panel_menu_create(DLANG(PAUSE, PAUSE_TITLE), false);
    struct DjuiBase* body = djui_three_panel_get_body(panel);
    {
        struct DjuiButton* resume = djui_button_create(body, DLANG(PAUSE, RESUME), DJUI_BUTTON_STYLE_NORMAL, djui_panel_pause_resume);
        defaultBase = &resume->base;
        djui_button_create(body, DLANG(PLAYER, PLAYER), DJUI_BUTTON_STYLE_NORMAL, djui_panel_pause_customize_create);
        djui_button_create(body, DLANG(PAUSE, OPTIONS), DJUI_BUTTON_STYLE_NORMAL, djui_panel_options_create);

        if (gLocalMods.entryCount > 0) {
            djui_button_create(body, DLANG(HOST_MODS, MODS), DJUI_BUTTON_STYLE_NORMAL, djui_panel_pause_mods_root_create);
        }

        djui_button_create(body, "Exit to Main Menu", DJUI_BUTTON_STYLE_BACK, djui_panel_pause_exit_to_main_menu);
        djui_button_create(body, DLANG(MAIN, QUIT), DJUI_BUTTON_STYLE_BACK, djui_panel_pause_quit);
    }

    djui_panel_add(caller, panel, defaultBase);
    gInteractableOverridePad = true;
    gDjuiPanelPauseCreated = true;
}
