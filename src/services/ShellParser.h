#pragma once
#include <string>
#include <vector>
namespace MtSh {
bool tokenize(const std::string& command,std::vector<std::string>& args);
bool path(const std::string& cwd,const std::string& input,std::string& out);
}
