#include "../src/services/BLEScanSession.h"
#include <cassert>
#include <iostream>
struct Scan {
 bool fail=false,active=false;int starts=0,stops=0,ticks=0;unsigned duration=0;
 bool start(unsigned ms,bool keep){assert(!keep);starts++;duration=ms;active=!fail;return !fail;}
 bool isScanning()const{return active;}
 void stop(){stops++;active=false;}
};
int main(){
 using namespace BLEScanSession;
 std::atomic<bool> cancel{false};Scan scanner;
 auto complete=[&]{scanner.ticks++;if(scanner.ticks>=3)scanner.active=false;};
 assert(run(scanner,cancel,complete,[&]{return !scanner.active;})==Result::Complete);
 assert(scanner.starts==1&&scanner.duration==6000&&scanner.ticks==3);
 // The next scan must start afresh, even if the previous run had no devices.
 scanner.ticks=0;assert(run(scanner,cancel,complete,[&]{return !scanner.active;})==Result::Complete);assert(scanner.starts==2);
 scanner.fail=true;scanner.ticks=0;
 assert(run(scanner,cancel,complete,[&]{return !scanner.active;})==Result::StartFailed&&scanner.ticks==0);
 scanner.fail=false;cancel=true;int starts=scanner.starts;
 assert(run(scanner,cancel,complete,[&]{return !scanner.active;})==Result::Cancelled&&scanner.starts==starts);
 cancel=false;scanner.ticks=0;
 assert(run(scanner,cancel,[&]{cancel=true;},[]{return false;})==Result::Cancelled);
 assert(scanner.stops==1&&!scanner.active);
 // Simulate cancellation exactly between the pre-start check and SDK start.
 struct Racing:Scan {std::atomic<bool>& flag;explicit Racing(std::atomic<bool>& f):flag(f){} bool start(unsigned ms,bool keep){flag=true;return Scan::start(ms,keep);}} racing(cancel);
 cancel=false;assert(run(racing,cancel,[]{},[]{return false;})==Result::Cancelled&&racing.stops==1);
 cancel=false;scanner.ticks=0;int flushed=0;
 assert(run(scanner,cancel,[&]{scanner.active=false;flushed++;},[&]{return flushed>=2;})==Result::Complete);
 assert(flushed==2); // Controller stops first; callback flushes results later.
 std::cout<<"BLE scan session passed: explicit start errors, repeat scan, completion and cancellation before/during SDK start\n";
}
