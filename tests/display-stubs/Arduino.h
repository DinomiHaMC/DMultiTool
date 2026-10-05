#pragma once
#include <string>
#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <sstream>
using std::min;using std::max;
class String {
 std::string text;
 public:
 String()=default;String(const char* s):text(s?s:""){}String(const std::string& s):text(s){}
 String(unsigned long n,int base=10){std::ostringstream stream;if(base==16)stream<<std::hex;stream<<n;text=stream.str();}
 String(int n,int base=10):String((unsigned long)n,base){}
 String(unsigned int n,int base=10):String((unsigned long)n,base){}
 String& operator+=(char c){text+=c;return *this;}String& operator+=(const String& s){text+=s.text;return *this;}char operator[](size_t i)const{return text[i];}void remove(size_t start){text.erase(start);}
 bool isEmpty()const{return text.empty();}size_t length()const{return text.length();}const char* c_str()const{return text.c_str();}
 String substring(size_t start,size_t end=std::string::npos)const{return start>=text.size()?String():String(text.substr(start,end==std::string::npos?end:end-start));}
 friend String operator+(const String& a,const String& b){return String(a.text+b.text);}
 friend bool operator==(const String& a,const String& b){return a.text==b.text;}
 friend bool operator!=(const String& a,const String& b){return !(a==b);}
};
inline uint32_t testUiMillis=1000;
inline uint32_t millis(){return testUiMillis;}
template<class T> T constrain(T value,T low,T high){return std::min(high,std::max(low,value));}
