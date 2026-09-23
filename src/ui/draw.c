#include "ui.h"
#include <stdio.h>
#include <string.h>
#if !defined(FXCG50) && !defined(TARGET_FXCG50)
#include <assert.h>
#endif
#include "font_data.h"
void ng_rect(NgCanvas *c,int x,int y,int w,int h,int color)
{
 if(x<0){w+=x;x=0;}if(y<0){h+=y;y=0;}
 if(x+w>396)w=396-x;
 if(y+h>224)h=224-y;
 if(w>0 && h>0)c->rect(c->context,x,y,w,h,(uint16_t)color);
}
void ng_border(NgCanvas *c,int x,int y,int w,int h,int color,int thick)
{
 ng_rect(c,x,y,w,thick,color);ng_rect(c,x,y+h-thick,w,thick,color);
 ng_rect(c,x,y,thick,h,color);ng_rect(c,x+w-thick,y,thick,h,color);
}
void ng_line(NgCanvas *c,int x1,int y1,int x2,int y2,int color)
{
 int dx=x2-x1,dy=y2-y1,sx=dx<0?-1:1,sy=dy<0?-1:1;
 if(dx<0)dx=-dx;
 if(dy<0)dy=-dy;
 int err=dx-dy;
 for(;;){ng_rect(c,x1,y1,1,1,color);if(x1==x2 && y1==y2)break;
  int e=2*err;if(e>-dy){err-=dy;x1+=sx;}if(e<dx){err+=dx;y1+=sy;}}
}
static unsigned glyph(unsigned char c){return c>=32 && c<=126?c-32:'?'-32;}
int ng_text_width(const char *s,int scale)
{int w=0;for(;*s;s++)w+=normal_width[glyph((unsigned char)*s)]+1;return w?(w-1)*scale:0;}
int ng_small_width(const char *s)
{int w=0;for(;*s;s++)w+=small_width[glyph((unsigned char)*s)]+1;return w?w-1:0;}
static void letters(NgCanvas *c,int x,int y,const char *s,int color,int scale,bool small)
{
 int advance=0,height=small?7:11;
#if !defined(FXCG50) && !defined(TARGET_FXCG50)
 /* Audit the text before primitive clipping could conceal a layout defect. */
 int width=small?ng_small_width(s):ng_text_width(s,scale);
 if(*s && (x<0 || y<0 || x+width>396 || y+height*scale>224)) {
  fprintf(stderr,"Text outside framebuffer: %d,%d %dx%d: %s\n",x,y,width,height*scale,s);
  assert(!"text outside framebuffer");
 }
#endif
 for(;*s;s++) {
  unsigned k=glyph((unsigned char)*s);
  unsigned width=small?small_width[k]:normal_width[k];
  const unsigned char *rows=small?small_rows[k]:normal_rows[k];
  for(int r=0;r<height;r++)for(unsigned col=0;col<width;col++)if(rows[r]&(1u<<col))
   ng_rect(c,x+(advance+(int)col)*scale,y+r*scale,scale,scale,color);
  advance+=(int)width+1;
 }
}
void ng_text(NgCanvas *c,int x,int y,const char *s,int color,int scale)
{letters(c,x,y,s,color,scale,false);}
void ng_small(NgCanvas *c,int x,int y,const char *s,int color)
{letters(c,x,y,s,color,1,true);}
int ng_operator_color(int symbol,int fallback)
{
 switch(symbol){case '+':return NG_RGB(24,0,23);case '-':return NG_RGB(25,12,0);
 case '*':case 'x':return NG_RGB(0,24,4);case '/':return NG_RGB(0,18,23);default:return fallback;}
}
static int expression_color(const char *s,size_t i,int color)
{
 bool exponent=(s[i]=='-' || s[i]=='+') && i>=2 &&
  (s[i-1]=='e' || s[i-1]=='E') && s[i-2]>='0' && s[i-2]<='9' && s[i+1]>='0' && s[i+1]<='9';
 bool previous_letter=i && ((s[i-1]>='A' && s[i-1]<='Z') || (s[i-1]>='a' && s[i-1]<='z'));
 bool next_letter=(s[i+1]>='A' && s[i+1]<='Z') || (s[i+1]>='a' && s[i+1]<='z');
 size_t before=i,after=i+1;while(before && s[before-1]==' ')before--;while(s[after]==' ')after++;
 bool prose=before && ((s[before-1]>='A' && s[before-1]<='Z') || (s[before-1]>='a' && s[before-1]<='z')) && ((s[after]>='A' && s[after]<='Z') || (s[after]>='a' && s[after]<='z'));
 bool word=((s[i]=='-' || s[i]=='/') && prose) || (s[i]=='x' && (previous_letter || next_letter)) || (s[i]=='-' && previous_letter && next_letter);
 return exponent || word?color:ng_operator_color(s[i],color);
}
static void expression_span(NgCanvas *c,int x,int y,const char *s,int color,int scale,bool small,const char *original,size_t offset,size_t count)
{
 int advance=0;
 for(size_t i=0;s[i];i++){
  char ch[2]={s[i],0};int ink=i<count?expression_color(original,offset+i,color):color;
  letters(c,x+advance,y,ch,ink,scale,small);
  advance+=((small?small_width[glyph((unsigned char)s[i])]:normal_width[glyph((unsigned char)s[i])])+1)*scale;
 }
}
static void expression(NgCanvas *c,int x,int y,const char *s,int color,int scale,bool small)
{expression_span(c,x,y,s,color,scale,small,s,0,strlen(s));}
void ng_expression(NgCanvas *c,int x,int y,const char *s,int color,int scale)
{expression(c,x,y,s,color,scale,false);}
void ng_small_expression(NgCanvas *c,int x,int y,const char *s,int color)
{expression(c,x,y,s,color,1,true);}
static void shorten(char *out,unsigned capacity,const char *s,int width,bool small)
{
 snprintf(out,capacity,"%s",s);
 unsigned n=(unsigned)strlen(out);
 while(n && (small?ng_small_width(out):ng_text_width(out,1))>width){out[--n]=0;}
 if(s[n]) {
  while(n && ((small?ng_small_width(out):ng_text_width(out,1))+(small?ng_small_width("..."):ng_text_width("...",1))+1>width || n+3>=capacity))out[--n]=0;
  if(width>=(small?ng_small_width("..."):ng_text_width("...",1)) && n+3<capacity)strcat(out,"...");
 }
}
void ng_small_fit(NgCanvas *c,int x,int y,int width,const char *s,int color)
{char text[256];shorten(text,sizeof text,s,width,true);ng_small(c,x,y,text,color);}
void ng_text_fit(NgCanvas *c,int x,int y,int width,const char *s,int color,int scale)
{
 while(scale>1 && ng_text_width(s,scale)>width)scale--;
 if(ng_text_width(s,scale)<=width)ng_text(c,x,y,s,color,scale);
 else ng_small_fit(c,x,y,width,s,color);
}
void ng_center(NgCanvas *c,int x,int y,int w,const char *s,int color,int scale)
{
 while(scale>1 && ng_text_width(s,scale)>w)scale--;
 if(ng_text_width(s,scale)<=w)ng_text(c,x+(w-ng_text_width(s,scale))/2,y,s,color,scale);
 else {char text[256];shorten(text,sizeof text,s,w,true);ng_small(c,x+(w-ng_small_width(text))/2,y,text,color);}
}
bool ng_text_line(char *out,unsigned capacity,const char **text,int width,bool small)
{
 const char *start=*text,*p=start;unsigned n=0,space=0;
 if(!*p || capacity<2){if(capacity)out[0]=0;return false;}
 while(*p && *p!='\n' && n+1<capacity) {
  out[n]=*p;out[n+1]=0;
  if(n && (small?ng_small_width(out):ng_text_width(out,1))>width)break;
  if(*p==' ')space=n;
  n++;p++;
 }
 if(*p && *p!='\n' && space){n=space;p=start+space;}
 while(n && out[n-1]==' ')n--;
 out[n]=0;while(*p==' ')p++;if(*p=='\n')p++;
 *text=p;return true;
}
void ng_wrap(NgCanvas *c,int x,int y,int width,int spacing,unsigned lines,const char *text,int color,bool small)
{
 char line[256];
 if(!small) {
  const char *remaining=text;
  for(unsigned i=0;i<lines && ng_text_line(line,sizeof line,&remaining,width,false);i++){}
  if(*remaining)small=true;
 }
 for(unsigned i=0;i<lines && ng_text_line(line,sizeof line,&text,width,small);i++) {
  if(i+1==lines && *text){size_t n=strlen(line);if(n+3<sizeof line)strcat(line,"...");}
  if(small)ng_small_fit(c,x,y+(int)i*spacing,width,line,color);
  else ng_text_fit(c,x,y+(int)i*spacing,width,line,color,1);
 }
}
void ng_wrap_expression(NgCanvas *c,int x,int y,int width,int spacing,unsigned lines,const char *text,int color,bool small)
{
 char line[256],fitted[256];const char *original=text;
 if(!small){const char *rest=text;for(unsigned i=0;i<lines && ng_text_line(line,sizeof line,&rest,width,false);i++){}if(*rest)small=true;}
 for(unsigned i=0;i<lines;i++){
  const char *start=text;if(!ng_text_line(line,sizeof line,&text,width,small))break;
  if(i+1==lines && *text && strlen(line)+3<sizeof line)strcat(line,"...");
  shorten(fitted,sizeof fitted,line,width,small);
  size_t count=0;while(fitted[count] && start[count] && fitted[count]==start[count])count++;
  expression_span(c,x,y+(int)i*spacing,fitted,color,1,small,original,(size_t)(start-original),count);
 }
}
void ng_number(NgCanvas *c,int x,int y,int value,int color,int scale)
{char b[20];snprintf(b,sizeof(b),"%d",value);ng_text(c,x,y,b,color,scale);}
void ng_input(NgCanvas *c,int x,int y,int w,const char *value)
{
 ng_rect(c,x,y,w,21,NG_WHITE);ng_border(c,x,y,w,21,NG_BLUE,1);
 const char *start=value;while(*start && ng_text_width(start,1)>w-14)start++;
 ng_text(c,x+6,y+5,start,NG_BLUE,1);
 int cursor=x+6+ng_text_width(start,1)+2;ng_rect(c,cursor,y+4,1,12,NG_BLUE);
}
void ng_input_expression(NgCanvas *c,int x,int y,int w,const char *value)
{
 ng_rect(c,x,y,w,21,NG_WHITE);ng_border(c,x,y,w,21,NG_BLUE,1);
 const char *start=value;while(*start && ng_text_width(start,1)>w-14)start++;
 expression_span(c,x+6,y+5,start,NG_BLUE,1,false,value,(size_t)(start-value),strlen(start));
 int cursor=x+6+ng_text_width(start,1)+2;ng_rect(c,cursor,y+4,1,12,NG_BLUE);
}
void ng_card(NgCanvas *c,int x,int y,int w,int h,const char *label,bool selected,bool used)
{
 ng_rect(c,x,y,w,h,used?NG_LINE:NG_WHITE);
 ng_border(c,x,y,w,h,selected?NG_BLUE:NG_LINE,selected?2:1);
 int scale=ng_text_width(label,2)<w-8 && h>=29?2:1;
 ng_center(c,x,y+(h-10*scale)/2,w,label,used?NG_MUTED:NG_INK,scale);
}
NgGridLayout ng_grid_layout(unsigned rows,unsigned cols,bool outside)
{
 int width=outside?210:228,height=outside?138:156;
 int s=rows?(height/(int)rows):16;
 if(cols && s>width/(int)cols)s=width/(int)cols;
 if(s>36)s=36;
 return (NgGridLayout){(outside?23:10)+(width-(int)cols*s)/2,
  (outside?42:29)+(height-(int)rows*s)/2,s};
}
void ng_grid_cell(NgCanvas *c,NgGridLayout l,unsigned cols,unsigned index,const char *label,bool fixed,bool selected,int fill)
{
 int x=l.x+(int)(index%cols)*l.size,y=l.y+(int)(index/cols)*l.size;
 ng_rect(c,x,y,l.size,l.size,fill);ng_border(c,x,y,l.size,l.size,NG_LINE,1);
 int color=fixed?NG_INK:NG_BLUE;
 int scale=l.size>=30 && ng_text_width(label,2)<l.size-5?2:1;
 ng_center(c,x,y+(l.size-10*scale)/2,l.size,label,color,scale);
 if(selected)ng_border(c,x,y,l.size,l.size,NG_BLUE,2);
 if(fixed)ng_rect(c,x+2,y+2,2,2,NG_MUTED);
}
