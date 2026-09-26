// Host-only exact subset DP. Real parser reduced-value bounds, no heuristic cut.
// Count quotient: equal-valued leaf occurrence renaming, +/* commutation,
// redundant parentheses, and adjacent double negation. Association is retained.
// Unary minus of zero is a distinct canonical expression.
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

struct Rational {
 int64_t n=0,d=1;
 bool operator<(const Rational &x)const{return std::tie(n,d)<std::tie(x.n,x.d);}
};
static bool positive_only=false;
static bool rational(int64_t n,int64_t d,Rational &out){
 if(!d)return false;
 if(d<0){n=-n;d=-d;}
 int64_t divisor=std::gcd(n,d);n/=divisor;d/=divisor;
 if(n < -1000000000LL || n > 1000000000LL || d > 1000000000LL || (positive_only&&(n<=0||d!=1)))return false;
 out={n,d};return true;
}
struct Feature {
 int fractional=0,division=0,noncomm=0,depth=0;
 int64_t denominator=1;
 int subtraction=0,negative=0,unary=0;
 std::string expression;
 auto first()const{return std::tie(fractional,division,noncomm);}
 auto order()const{return std::tie(fractional,division,noncomm,depth,denominator,subtraction,negative,expression);}
 auto grade_first()const{return std::make_tuple(fractional,division,noncomm+unary,negative);}
 auto grade_order()const{return std::make_tuple(fractional,division,noncomm+unary,negative,depth,denominator,expression);}
};
struct Entry {uint64_t count=0;std::vector<Feature> best,graded;};
using Table=std::map<Rational,Entry>;
using Cards=std::vector<int>;
static std::map<Cards,Table> cache;
static bool allow_unary=true;

static void keep(std::vector<Feature> &best,Feature candidate,bool grade=false){
 auto prefix=[grade](const Feature &f){return grade?f.grade_first():std::make_tuple(f.fractional,f.division,f.noncomm,0);};
 if(!best.empty()){
  if(prefix(best.front())<prefix(candidate))return;
  if(prefix(candidate)<prefix(best.front()))best.clear();
 }
 for(auto it=best.begin();it!=best.end();){
  if(it->depth<=candidate.depth && it->denominator<=candidate.denominator){
   if(it->depth!=candidate.depth || it->denominator!=candidate.denominator || (grade?it->grade_order()<=candidate.grade_order():it->order()<=candidate.order()))return;
  }
  if(candidate.depth<=it->depth && candidate.denominator<=it->denominator)it=best.erase(it);
  else ++it;
 }
 best.push_back(std::move(candidate));
}
static Feature combine(const Feature &a,const Feature &b,char op,const Rational &value){
 Feature f;
 f.fractional=a.fractional+b.fractional+(value.d>1);
 f.division=a.division+b.division+(op=='/');
 f.subtraction=a.subtraction+b.subtraction+(op=='-');f.noncomm=f.division+f.subtraction;
 f.depth=1+std::max(a.depth,b.depth);f.denominator=std::max({a.denominator,b.denominator,value.d});
 f.negative=a.negative+b.negative+(value.n<0);
 f.unary=a.unary+b.unary;
 std::string left=a.expression,right=b.expression;
 if((op=='+'||op=='*')&&right<left)std::swap(left,right);
 f.expression="("+left+op+right+")";return f;
}
static void add(Table &base,const Rational &a,const Entry &ea,const Rational &b,const Entry &eb,
                char op,uint64_t count,bool filter,int target){
 Rational value;bool ok;
 if(op=='+')ok=rational(a.n*b.d+b.n*a.d,a.d*b.d,value);
 else if(op=='-')ok=rational(a.n*b.d-b.n*a.d,a.d*b.d,value);
 else if(op=='*')ok=rational(a.n*b.n,a.d*b.d,value);
 else ok=rational(a.n*b.d,a.d*b.n,value);
 if(!ok || (filter && (value.d!=1 || (value.n!=target && (!allow_unary || value.n!=-target)))))return;
 Entry &dest=base[value];dest.count+=count;
 for(const auto &fa:ea.best)for(const auto &fb:eb.best)keep(dest.best,combine(fa,fb,op,value));
 for(const auto &fa:ea.graded)for(const auto &fb:eb.graded)keep(dest.graded,combine(fa,fb,op,value),true);
}
static Table solve(const Cards &cards,bool filter=false,int target=0);
static const Table &subtable(const Cards &cards){
 auto found=cache.find(cards);if(found!=cache.end())return found->second;
 return cache.emplace(cards,solve(cards)).first->second;
}
static Table solve(const Cards &cards,bool filter,int target){
 Table base;
 if(cards.size()==1){Feature f;f.expression=std::to_string(cards[0]);base[{cards[0],1}]={1,{f},{f}};}
 else {
  std::set<std::pair<Cards,Cards>> splits;
  for(unsigned mask=1;mask<(1u<<cards.size())-1;mask++){
   Cards a,b;for(unsigned i=0;i<cards.size();i++)((mask>>i&1)?a:b).push_back(cards[i]);
   if(b<a)std::swap(a,b);splits.emplace(a,b);
  }
  for(const auto &partition:splits){
   const auto &a=subtable(partition.first),&b=subtable(partition.second);
   bool same=partition.first==partition.second;
   for(auto ia=a.begin();ia!=a.end();++ia)for(auto ib=same?b.lower_bound(ia->first):b.begin();ib!=b.end();++ib){
    bool diagonal=same && !(ia->first<ib->first) && !(ib->first<ia->first);
    uint64_t product=ia->second.count*ib->second.count;
    uint64_t unordered=diagonal?ia->second.count*(ia->second.count+1)/2:product;
    add(base,ia->first,ia->second,ib->first,ib->second,'+',unordered,filter,target);
    add(base,ia->first,ia->second,ib->first,ib->second,'*',unordered,filter,target);
    add(base,ia->first,ia->second,ib->first,ib->second,'-',product,filter,target);
    add(base,ia->first,ia->second,ib->first,ib->second,'/',product,filter,target);
    if(!diagonal){
     add(base,ib->first,ib->second,ia->first,ia->second,'-',product,filter,target);
     add(base,ib->first,ib->second,ia->first,ia->second,'/',product,filter,target);
    }
   }
  }
 }
 if(!allow_unary)return base;
 Table result=base;
 for(const auto &item:base){
  Rational value={-item.first.n,item.first.d};Entry &dest=result[value];dest.count+=item.second.count;
  for(auto f:item.second.best){
   f.fractional+=value.d>1;f.negative+=value.n<0;f.depth++;f.unary++;
   f.expression="-"+f.expression;keep(dest.best,std::move(f));
  }
  for(auto f:item.second.graded){
   f.fractional+=value.d>1;f.negative+=value.n<0;f.depth++;f.unary++;
   f.expression="-"+f.expression;keep(dest.graded,std::move(f),true);
  }
 }
 return result;
}
static void emit(const Rational &value,const Entry &entry){
 const Feature &f=*std::min_element(entry.best.begin(),entry.best.end(),[](const Feature&a,const Feature&b){return a.order()<b.order();});
 std::cout<<"{\"num\":"<<value.n<<",\"den\":"<<value.d<<",\"canonical_count\":"<<entry.count
 <<",\"minimal_tuple\":["<<f.fractional<<','<<f.division<<','<<f.noncomm<<','<<f.depth<<','<<f.denominator
 <<"],\"fractional_steps\":"<<f.fractional<<",\"division_steps\":"<<f.division
 <<",\"subtraction_steps\":"<<f.subtraction<<",\"tree_depth\":"<<f.depth
 <<",\"max_denominator\":"<<f.denominator<<",\"negative_intermediate_steps\":"<<f.negative
 <<",\"unary_steps\":"<<f.unary<<",\"easiest_expression\":\""<<f.expression<<"\"";
 const Feature &g=*std::min_element(entry.graded.begin(),entry.graded.end(),[](const Feature&a,const Feature&b){return a.grade_order()<b.grade_order();});
 int score=g.fractional*65536+g.division*8192+(g.noncomm+g.unary)*256+g.negative*16+g.depth;
 std::cout<<",\"graded\":{\"score\":"<<score<<",\"fractional_steps\":"<<g.fractional<<",\"division_steps\":"<<g.division
 <<",\"subtraction_steps\":"<<g.subtraction<<",\"noncommutative_steps\":"<<g.noncomm<<",\"unary_steps\":"<<g.unary
 <<",\"negative_intermediate_steps\":"<<g.negative<<",\"tree_depth\":"<<g.depth<<",\"max_denominator\":"<<g.denominator
 <<",\"expression\":\""<<g.expression<<"\"}}";
}
int main(int argc,char **argv){
 bool all=false;
 for(int i=1;i<argc;i++){std::string flag=argv[i];if(flag=="--all")all=true;else if(flag=="--no-unary")allow_unary=false;else if(flag=="--integer"){positive_only=true;allow_unary=false;}else return 2;}
 std::string line;
 while(std::getline(std::cin,line)){
  std::istringstream input(line);unsigned n;int target;if(!(input>>n>>target)||n<1||n>6)return 2;
  Cards cards(n);for(int &v:cards)if(!(input>>v)||v<1||v>1000000)return 2;
  std::sort(cards.begin(),cards.end());cache.clear();Table result=solve(cards,!all,target);
  std::cout<<'[';bool first=true;
  for(const auto &item:result)if(all || (item.first.n==target&&item.first.d==1)){if(!first)std::cout<<',';first=false;emit(item.first,item.second);}
  std::cout<<"]\n"<<std::flush;
 }
 return 0;
}
