#define BGVPK_MAGIC 'BG'
#define BGVPK_CFG_VER 1

typedef struct bgvpk_export_param_struct {
    uint16_t magic; // bgvpk magic | cfg version
    uint8_t show_dlg; // show user dialog before installing
    uint8_t target; // 0 - ux0:data, 1- app
    char titleid[12]; // titleid of the app that dlgs
} bgvpk_export_param_struct;