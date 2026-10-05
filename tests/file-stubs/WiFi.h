#pragma once
constexpr int WL_CONNECTED=3;
struct FakeWiFi { int state=3;int status()const { return state; } };
inline FakeWiFi WiFi;
