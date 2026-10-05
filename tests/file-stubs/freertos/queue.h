#pragma once
#include "FreeRTOS.h"
#include <deque>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cstring>
struct FakeQueue { size_t capacity,size;std::deque<std::vector<uint8_t>> bytes;std::mutex mutex;std::condition_variable cv; };
using QueueHandle_t=FakeQueue*;
inline bool failQueue=false;
inline QueueHandle_t xQueueCreate(size_t capacity,size_t size) { if(failQueue)return nullptr;auto q=new FakeQueue;q->capacity=capacity;q->size=size;return q; }
inline int xQueueSend(QueueHandle_t q,const void* bytes,uint32_t timeout) {
  std::unique_lock<std::mutex> lock(q->mutex);
  if(!q->cv.wait_for(lock,std::chrono::milliseconds(timeout),[&]{return q->bytes.size()<q->capacity;}))return pdFALSE;
  q->bytes.emplace_back((const uint8_t*)bytes,(const uint8_t*)bytes+q->size);return pdTRUE;
}
inline int xQueueReceive(QueueHandle_t q,void* bytes,uint32_t) {
  std::lock_guard<std::mutex> lock(q->mutex);if(q->bytes.empty())return pdFALSE;
  memcpy(bytes,q->bytes.front().data(),q->size);q->bytes.pop_front();q->cv.notify_one();return pdTRUE;
}
inline size_t uxQueueMessagesWaiting(QueueHandle_t q) { std::lock_guard<std::mutex> lock(q->mutex);return q->bytes.size(); }
inline void vQueueDelete(QueueHandle_t q) { delete q; }
