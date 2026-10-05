#include "../src/services/Calculator.h"
#include "../src/services/ShellParser.h"
#include "../src/ui/GridNavigation.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main() {
  double value;std::string error;
  auto eval=[&](const char* text,double expected,bool degrees=true,double ans=0) {
    assert(Calculator::evaluate(text,degrees,ans,value,error));assert(std::abs(value-expected)<1e-8);
  };
  eval("2+3*4",14);eval("(2+3)*4",20);eval("-2^2",-4);eval("(-2)^2",4);eval("2^3^2",512);
  eval("2^-2",0.25);eval("5!",120);eval("50%",0.5);eval("sin(30)",0.5);
  eval("sin(pi/2)",1,false);eval("sqrt(81)+log(100)+ln(e)",12);eval("ans*2",14,true,7);
  eval("pow(2,3)",8);eval("1e-3",0.001);eval("acos(0)",90);eval("abs(-3)",3);
  for(auto text:{"1/0","sqrt(-1)","log(0)","171!","2.5!","sin(","1 2","pow(1)","1e999","unknown(1)",""})assert(!Calculator::evaluate(text,true,0,value,error));
  assert(!Calculator::evaluate(std::string(50,'(')+"1"+std::string(50,')'),true,0,value,error));
  CalculatorModel model;model.selected=0;model.press();assert(model.scientific&&model.count()==40);
  model.selected=1;model.press();assert(!model.degrees);model.expression="2+2";model.selected=4;model.press();assert(model.result=="4"&&model.ans==4);
  model.selected=2;model.press();assert(model.expression=="2+");model.selected=3;model.press();assert(model.expression.empty());
  for(int n=1;n<=70;n++)for(int i=0;i<n;i++)for(auto direction:{std::pair<int,int>{-1,0},{1,0},{0,-1},{0,1}}) {
    int moved=GridNavigation::move(i,n,3,direction.first,direction.second,true);assert(moved>=0&&moved<n);
  }
  assert(GridNavigation::move(2,24,3,0,1,true)==5);
  assert(GridNavigation::move(11,24,3,0,1,true)==14);
  assert(GridNavigation::move(0,11,3,-1,0,true)==2);
  assert(GridNavigation::move(10,11,3,1,0,true)==9);
  std::vector<std::string> args;std::string path;
  assert(MtSh::tokenize("write 'a b.txt' \"hello world\"",args));assert(args.size()==3&&args[1]=="a b.txt"&&args[2]=="hello world");
  assert(MtSh::tokenize("wget https://example.test/a?x=1&y=2 /file.bin",args)&&args.size()==3);
  assert(!MtSh::tokenize("write \"unterminated",args));assert(!MtSh::tokenize("rm x\nreboot",args));
  assert(MtSh::path("/a/b","../c",path)&&path=="/a/c");
  assert(MtSh::path("/a","/b//./c",path)&&path=="/b/c");
  assert(!MtSh::path("/","../escape",path));assert(!MtSh::path("/","a\\b",path));
  assert(!MtSh::path("/",std::string(241,'a'),path));
  std::cout<<"Models passed: scientific calculator/errors, 2D navigation/pages, MtSh quotes/paths/bounds\n";
}
