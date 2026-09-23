#include "guesscalc.h"
#include "ui.h"
#include "../../assets/guesscalc_cryptarithm.h"
#include <stdio.h>
#include <string.h>

static const char *standard(unsigned mode){(void)mode;return "STANDARD";}
static bool check_key(int key){return key==NGK_EXE||key==NGK_F6;}
static bool auxiliary_key(int key){return key==NGK_AUX||key==NGK_F4;}
unsigned gc_extra_bank_count(unsigned id,unsigned difficulty,unsigned mode){return id==34&&difficulty<4&&!mode?30:0;}
static uint32_t random_step(uint32_t *state){uint32_t x=*state;x^=x<<13;x^=x>>17;x^=x<<5;*state=x;return x;}
static bool inside(int x,int y,unsigned n){return x>=0&&y>=0&&x<(int)n&&y<(int)n;}
static bool atom_at(const int16_t *atoms,unsigned n,int x,int y){return inside(x,y,n)&&atoms[y*(int)n+x]!=0;}
/* Ports run clockwise: top L->R, right T->B, bottom R->L, left B->T.
 * A direct hit takes precedence. A side atom at entry reflects immediately.
 * Interior diagonal atoms turn the beam away; two reverse it. */
int gc_blackbox_ray(unsigned n,const int16_t *atoms,unsigned port){
 if(n<2||n>8||!atoms||port>=4*n)return -3;
 int x,y,dx,dy;
 if(port<n){x=(int)port;y=-1;dx=0;dy=1;}
 else if(port<2*n){x=(int)n;y=(int)(port-n);dx=-1;dy=0;}
 else if(port<3*n){x=(int)(3*n-1-port);y=(int)n;dx=0;dy=-1;}
 else {x=-1;y=(int)(4*n-1-port);dx=1;dy=0;}
 for(unsigned steps=0;steps<=4*n*n+4;steps++){
  int ahead_x=x+dx,ahead_y=y+dy;
  if(atom_at(atoms,n,ahead_x,ahead_y))return -1;
  bool left=atom_at(atoms,n,ahead_x+dy,ahead_y-dx);
  bool right=atom_at(atoms,n,ahead_x-dy,ahead_y+dx);
  if(!inside(x,y,n)&&(left||right))return -2;
  if(left&&right){dx=-dx;dy=-dy;}
  else if(left){int old=dx;dx=-dy;dy=old;}
  else if(right){int old=dx;dx=dy;dy=-old;}
  else {
   x=ahead_x;y=ahead_y;
   if(!inside(x,y,n)){
    int exit_port=y<0?x:x>=(int)n?(int)n+y:y>=(int)n?(int)(3*n)-1-x:(int)(4*n)-1-y;
    return exit_port==(int)port?-2:exit_port;
   }
  }
 }
 return -3; /* Finite (cell,direction) state bound; never accept a cycle. */
}
static uint32_t blackbox_atoms(uint32_t seed,unsigned n,unsigned count,int16_t *out){
 uint32_t rng=seed?seed:1;memset(out,0,NG_CELLS*sizeof(*out));
 for(unsigned k=0;k<count;k++){
  unsigned rank=random_step(&rng)%(n*n-k);
  for(unsigned i=0;i<n*n;i++)if(!out[i]){if(!rank){out[i]=1;break;}--rank;}
 }
 return rng;
}
static void hidden_atoms(const NgGame *g,int16_t *atoms){for(unsigned i=0;i<NG_CELLS;i++)atoms[i]=g->fixed[i];}
static int ray_encode(int result){return result==-1?1:result==-2?2:result>=0?result+3:0;}
static unsigned marked_atoms(const NgGame *g){unsigned count=0;for(unsigned i=0;i<(unsigned)g->rows*g->cols;i++)count+=g->board[i]==1;return count;}
bool gc_blackbox_complete(const NgGame *g){
 if(!g||g->rows<2||g->rows>8||g->cols!=g->rows||g->data[0]<0||marked_atoms(g)!=(unsigned)g->data[0])return false;
 int16_t candidate[NG_CELLS]={0},hidden[NG_CELLS];hidden_atoms(g,hidden);
 for(unsigned i=0;i<(unsigned)g->rows*g->cols;i++)candidate[i]=g->board[i]==1;
 for(unsigned port=0;port<4u*g->rows;port++){
  int expected=gc_blackbox_ray(g->rows,hidden,port);
  if(expected==-3||gc_blackbox_ray(g->rows,candidate,port)!=expected)return false;
 }
 return true;
}
static void init_blackbox(NgGame *g){
 unsigned n=5+g->difficulty;int16_t atoms[NG_CELLS];
 g->rows=g->cols=(uint8_t)n;g->data[0]=(int32_t)(3+g->difficulty);g->puzzle_id=g->seed;
 g->rng=blackbox_atoms(g->seed,n,(unsigned)g->data[0],atoms);
 for(unsigned i=0;i<NG_CELLS;i++)g->fixed[i]=(uint8_t)atoms[i];
 ng_message(g,"Mark atoms; F4 selects rays. F6 checks your layout.");
}
static void ray_message(NgGame *g,unsigned port){
 int result=g->data[8+port];
 if(result==1)snprintf(g->message,sizeof(g->message),"Port %u: HIT (the beam strikes an atom).",port+1);
 else if(result==2)snprintf(g->message,sizeof(g->message),"Port %u: REFLECT (the beam returns here).",port+1);
 else if(result>=3)snprintf(g->message,sizeof(g->message),"Port %u exits at port %u; the path is reversible.",port+1,(unsigned)result-2);
 else snprintf(g->message,sizeof(g->message),"Port %u is untested. EXE fires a beam.",port+1);
}
static bool action_blackbox(NgGame *g,int key){
 if(auxiliary_key(key)){g->phase^=1;ng_message(g,g->phase?"RAYS: arrows choose a port; EXE fires.":"CELLS: arrows move; 1 atom, 2 exclude, 0 clear.");return true;}
 if(key==NGK_UP||key==NGK_DOWN||key==NGK_LEFT||key==NGK_RIGHT){
  if(!g->phase)return ng_grid_nav(g,key);
  int delta=key==NGK_LEFT?-1:key==NGK_RIGHT?1:key==NGK_UP?-(int)g->rows:g->rows;
  g->data[1]=(g->data[1]+delta+4*g->rows)%(4*g->rows);ray_message(g,(unsigned)g->data[1]);return true;
 }
 if(g->status!=NG_PLAYING||g->moves==UINT32_MAX)return false;
 if(key==NGK_HINT||key==NGK_F3){
  for(unsigned i=0;i<(unsigned)g->rows*g->cols;i++)if(g->fixed[i]&&g->board[i]!=1){
   g->board[i]=1;g->cursor=(uint8_t)i;g->phase=0;g->assisted=1;++g->moves;ng_message(g,"One hidden atom revealed (assisted). Other marks remain.");return true;
  }
  ng_message(g,"Every hidden atom is already marked; check other marks.");return true;
 }
 if(key==NGK_F6){
  if(g->data[3]>=100000)return false;
  ++g->moves;++g->data[3];
  if(gc_blackbox_complete(g)){g->status=NG_WON;ng_message(g,"All ray observations match! Equivalent layouts accepted.");}
  else ng_message(g,"Use the required atom count and match every ray.");
  g->score=(unsigned)g->data[2]+5u*((unsigned)g->data[3]-(g->status==NG_WON));return true;
 }
 if(g->phase){
  if(key!=NGK_EXE)return false;
  unsigned port=(unsigned)g->data[1];
  if(!g->data[8+port]){
   int16_t atoms[NG_CELLS];hidden_atoms(g,atoms);int result=gc_blackbox_ray(g->rows,atoms,port);
   if(result==-3){ng_message(g,"Invalid ray state.");return true;}
   g->data[8+port]=ray_encode(result);g->data[48+port]=1;
   if(result>=0)g->data[8+(unsigned)result]=(int32_t)port+3;
   ++g->data[2];++g->moves;++g->score;
  }
  ray_message(g,port);return true;
 }
 int value=key=='1'?1:key=='2'?2:key=='0'||key==NGK_DEL?0:key==NGK_EXE?(g->board[g->cursor]+1)%3:-1;
 if(value<0)return false;
 if(g->board[g->cursor]!=value){g->board[g->cursor]=(int16_t)value;++g->moves;}
 return true;
}
static bool empty_text(const NgGame *g){
 for(unsigned i=0;i<NG_INPUT;i++)if(g->input[i])return false;
 for(unsigned r=0;r<NG_HISTORY;r++)for(unsigned j=0;j<40;j++)if(g->history[r][j])return false;
 return true;
}
static bool valid_blackbox(const NgGame *g){
 if(g->id!=33||g->difficulty>3||g->mode||g->pack_revision!=2||g->status>NG_WON||g->phase>1||g->rows!=5+g->difficulty||g->cols!=g->rows||g->cursor>=g->rows*g->cols||g->history_count||g->scroll||g->notes_mode||g->cpu_pending||g->turn||!empty_text(g))return false;
 unsigned n=g->rows,ports=4*n;int16_t atoms[NG_CELLS];
 if(g->data[0]!=3+g->difficulty||g->data[1]<0||g->data[1]>=(int)ports||g->data[2]<0||g->data[2]>(int)ports||g->data[3]<0||g->data[3]>100000||g->puzzle_id!=g->seed||g->rng!=blackbox_atoms(g->seed,n,(unsigned)g->data[0],atoms))return false;
 unsigned fired=0;
 for(unsigned i=0;i<NG_CELLS;i++)if(g->fixed[i]!=atoms[i]||g->notes[i]||g->board[i]<0||g->board[i]>2||(i>=n*n&&g->board[i]))return false;
 for(unsigned i=0;i<NG_DATA;i++)if(i>3&&(i<8||i>=8+ports)&&(i<48||i>=48+ports)&&g->data[i])return false;
 for(unsigned port=0;port<ports;port++){
  int result=gc_blackbox_ray(n,atoms,port),encoded=g->data[8+port],shot=g->data[48+port];
  if(result==-3||shot<0||shot>1||(shot&&!encoded)||(encoded&&encoded!=ray_encode(result)))return false;
  if(encoded&&!shot&&(result<0||g->data[48+(unsigned)result]!=1))return false;
  if(encoded&&result>=0&&g->data[8+(unsigned)result]!=(int)port+3)return false;
  fired+=(unsigned)shot;
 }
 if(fired!=(unsigned)g->data[2]||g->moves<fired+(unsigned)g->data[3])return false;
 if(g->status==NG_WON&&(!g->data[3]||!gc_blackbox_complete(g)))return false;
 return g->score==fired+5u*((unsigned)g->data[3]-(g->status==NG_WON));
}
static void port_position(unsigned n,unsigned p,int *x,int *y){
 int s=144/(int)(n+2),ox=20+s,oy=31+s;
 if(p<n){*x=ox+(int)p*s;*y=oy-s;}
 else if(p<2*n){*x=ox+(int)n*s;*y=oy+(int)(p-n)*s;}
 else if(p<3*n){*x=ox+(int)(3*n-1-p)*s;*y=oy+(int)n*s;}
 else {*x=ox-s;*y=oy+(int)(4*n-1-p)*s;}
}
static void render_blackbox(const NgGame *g,NgCanvas *c){
 unsigned n=g->rows;int s=144/(int)(n+2),ox=20+s,oy=31+s;char label[40];
 for(unsigned i=0;i<n*n;i++){
  int x=ox+(int)(i%n)*s,y=oy+(int)(i/n)*s;
  ng_rect(c,x,y,s,s,g->board[i]==1?NG_BLUE:NG_WHITE);ng_border(c,x,y,s,s,!g->phase&&i==g->cursor?NG_MAGENTA:NG_LINE,!g->phase&&i==g->cursor?2:1);
  if(g->board[i]==2){ng_line(c,x+3,y+3,x+s-4,y+s-4,NG_MUTED);ng_line(c,x+s-4,y+3,x+3,y+s-4,NG_MUTED);}
 }
 for(unsigned p=0;p<4*n;p++){
  int x,y;port_position(n,p,&x,&y);int result=g->data[8+p];
  if(result==1)strcpy(label,"H");else if(result==2)strcpy(label,"R");else snprintf(label,sizeof(label),"%u",result>=3?(unsigned)result-2:p+1);
  if(g->phase&&p==(unsigned)g->data[1])ng_rect(c,x,y,s,s,NG_PALE);
  ng_small(c,x+(s-ng_small_width(label))/2,y+(s-7)/2,label,result?NG_BLUE:NG_MUTED);
  if(g->phase&&p==(unsigned)g->data[1])ng_border(c,x,y,s,s,NG_MAGENTA,1);
 }
 ng_text(c,187,33,g->phase?"RAY PROBES":"ATOM MARKS",NG_BLUE,1);
 snprintf(label,sizeof(label),"Atoms %u / %d",marked_atoms(g),(int)g->data[0]);ng_text(c,187,54,label,NG_INK,1);
 snprintf(label,sizeof(label),"Probes %d  Penalty %u",(int)g->data[2],5u*((unsigned)g->data[3]-(g->status==NG_WON)));ng_small(c,187,73,label,NG_MUTED);
 snprintf(label,sizeof(label),"Port %u",(unsigned)g->data[1]+1);ng_text(c,187,91,label,NG_INK,1);
 int result=g->data[8+(unsigned)g->data[1]];
 if(result==1)strcpy(label,"HIT");else if(result==2)strcpy(label,"REFLECT");else if(result>=3)snprintf(label,sizeof(label),"EXIT AT %u",(unsigned)result-2);else strcpy(label,"UNTESTED");
 ng_text(c,187,109,label,NG_BLUE,1);
 ng_small(c,187,130,"F4: cells / rays",NG_MUTED);ng_small(c,187,143,g->phase?"Arrows: port  EXE: fire":"1 atom  2 X  0/DEL clear",NG_MUTED);ng_small(c,187,156,"F6: CHECK layout",NG_MUTED);ng_small(c,187,169,"H hit / R reflected / # exit",NG_MUTED);
}

static const GcCryptPack *crypt_pack(const NgGame *g){return &gc_crypt_pack[g->puzzle_id];}
static bool crypt_words_complete(const GcCryptPack *p,const int16_t *digits){
 unsigned used=0;for(unsigned i=0;i<p->letters;i++){if(digits[i]<0||digits[i]>9||(used&(1u<<digits[i])))return false;used|=1u<<digits[i];}
 unsigned total=0;
 for(unsigned w=0;w<=p->count;w++){
  const char *word=p->words[w];unsigned value=0;
  if(word[1]&&digits[(unsigned)(word[0]-'A')]==0)return false;
  for(unsigned j=0;word[j];j++)value=value*10+(unsigned)digits[(unsigned)(word[j]-'A')];
  if(w<p->count)total+=value;else return total==value;
 }
 return false;
}
bool gc_cryptarithm_complete(const NgGame *g){
 return g&&g->puzzle_id<120&&crypt_words_complete(crypt_pack(g),g->board);
}
static void init_cryptarithm(NgGame *g){
 g->puzzle_id=g->difficulty*30+ng_bank_pick(g,30);const GcCryptPack *p=crypt_pack(g);
 g->rows=2;g->cols=(uint8_t)((p->letters+1)/2);g->data[0]=p->count;g->data[1]=p->letters;
 for(unsigned i=0;i<p->letters;i++)g->board[i]=-1;
 ng_message(g,"Assign a different digit to each letter; F6 checks.");
}
static bool action_cryptarithm(NgGame *g,int key){
 unsigned count=crypt_pack(g)->letters;
 if(key==NGK_UP||key==NGK_DOWN||key==NGK_LEFT||key==NGK_RIGHT){
  int delta=key==NGK_LEFT?-1:key==NGK_RIGHT?1:key==NGK_UP?-(int)g->cols:g->cols;
  g->cursor=(uint8_t)(((int)g->cursor+delta+(int)count)%(int)count);return true;
 }
 if(auxiliary_key(key)){
  for(unsigned k=1;k<=count;k++){unsigned i=(g->cursor+k)%count;if(g->board[i]<0){g->cursor=(uint8_t)i;break;}}
  ng_message(g,"NEXT selects the next unassigned letter.");return true;
 }
 if(g->status!=NG_PLAYING||g->moves==UINT32_MAX)return false;
 if(check_key(key)){
  if(g->data[3]>=100000)return false;
  ++g->moves;++g->data[3];g->score=(unsigned)g->data[3];
  if(gc_cryptarithm_complete(g)){g->status=NG_WON;ng_message(g,"Distinct digits, nonzero leading letters, exact sum!");}
  else ng_message(g,"Fill all letters; distinct digits, no leading zero, exact sum.");
  return true;
 }
 int value=key>='0'&&key<='9'?key-'0':key==NGK_DEL?-1:-2;
 if(value==-2)return false;
 if(g->board[g->cursor]!=value){g->board[g->cursor]=(int16_t)value;++g->moves;}
 return true;
}
static bool valid_cryptarithm(const NgGame *g){
 if(g->id!=34||g->difficulty>3||g->mode||g->pack_revision!=2||g->puzzle_id<g->difficulty*30u||g->puzzle_id>=g->difficulty*30u+30||g->status>NG_WON||g->phase||g->history_count||g->scroll||g->notes_mode||g->cpu_pending||g->turn||!empty_text(g))return false;
 const GcCryptPack *p=crypt_pack(g);
 if(g->rows!=2||g->cols!=(p->letters+1)/2||g->cursor>=p->letters||g->data[0]!=p->count||g->data[1]!=p->letters||g->data[3]<0||g->data[3]>100000||g->score!=(unsigned)g->data[3]||g->moves<g->score)return false;
 for(unsigned i=0;i<NG_CELLS;i++)if(g->fixed[i]||g->notes[i]||(i<p->letters?(g->board[i]<-1||g->board[i]>9):g->board[i]!=0))return false;
 for(unsigned i=0;i<NG_DATA;i++)if(i!=0&&i!=1&&i!=3&&g->data[i])return false;
 return g->status!=NG_WON||(g->data[3]&&gc_cryptarithm_complete(g));
}
static void render_cryptarithm(const NgGame *g,NgCanvas *c){
 const GcCryptPack *p=crypt_pack(g);char line[40],numbers[8];
 ng_text(c,15,30,"LETTER ADDITION",NG_BLUE,1);
 for(unsigned w=0;w<=p->count;w++){
  int y=50+(int)w*22;snprintf(line,sizeof(line),"%c %6s",w==p->count?'=':w?'+':' ',p->words[w]);ng_expression(c,17,y,line,NG_INK,1);
  unsigned j;for(j=0;p->words[w][j];j++){int value=g->board[(unsigned)(p->words[w][j]-'A')];numbers[j]=value<0?'_':(char)('0'+value);}numbers[j]=0;
  ng_text(c,115,y,numbers,NG_BLUE,1);
 }
 ng_line(c,17,89,174,89,NG_LINE);
 ng_small(c,205,34,"Letters use DIFFERENT digits",NG_MUTED);ng_small(c,205,48,"First letter cannot be zero",NG_MUTED);ng_small(c,211,70,"Arrows: choose a letter",NG_MUTED);ng_small(c,211,84,"0..9: set   DEL: clear",NG_MUTED);ng_small(c,211,98,"NEXT: next empty letter",NG_MUTED);
 unsigned columns=g->cols;
 for(unsigned i=0;i<p->letters;i++){
  int x=13+(int)(i%columns)*74,y=119+(int)(i/columns)*31;
  snprintf(line,sizeof(line),"%c = %c",(int)('A'+i),g->board[i]<0?'_':(int)('0'+g->board[i]));
  ng_card(c,x,y,68,27,"",i==g->cursor,false);ng_expression(c,x+9,y+8,line,NG_INK,1);
 }
}

const NgModule ng_guesscalc_extra[2]={
 {33,"BLACK BOX","BLACK BOX","Find hidden atoms by firing beams from edges.\nE/N/H/MASTER: 5/6/7/8 square; 3/4/5/6 atoms.\nA beam hitting an atom is absorbed (H).\nAn atom diagonally ahead bends it away.\nTwo diagonally ahead reverse the beam.\nA side atom at entry reflects immediately (R).\nDirect hits take precedence over diagonals.\nExit numbers pair the entry and exit ports.\nAny layout with the correct atom count and\nALL the same ray outcomes wins; equivalent\nlayouts are accepted, not just the hidden one.\nCELLS: arrows move; EXE cycles blank/atom/X.\n1 atom, 2 excluded, 0/DEL clear. F4: RAYS.\nRAYS: arrows select port; EXE fires. F4: CELLS.\nF6 CHECK costs 5 penalty points if wrong.\nScore = new probes + penalties; lower is best.\nHINT reveals one atom and marks assisted.",NGF_UNDO|NGF_HINT,"RAY/CELL","CHECK",1,standard,init_blackbox,action_blackbox,NULL,valid_blackbox,render_blackbox},
 {34,"CRYPTARITHM","CRYPTARITHM","Replace letters with decimal digits so the\nvertical addition is true. Each letter keeps\none digit; different letters use different\ndigits. Leading letters cannot be zero.\nE/N/H/MASTER: two 2/3/4/5-digit addends.\nLonger carries and more letters are involved;\nthis is a structural level, not a human rating.\n30 original, unique-solution puzzles per level.\nArrows select a letter. Digits set it.\nDEL clears it. NEXT selects the next blank.\nEXE or F6 checks all public arithmetic rules.\nAny valid assignment is accepted. UNDO works\non digit edits and checks. No stored answer\nis consulted by the native checker.",NGF_UNDO,"NEXT","CHECK",1,standard,init_cryptarithm,action_cryptarithm,NULL,valid_cryptarithm,render_cryptarithm}
};
