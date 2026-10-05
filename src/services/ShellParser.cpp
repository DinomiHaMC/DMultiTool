#include "ShellParser.h"
namespace MtSh {
bool tokenize(const std::string& command,std::vector<std::string>& args) {
  args.clear();std::string part;char quote=0;bool escape=false,started=false;
  if(command.size()>768)return false;
  for(unsigned char c:command) {
    if(c<32&&c!='\t')return false;
    if(escape) { part+=c;escape=false; }
    else if(c=='\\') { escape=true;started=true; }
    else if(quote) { if(c==quote)quote=0;else part+=c; }
    else if(c=='\''||c=='"') { quote=c;started=true; }
    else if(c==' '||c=='\t') {
      if(started) { args.push_back(part);part.clear();started=false; }
    } else { part+=c;started=true; }
    if(part.size()>512||args.size()>8)return false;
  }
  if(quote||escape)return false;
  if(started)args.push_back(part);
  return args.size()<=8;
}
bool path(const std::string& cwd,const std::string& input,std::string& out) {
  if(input.empty()||input.size()>240)return false;
  std::string full=input[0]=='/'?input:cwd+"/"+input;
  std::vector<std::string> parts;std::string part;
  for(size_t i=0;i<=full.size();i++) {
    unsigned char c=i==full.size()?'/':full[i];
    if(c<32||c==127||c=='\\')return false;
    if(c=='/') {
      if(part=="..") { if(parts.empty())return false;parts.pop_back(); }
      else if(!part.empty()&&part!=".")parts.push_back(part);
      part.clear();
    } else part+=c;
  }
  out="";for(const auto& p:parts)out+="/"+p;
  if(out.empty())out="/";
  return out.size()<=240;
}
}
