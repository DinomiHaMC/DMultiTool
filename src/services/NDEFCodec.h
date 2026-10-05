#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
namespace NDEF  {
  std::vector<uint8_t> encode(const std::string& value,bool uri);
  bool decode(const uint8_t* data,size_t length,std::string& output);
}
