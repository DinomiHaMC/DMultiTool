#pragma once
#include <Arduino.h>
#include <NetworkClientSecure.h>
#include <vector>
constexpr int HTTPC_STRICT_FOLLOW_REDIRECTS=1;
inline std::vector<uint8_t> responseBody;
inline int responseStatus=200;
inline bool unknownSize=false;
class HTTPClient {
public:
  void setConnectTimeout(int) {}void setTimeout(int) {}void setFollowRedirects(int) {}
  bool begin(NetworkClient&,const String&) { return true; }
  int GET() { return responseStatus; }
  int getSize() { return unknownSize?-1:int(responseBody.size()); }
  int writeToStream(Stream* sink) {
    size_t sent=0;
    while(sent<responseBody.size()) {
      size_t n=std::min(size_t(37),responseBody.size()-sent);
      if(sink->write(responseBody.data()+sent,n)!=n)return -10;
      sent+=n;
    }
    return sent;
  }
  void end() {}
};
