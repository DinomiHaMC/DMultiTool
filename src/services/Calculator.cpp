#include "Calculator.h"
#include "../ui/GridNavigation.h"
#include <cmath>
#include <cstdlib>
#include <cctype>
#include <cstdio>
namespace {
constexpr double Pi=3.14159265358979323846;
struct Parser {
  const std::string& text;bool degrees;double ans;size_t pos=0;int depth=0;std::string error;
  void spaces() { while(pos<text.size()&&std::isspace((unsigned char)text[pos]))pos++; }
  bool take(char c) { spaces();if(pos<text.size()&&text[pos]==c) { pos++;return true; }return false; }
  double fail(const char* why) { if(error.empty())error=why;return 0; }
  double expression() {
    double value=term();
    while(error.empty()) { if(take('+'))value+=term();else if(take('-'))value-=term();else break; }
    return value;
  }
  double term() {
    double value=unary();
    while(error.empty()) {
      if(take('*'))value*=unary();
      else if(take('/')) { double divisor=unary();if(divisor==0)return fail("Division by zero");value/=divisor; }
      else break;
    }
    return value;
  }
  double unary() {
    if(++depth>24) { --depth;return fail("Expression too deep"); }
    double value;
    if(take('+'))value=unary();else if(take('-'))value=-unary();else value=power();
    --depth;return value;
  }
  double power() {
    double value=primary();
    while(error.empty()) {
      if(take('%'))value/=100;
      else if(take('!')) {
        if(value<0||value>170||std::floor(value)!=value)return fail("Invalid factorial");
        value=std::tgamma(value+1);
      } else break;
    }
    if(error.empty()&&take('^'))value=std::pow(value,unary());
    return value;
  }
  double primary() {
    spaces();if(!error.empty())return 0;
    if(take('(')) { double value=expression();if(!take(')'))return fail("Missing )");return value; }
    if(pos>=text.size())return fail("Expected value");
    if(std::isalpha((unsigned char)text[pos])) {
      size_t start=pos;while(pos<text.size()&&std::isalpha((unsigned char)text[pos]))pos++;
      std::string name=text.substr(start,pos-start);
      if(name=="pi")return Pi;
      if(name=="e")return std::exp(1.0);
      if(name=="ans")return ans;
      if(!take('('))return fail("Unknown name");
      double value=expression(),second=0;bool hasSecond=take(',');if(hasSecond)second=expression();
      if(!take(')'))return fail("Missing )");
      if(name=="pow"&&hasSecond)return std::pow(value,second);
      if(hasSecond)return fail("Too many arguments");
      double angle=degrees?value*Pi/180:value;
      if(name=="sin")return std::sin(angle);
      if(name=="cos")return std::cos(angle);
      if(name=="tan")return std::tan(angle);
      if(name=="asin")return std::asin(value)*(degrees?180/Pi:1);
      if(name=="acos")return std::acos(value)*(degrees?180/Pi:1);
      if(name=="atan")return std::atan(value)*(degrees?180/Pi:1);
      if(name=="sqrt")return std::sqrt(value);
      if(name=="ln")return std::log(value);
      if(name=="log")return std::log10(value);
      if(name=="exp")return std::exp(value);
      if(name=="abs")return std::fabs(value);
      return fail("Unknown function");
    }
    char* end=nullptr;const char* start=text.c_str()+pos;double value=std::strtod(start,&end);
    if(end==start)return fail("Expected number");
    pos+=end-start;return value;
  }
};
}
bool Calculator::evaluate(const std::string& text,bool degrees,double ans,double& result,std::string& error) {
  if(text.empty()||text.size()>128) { error="Expression empty / too long";return false; }
  Parser parser{text,degrees,ans,0,0,{}};result=parser.expression();parser.spaces();
  if(parser.error.empty()&&parser.pos!=text.size())parser.error="Unexpected input";
  if(parser.error.empty()&&!std::isfinite(result))parser.error="Math domain / overflow";
  error=parser.error;return error.empty();
}
const char* CalculatorModel::label(int i)const {
  static const char* keys[]={"SCI","DEG","DEL","AC","=","7","8","9","/","(","4","5","6","*",")","1","2","3","-","%","0",".","ans","+","^","sin(","cos(","tan(","ln(","log(","asin(","acos(","atan(","sqrt(","abs(","pi","e","exp(",",","!"};
  if(i==0)return scientific?"BASIC":"SCI";
  if(i==1)return degrees?"DEG":"RAD";
  return i>=0&&i<count()?keys[i]:"";
}
void CalculatorModel::move(int dx,int dy) { selected=GridNavigation::move(selected,count(),5,dx,dy,true); }
void CalculatorModel::press() {
  if(selected==0) { scientific=!scientific;return; }
  if(selected==1) { degrees=!degrees;return; }
  if(selected==2) { if(!expression.empty())expression.pop_back();return; }
  if(selected==3) { expression.clear();result="0";return; }
  if(selected==4) {
    double value;std::string error;
    if(Calculator::evaluate(expression,degrees,ans,value,error)) { ans=value;char text[40];std::snprintf(text,sizeof(text),"%.10g",value);result=text; }
    else result=error;
    return;
  }
  const char* key=label(selected);if(expression.size()+std::char_traits<char>::length(key)<=128)expression+=key;
}
