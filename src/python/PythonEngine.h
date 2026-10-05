#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// POD-only interface: interpreter runs on its worker; hardware stays in main task.
enum HPythonOp {
 HP_BUTTON,HP_PRESSED,HP_SLEEP,HP_MILLIS,HP_ALIVE,HP_TITLE,HP_LABEL,HP_WIDGET,
 HP_SET_TEXT,HP_SELECTED,HP_RECT,HP_CLEAR,HP_WIDTH,HP_HEIGHT,HP_IR,HP_BEEP,
 HP_READ,HP_WRITE,HP_APPEND,HP_LIST,HP_MKDIR,HP_REMOVE,HP_DESTROY
};
typedef struct { int op;int32_t args[6];char path[128];char text[1025]; } HPythonRequest;
typedef struct { int32_t value;char text[2049]; } HPythonReply;
typedef struct {
 void* context;
 int (*exchange)(void*,const HPythonRequest*,HPythonReply*);
 int (*stop)(void*);
 void (*yield)(void*);
 void (*log)(void*,const char*);
} HPythonHost;
// 0 success, 1 cancelled, 2 memory limit, 3 runtime/syntax/bridge error.
int hp_python_run(const char* source,const HPythonHost* host,size_t budget);
size_t hp_python_peak(void);
#ifdef __cplusplus
}
#endif
