#include "app.h"
bool ng_entry_level(const NgApp *a)
{
 /* Classic 2048 has no target-level selector. */
 return !(a->selected_id==26 && a->settings.mode[25]==0);
}
bool ng_mode_chooser(unsigned id)
{
 (void)id;return false;
}
unsigned ng_entry_count(const NgApp *a)
{return (a->resumable && a->session.game.id==a->selected_id?1u:0u)+1u+(ng_entry_level(a)?1u:0u)+(a->selected_id==6?1u:0u)+(ng_module(a->selected_id)->modes>1?1u:0u);}
int ng_entry_action(const NgApp *a,unsigned row)
{
 if(a->resumable && a->session.game.id==a->selected_id){if(!row)return NG_ENTRY_RESUME;row--;}
 if(!row)return NG_ENTRY_NEW;
 row--;
 if(ng_entry_level(a)){if(!row)return NG_ENTRY_LEVEL;row--;}
 if(a->selected_id==6){if(!row)return NG_ENTRY_TARGET;row--;}
 return !row && ng_module(a->selected_id)->modes>1?NG_ENTRY_MODE:-1;
}
unsigned ng_entry_row(const NgApp *a,int action)
{for(unsigned i=0;i<ng_entry_count(a);i++)if(ng_entry_action(a,i)==action)return i;return 0;}
