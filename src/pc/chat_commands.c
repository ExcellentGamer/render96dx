#include "pc/lua/smlua_hooks.h"
#include "pc/djui/djui_chat_message.h"
#include "chat_commands.h"
#ifdef DEVELOPMENT
#include "pc/dev/chat.h"
#endif

bool exec_chat_command(char* command) {
#ifdef DEVELOPMENT
    if (exec_dev_chat_command(command)) {
        return true;
    }
#endif

    return smlua_call_chat_command_hook(command);
}

void display_chat_commands(void) {
#ifdef DEVELOPMENT
    dev_display_chat_commands();
#endif
    smlua_display_chat_commands();
}
