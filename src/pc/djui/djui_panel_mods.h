#pragma once
#include "djui.h"

struct ModCategory {
    const char* langKey;
    const char* category;
};

void djui_panel_mods_create(struct DjuiBase* caller);
