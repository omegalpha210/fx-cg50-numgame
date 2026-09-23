#include "guesscalc_math.h"
#include <stddef.h>
#include <string.h>

#define GC_BOUND INT64_C(1000000000)
typedef struct {const char *s;unsigned pos,depth;bool ok,integer;GcExpression *out;} Parser;
static int64_t magnitude(int64_t n) { return n<0?-n:n; }
static int64_t gcd(int64_t a,int64_t b) {a=magnitude(a);while(b){int64_t t=a%b;a=b;b=t;}return a?a:1;}
static GcRational normalize(Parser *p,int64_t n,int64_t d) {
 GcRational r={0,1};if(!d){p->ok=false;return r;}if(d<0){n=-n;d=-d;}
 int64_t v=gcd(n,d);n/=v;d/=v;
 if(magnitude(n)>GC_BOUND||d>GC_BOUND||(p->integer&&(n<=0||d!=1))){p->ok=false;return r;}
 r.num=n;r.den=d;return r;
}
/* Callers provide a 97-byte input field; inspect no byte beyond that field.
 * Requiring the terminator up front also bounds the equation RHS and exponent
 * scans, including malformed runs of zeros that never grow numerically. */
static bool bounded_text(const char *text){
 if(!text)return false;
 for(unsigned i=0;i<=96;i++)if(!text[i])return i!=0;
 return false;
}
static GcRational expression(Parser *p);
static GcRational atom(Parser *p) {
 GcRational r={0,1};if(!p->ok)return r;
 if(++p->depth>12){p->ok=false;--p->depth;return r;}
 char c=p->s[p->pos];
 if(c=='('){++p->pos;r=expression(p);if(p->s[p->pos]!=')')p->ok=false;else ++p->pos;}
 else if(c=='-'){++p->pos;if(++p->out->operations>31)p->ok=false;r=atom(p);r=normalize(p,-r.num,r.den);}
 else if(c>='0'&&c<='9'){
  int64_t n=0;unsigned start=p->pos;
  while(p->s[p->pos]>='0'&&p->s[p->pos]<='9'){
   n=n*10+(p->s[p->pos++]-'0');if(n>1000000){p->ok=false;break;}
  }
  if(p->pos-start>1&&p->s[start]=='0')p->ok=false;
  if(p->out->count>=GC_MAX_LITERALS)p->ok=false;
  else p->out->literals[p->out->count++]=(int)n;
  r=normalize(p,n,1);
 }else p->ok=false;
 --p->depth;return r;
}
static GcRational combine(Parser *p,GcRational a,GcRational b,char op){
 if(!p->ok)return a;
 if(++p->out->operations>31){p->ok=false;return a;}
 /* Each product is <= 1e18; the sum/difference is <= 2e18. */
 if(op=='+')return normalize(p,a.num*b.den+b.num*a.den,a.den*b.den);
 if(op=='-')return normalize(p,a.num*b.den-b.num*a.den,a.den*b.den);
 if(op=='*')return normalize(p,a.num*b.num,a.den*b.den);
 return normalize(p,a.num*b.den,a.den*b.num);
}
static GcRational term(Parser *p){GcRational a=atom(p);while(p->ok&&(p->s[p->pos]=='*'||p->s[p->pos]=='/')){char op=p->s[p->pos++];GcRational b=atom(p);a=combine(p,a,b,op);}return a;}
static GcRational expression(Parser *p){GcRational a=term(p);while(p->ok&&(p->s[p->pos]=='+'||p->s[p->pos]=='-')){char op=p->s[p->pos++];GcRational b=term(p);a=combine(p,a,b,op);}return a;}
bool gc_expression(const char *text,bool integer,GcExpression *out){
 if(!out||!bounded_text(text))return false;
 memset(out,0,sizeof(*out));Parser p={text,0,0,true,integer,out};out->value=expression(&p);
 return p.ok&&text[p.pos]=='\0';
}
bool gc_cards(const GcExpression *e,const int16_t *cards,unsigned count,bool all){
 if(!e||!cards||count>16||e->count>16||(all&&e->count!=count))return false;
 unsigned used=0;for(unsigned i=0;i<e->count;i++){
  bool found=false;for(unsigned j=0;j<count;j++)if(!(used&(1u<<j))&&cards[j]==e->literals[i]){used|=1u<<j;found=true;break;}
  if(!found)return false;
 }return true;
}
bool gc_equation(const char *text){
 if(!bounded_text(text))return false;
 char lhs[97];size_t n=0;while(text[n]&&text[n]!='='&&n<96)++n;
 if(!n||n>=96||text[n]!='=')return false;
 for(size_t i=0;i<n;i++)if(!((text[i]>='0'&&text[i]<='9')||strchr("+-*/",text[i])))return false;
 if(strchr("+-*/",text[0]))return false;
 for(size_t i=1;i<n;i++)if(strchr("+-*/",text[i])&&strchr("+-*/",text[i-1]))return false;
 memcpy(lhs,text,n);lhs[n]=0;const char *rhs=text+n+1;
 int sign=1;if(*rhs=='-'){sign=-1;++rhs;}
 if(!*rhs||(rhs[0]=='0'&&rhs[1]))return false;
 int64_t value=0;unsigned k=0;for(;rhs[k];k++){if(k>=7||rhs[k]<'0'||rhs[k]>'9')return false;value=value*10+rhs[k]-'0';}
 GcExpression e;return gc_expression(lhs,false,&e)&&e.value.den==1&&e.value.num==sign*value;
}
void gc_feedback(const char *secret,const char *guess,unsigned n,uint8_t *feedback){
 unsigned counts[128]={0};for(unsigned i=0;i<n;i++){feedback[i]=secret[i]==guess[i]?2:0;if(!feedback[i]&&(unsigned char)secret[i]<128)++counts[(unsigned char)secret[i]];}
 for(unsigned i=0;i<n;i++)if(!feedback[i]&&(unsigned char)guess[i]<128&&counts[(unsigned char)guess[i]]){feedback[i]=1;--counts[(unsigned char)guess[i]];}
}
void gc_baseball(const char *s,const char *g,unsigned n,unsigned *strikes,unsigned *balls){uint8_t f[16]={0};*strikes=0;*balls=0;if(n>16)return;gc_feedback(s,g,n,f);for(unsigned i=0;i<n;i++){*strikes+=f[i]==2;*balls+=f[i]==1;}}
bool gc_is_prime(unsigned n){if(n<2)return false;for(unsigned p=2;p<=n/p;p++)if(n%p==0)return false;return true;}
bool gc_factorization(const char *s,int target){
 if(target<2||!bounded_text(s))return false;
 unsigned pos=0,terms=0;uint64_t product=1;
 while(s[pos]){
  if(++terms>16||pos>96)return false;
  unsigned base=0,start=pos;
  while(s[pos]>='0'&&s[pos]<='9'){base=base*10+(unsigned)(s[pos++]-'0');if(base>1000000)return false;}
  if(pos==start||(pos-start>1&&s[start]=='0')||!gc_is_prime(base))return false;
  unsigned exp=1;if(s[pos]=='^'){++pos;start=pos;exp=0;while(s[pos]>='0'&&s[pos]<='9'){exp=exp*10+(unsigned)(s[pos++]-'0');if(exp>20)return false;}if(start==pos||!exp||(pos-start>1&&s[start]=='0'))return false;}
  for(unsigned k=0;k<exp;k++){product*=base;if(product>(uint64_t)target)return false;}
  if(!s[pos])break;
  if(s[pos++]!='*'||!s[pos])return false;
 }return product==(uint64_t)target;
}
