#include "djui.h"
#include "djui_panel.h"
#include "djui_panel_menu.h"
#include "djui_panel_qol_bugfixes.h"
#include "pc/configfile.h"

void djui_panel_qol_bugfixes_create(struct DjuiBase* caller) {
    struct DjuiThreePanel* panel = djui_panel_menu_create("QoL - Bugfixes", false);
    struct DjuiBase* body = djui_three_panel_get_body(panel);
    {
        djui_checkbox_create(body, "Fix max lives/coins overflow", &configBugfixMaxLives, NULL);
        djui_checkbox_create(body, "Fix King Bob-omb music fade", &configBugfixKingBobOmbFadeMusic, NULL);
        djui_checkbox_create(body, "Fix Koopa race music on warp", &configBugfixKoopaRaceMusic, NULL);
        djui_checkbox_create(body, "Fix piranha plant state reset", &configBugfixPiranhaPlantStateReset, NULL);
        djui_checkbox_create(body, "Fix sleeping piranha plant damage", &configBugfixPiranhaPlantSleepDamage, NULL);
        djui_checkbox_create(body, "Fix Bowser key showing as star", &configBugfixStarBowserKey, NULL);

        djui_button_create(body, DLANG(MENU, BACK), DJUI_BUTTON_STYLE_BACK, djui_panel_menu_back);
    }

    djui_panel_add(caller, panel, NULL);
}
