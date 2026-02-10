#include "djui.h"
#include "djui_panel.h"
#include "djui_panel_menu.h"
#include "djui_panel_qol.h"
#include "djui_panel_qol_general.h"
#include "djui_panel_qol_bugfixes.h"

void djui_panel_qol_create(struct DjuiBase* caller) {
    struct DjuiThreePanel* panel = djui_panel_menu_create("QoL", false);
    struct DjuiBase* body = djui_three_panel_get_body(panel);
    {
        djui_button_create(body, "General", DJUI_BUTTON_STYLE_NORMAL, djui_panel_qol_general_create);
        djui_button_create(body, "Bugfixes", DJUI_BUTTON_STYLE_NORMAL, djui_panel_qol_bugfixes_create);

        djui_button_create(body, DLANG(MENU, BACK), DJUI_BUTTON_STYLE_BACK, djui_panel_menu_back);
    }

    djui_panel_add(caller, panel, NULL);
}
