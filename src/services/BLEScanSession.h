#pragma once
#include <atomic>
namespace BLEScanSession {
constexpr unsigned DurationMs=6000;
enum class Result { Complete, Cancelled, StartFailed };
// SDK start and completion are distinct; never turn start failure into
// an apparently successful empty scan. Cancellation may precede task startup.
template<class Scanner,class Wait,class Completed>
Result run(Scanner& scanner,const std::atomic<bool>& cancelled,Wait wait,Completed completed) {
  if(cancelled.load())return Result::Cancelled;
  if(!scanner.start(DurationMs,false))return Result::StartFailed;
  // Wait for onScanEnd to flush pending scan responses before copying results.
  while(scanner.isScanning()||(!cancelled.load()&&!completed())) {
    if(cancelled.load())scanner.stop();
    wait();
  }
  return cancelled.load()?Result::Cancelled:Result::Complete;
}
}
