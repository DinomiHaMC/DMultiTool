#include "ScriptParser.h"
#include <cctype>
namespace Script  {
  bool tokenize(const std::string& line,std::vector<std::string>& tokens)  {
    tokens.clear();
    std::string token;
    bool quote=false,escape=false,started=false;
    for(char c:line)  {
      if(escape)  {
        if(c!='"'&&c!=92)return false;
        token+=c;
        escape=false;
        continue;
      }
      if(quote&&c==92)  {
        escape=true;
        continue;
      }
      if(c=='"')  {
        quote=!quote;
        started=true;
        continue;
      }
      if(!quote&&c=='#')break;
      if(!quote&&isspace((unsigned char)c))  {
        if(started)  {
          tokens.push_back(token);
          token.clear();
          started=false;
        }
      }
      else  {
        token+=c;
        started=true;
      }
      if(token.size()>160||tokens.size()>6)return false;
    }
    if(quote||escape)return false;
    if(started)tokens.push_back(token);
    return tokens.size()<=6;
  }
}
