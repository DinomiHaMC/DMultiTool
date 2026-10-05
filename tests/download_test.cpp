#include "../src/services/DownloadService.h"
#include <HTTPClient.h>
#include <WiFi.h>
#include <esp_heap_caps.h>
#include <freertos/task.h>
#include <cassert>
#include <iostream>
#include <thread>
int main() {
  DownloadService service;
  responseBody.assign(8000,42);
  auto finish=[&] {
    for(int i=0;service.active&&i<10000;i++) { service.update();std::this_thread::sleep_for(std::chrono::microseconds(100)); }
    assert(!service.active);joinTasks();assert(clientObjects==0);
  };
  assert(service.start("http://test/file","/good"));finish();assert(SD.exists("/good")&&!SD.exists("/good.mtpart"));assert(SD.nodes["/good"]->bytes==responseBody);
  assert(!service.start("http://test/file","/good"));
  assert(service.start("http://test/file","/race"));
  SD.nodes["/race"]=std::make_shared<Node>();SD.nodes["/race"]->bytes={99};
  finish();assert(SD.nodes["/race"]->bytes==std::vector<uint8_t>{99}&&!SD.exists("/race.mtpart"));
  unknownSize=true;assert(service.start("http://test/file","/unknown"));finish();assert(SD.exists("/unknown"));unknownSize=false;
  responseStatus=404;assert(service.start("http://test/file","/404"));finish();assert(!SD.exists("/404")&&!SD.exists("/404.mtpart"));
  responseStatus=-5;assert(service.start("http://test/file","/failed"));finish();assert(!SD.exists("/failed"));
  responseStatus=200;responseBody.assign(500000,11);assert(service.start("http://test/file","/cancel"));service.cancel();finish();assert(!SD.exists("/cancel")&&!SD.exists("/cancel.mtpart"));
  responseBody.assign(1000,13);failWrites=true;assert(service.start("http://test/file","/full"));finish();failWrites=false;assert(!SD.exists("/full")&&!SD.exists("/full.mtpart"));
  failQueue=true;assert(!service.start("http://test/file","/queue"));failQueue=false;assert(!SD.exists("/queue.mtpart"));
  failTask=true;assert(!service.start("http://test/file","/task"));failTask=false;assert(!SD.exists("/task.mtpart"));
  assert(service.start("https://test/file","/https"));finish();assert(SD.exists("/https"));
  SD.nodes["/config/ca.pem"]=std::make_shared<Node>();SD.nodes["/config/ca.pem"]->bytes={1,2,3};
  assert(!service.start("https://test/file","/bad-ca"));SD.remove("/config/ca.pem");
  fakeHeap=1000;assert(!service.start("http://test/file","/ram"));fakeHeap=150000;
  WiFi.state=0;assert(!service.start("http://test/file","/wifi"));WiFi.state=3;
  responseBody.clear();assert(service.start("http://test/file","/empty"));finish();assert(SD.exists("/empty")&&SD.nodes["/empty"]->bytes.empty());
  std::cout<<"Download pipeline passed: queued stream, known/unknown size, HTTP/error failure, cancel, SD full, worker/queue failure, CA/RAM/WiFi preflight, empty file, resource cleanup\n";
}
