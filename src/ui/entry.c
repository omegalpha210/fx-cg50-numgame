#include "app.h"
bool ng_entry_level(const NgApp *a)
{
 /* Classic 2048 has no goal selector. MASTER local strategy has tactical starts. */
 return !(a->selected_id==26 && a->settings.mode[25]==0);
}
bool ng_mode_chooser(unsigned id)
{
 const NgModule *m=ng_module(id);if(m->modes<=1)return false;
 int width=0;for(unsigned i=0;i<m->modes;i++)width+=ng_text_width(m->mode_name(i),1)+12;
 return m->modes>3 || width>228;
}
unsigned ng_entry_count(const NgApp *a)
{return (a->active?1u:0u)+1u+(ng_entry_level(a)?1u:0u)+(ng_module(a->selected_id)->modes>1?1u:0u);}
int ng_entry_action(const NgApp *a,unsigned row)
{
 if(a->active){if(!row)return NG_ENTRY_RESUME;row--;}
 if(!row)return NG_ENTRY_NEW;
 row--;
 if(ng_entry_level(a)){if(!row)return NG_ENTRY_LEVEL;row--;}
 return !row && ng_module(a->selected_id)->modes>1?NG_ENTRY_MODE:-1;
}
unsigned ng_entry_row(const NgApp *a,int action)
{for(unsigned i=0;i<ng_entry_count(a);i++)if(ng_entry_action(a,i)==action)return i;return 0;}
