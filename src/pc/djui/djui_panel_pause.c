#include "sm64.h"
#include "djui.h"
#include "djui_panel.h"
#include "djui_panel_player.h"
#include "djui_panel_dynos.h"
#include "djui_panel_options.h"
#include "djui_panel_menu.h"
#include "djui_panel_confirm.h"
#include "djui_panel_mod_menu.h"
#include "pc/pc_main.h"
#include "pc/configfile.h"
#include "pc/lua/smlua_hooks.h"
#include "game/object_helpers.h"
#include "behavior_table.h"
#include "game/level_update.h"
#include "data/dynos.c.h"

bool gDjuiPanelPauseCreated = false;

static void djui_panel_pause_quit(struct DjuiBase* caller);

static void djui_panel_pause_resume(UNUSED struct DjuiBase* caller) {
    djui_panel_shutdown();
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
        struct DjuiButton* button1 = djui_button_create(body, DLANG(PLAYER, MODEL), DJUI_BUTTON_STYLE_NORMAL, djui_panel_character_create);
        defaultBase = &button1->base;

        bool dynosExists = false;
        int packCount = dynos_pack_get_count();
        for (int i = 0; i < packCount; i++) {
            if (dynos_pack_get_exists(i)) {
                dynosExists = true;
                break;
            }
        }
        if (dynosExists) {
            djui_button_create(body, DLANG(PAUSE, DYNOS_PACKS), DJUI_BUTTON_STYLE_NORMAL, djui_panel_dynos_create);
        }

        djui_button_create(body, DLANG(PAUSE, OPTIONS), DJUI_BUTTON_STYLE_NORMAL, djui_panel_options_create);

        struct Mod* addedMods[MAX_HOOKED_MOD_MENU_ELEMENTS] = { 0 };
        int modCount = 0;
        for (int i = 0; i < gHookedModMenuElementsCount; i++) {
            struct LuaHookedModMenuElement* hooked = &gHookedModMenuElements[i];
            bool shouldContinue = false;
            for (int i = 0; i < MAX_HOOKED_MOD_MENU_ELEMENTS; i++) {
                if (addedMods[i] == NULL) { break; }
                if (addedMods[i] == hooked->mod) {
                    shouldContinue = true;
                    break;
                }
            }
            if (shouldContinue) { continue; }
            addedMods[modCount++] = hooked->mod;
        }

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
        } else if (gHookedModMenuElementsCount > 0) {
            djui_button_create(body, DLANG(PAUSE, MOD_MENU), DJUI_BUTTON_STYLE_NORMAL, djui_panel_mod_menu_create);
        }

        djui_button_create(body, DLANG(PAUSE, RESUME), DJUI_BUTTON_STYLE_NORMAL, djui_panel_pause_resume);

        djui_button_create(body, DLANG(MAIN, QUIT), DJUI_BUTTON_STYLE_BACK, djui_panel_pause_quit);
    }

    djui_panel_add(caller, panel, defaultBase);
    gInteractableOverridePad = true;
    gDjuiPanelPauseCreated = true;
}
