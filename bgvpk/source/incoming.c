typedef struct SceIncomingDialogParam {
    int fw; //ex. 0x1650041
    char titleid[0x10]; //ex. "PCSA00044" (icon0.png of that app will be shown in dialog window)
    char audioPath[0x80]; //ex. "app0:resource/CallRingingIn.mp3" .at9 and .aac also supported (audio will be played while dialog is active, playback is done by sceShell)
    unsigned int dialogTimer; //Time to show dialog in seconds (including audio)
    int unk_BC; //ex. 1
    char reserved1[0x3E];
    char buttonRightText[0x3E]; //UTF-16 (function - accept. Opens app from titleid)
    short separator0; //must be 0
    char buttonLeftText[0x3E]; //UTF-16 (function - reject). If 0, only right button will be created
    short separator1; //must be 0
    char dialogWindowText[0x100]; //UTF-16 (also displayed in first notification)
    short separator2; //must be 0
} SceIncomingDialogParam;

void copycon(char* str1, const char* str2) {
    while (*str2) {
        *str1 = *str2;
        str1++;
        *str1 = '\0';
        str1++;
        str2++;
    }
}

int dialog_init(int type) {
    sceSysmoduleLoadModule(SCE_SYSMODULE_INCOMING_DIALOG);
    return sceIncomingDialogInitialize(type);
}

int dialog_show(char *caller_id, const char *message, const char *accept_msg, const char *cancel_msg) {
    SceIncomingDialogParam params;
    sceClibMemset(&params, 0, sizeof(SceIncomingDialogParam));
    params.fw = 0x00931011;
    params.dialogTimer = 216000;
    if (caller_id)
        sceClibStrncpy(params.titleid, caller_id, sizeof(params.titleid));
    if (accept_msg)
        copycon(params.buttonRightText, accept_msg);
    if (cancel_msg)
        copycon(params.buttonLeftText, cancel_msg);
    if (message)
        copycon(params.dialogWindowText, message);
    return sceIncomingDialogOpen(&params);
}

int dialog_wait(void) {
    int state = 2;
    while (state != 3 && state != 4) {
        state = sceIncomingDialogGetStatus();
    };
    return (state == 4) ? 1 : 0;
}

int dialog_close(void) {
    return sceIncomingDialogClose();
}

int dialog_deinit(void) {
    sceIncomingDialogFinish();
    return sceSysmoduleUnloadModule(SCE_SYSMODULE_INCOMING_DIALOG);
}