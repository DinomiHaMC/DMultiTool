#pragma once
#include <atomic>
inline std::atomic<int> clientObjects{0};
struct NetworkClient { NetworkClient() { clientObjects++; }virtual ~NetworkClient() { clientObjects--; } };
struct NetworkClientSecure:NetworkClient { void setCACert(const char*) {} };
