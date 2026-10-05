#pragma once
#include <string>
namespace Calculator {
bool evaluate(const std::string& expression,bool degrees,double ans,double& result,std::string& error);
}
class CalculatorModel {
public:
  std::string expression,result="0";
  double ans=0;
  bool scientific=false,degrees=true;
  int selected=0;
  int count()const { return scientific?40:25; }
  const char* label(int index)const;
  void move(int dx,int dy);
  void press();
};
