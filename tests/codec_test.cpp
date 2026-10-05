#include <cassert>
#include <iostream>
#include "../src/services/NDEFCodec.h"
#include "../src/services/ScriptParser.h"
int main(){
 for(bool uri:{false,true}){
  auto encoded=NDEF::encode(uri?"https://example.com":"Hello",uri);
  std::string decoded;assert(NDEF::decode(encoded.data(),encoded.size(),decoded));
  assert(decoded.find(uri?"https://example.com":"Hello")!=std::string::npos);
  for(size_t cut=0;cut+4<encoded.size();cut++){
   std::string out;assert(!NDEF::decode(encoded.data(),cut,out));
  }
 }
 assert(NDEF::encode(std::string(101,'A'),false).empty());
 std::string output;uint8_t bad[]={3,255,255,255,0};assert(!NDEF::decode(bad,sizeof(bad),output));
 uint8_t mime[]={3,8,0xD2,3,2,'a','/','b','O','K',0xFE};assert(NDEF::decode(mime,sizeof(mime),output));assert(output.find("MIME: a/b")!=std::string::npos);
 uint8_t languageOverflow[]={3,5,0xD1,1,1,'T',0x3F,0xFE};assert(!NDEF::decode(languageOverflow,sizeof(languageOverflow),output));
 std::vector<std::string> tokens;
 assert(Script::tokenize("PRINT \"Hello world\" # comment",tokens));assert(tokens.size()==2&&tokens[1]=="Hello world");
 assert(Script::tokenize("IR_NEC 0x00 0x10 0",tokens)&&tokens.size()==4);
 assert(Script::tokenize("PRINT \"\"",tokens)&&tokens[1].empty());
 assert(!Script::tokenize("PRINT \"unterminated",tokens));
 assert(!Script::tokenize(std::string(161,'a'),tokens));
 assert(!Script::tokenize("a b c d e f g",tokens));
 std::cout<<"NDEF and script parser tests passed\n";
}
