#ifndef NUMGAME_APP_H
#define NUMGAME_APP_H
#include "storage.h"
#include "ui.h"
enum { NG_MAIN,NG_CATEGORY,NG_ENTRY,NG_PLAY,NG_STATS,NG_SETTINGS };
enum { NG_MODAL_NONE,NG_MODAL_INIT,NG_MODAL_NEW,NG_MODAL_RULES,
 NG_MODAL_RESULT,NG_MODAL_PAUSE,NG_MODAL_SAVE_ERROR,NG_MODAL_RECORDS,NG_MODAL_DIAGNOSTICS,NG_MODAL_MODE,NG_MODAL_EVICT };
enum { NG_DOWN,NG_UP,NG_HOLD };
typedef struct {
 void *context;
 int (*load)(void *,NgSession *,unsigned);
 bool (*save)(void *,NgSession *);
 int (*load_settings)(void *,NgSettings *);
 bool (*save_settings)(void *,NgSettings *);
 void (*os_menu)(void *);
 void (*power_off)(void *);
 bool (*remove_game)(void *,unsigned);
 int (*load_state)(void *,NgSettings *,NgSession *,bool *);
 bool (*save_state)(void *,NgSettings *,const NgSession *,bool);
} NgHooks;
typedef struct {
 NgSession session;
 NgGame before;
 NgSettings settings;
 NgSummary summary[NG_ID_MAX];
 NgHooks hooks;
 uint64_t held,blocked;
 uint32_t epoch,seed;
 uint8_t screen,modal,category,selection,entry_selection,selected_id;
 uint8_t stats_category,stats_page,rules_scroll;
 uint8_t record_mode,record_difficulty,record_assisted;
 uint8_t previous_level[NG_ID_MAX],mode_choice,diag_page;
 bool shift_pending,alpha_pending,dirty,settings_dirty,active,resumable,result_view,save_failed;
 bool start_failed,start_failed_result;
 bool target_editing,target_replace;
 uint8_t target_cursor;
 char target_draft[5];
 uint32_t backlight_ms,apo_ms;
 bool power_os_settings,power_available;
 char notice[96];
} NgApp;
void ng_app_init(NgApp *a,NgHooks hooks,uint32_t seed);
bool ng_app_event(NgApp *a,int key,int type);
bool ng_app_tick(NgApp *a,uint32_t delta_ms);
bool ng_app_cpu(NgApp *a);
bool ng_checkpoint(NgApp *a);
void ng_app_barrier(NgApp *a);
int ng_key_index(int key);
void ng_render(const NgApp *a,NgCanvas *c);
void ng_render_hud(const NgApp *a,NgCanvas *c);
void ng_app_poweroff(NgApp *a);
/* Visible entry rows; shared by presentation and its navigation only. */
enum {NG_ENTRY_RESUME,NG_ENTRY_NEW,NG_ENTRY_LEVEL,NG_ENTRY_MODE,NG_ENTRY_TARGET};
bool ng_entry_level(const NgApp *a);
unsigned ng_entry_count(const NgApp *a);
int ng_entry_action(const NgApp *a,unsigned row);
unsigned ng_entry_row(const NgApp *a,int action);
bool ng_mode_chooser(unsigned id);
#endif
