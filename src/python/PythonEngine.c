#include "PythonEngine.h"
#include "TkinterSource.h"
#include "../third_party/pikapython/PikaMain.h"
#include "../third_party/pikapython/PikaVM.h"
#include "../third_party/pikapython/PikaStdLib_SysObj.h"
#include "../third_party/pikapython/device.h"
#include <setjmp.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

// Pika's default OOM handler spins. Our owned allocation list and C-only
// recovery boundary release the entire interpreter, including partial parses.
typedef struct Block { struct Block *prev,*next;size_t size;double alignment; } Block;
static Block* blocks;
static size_t used,peak,budget;
static const HPythonHost* host;
static jmp_buf recovery;
static int active;
static unsigned allocationTicks;
volatile PikaObj* __pikaMain;
extern volatile VMSignal g_PikaVMSignal;
extern volatile PikaMemInfo g_PikaMemInfo;
extern volatile PikaObjState g_PikaObjState;
static void stop_check(void) {
 if(active&&host&&host->stop(host->context))longjmp(recovery,1);
 if(active&&g_PikaVMSignal.vm_cnt>8) {
  if(host->log)host->log(host->context,"Python call depth limit");
  longjmp(recovery,3);
 }
}
static void fail(const char* message) {
 if(host&&host->log)host->log(host->context,message);
 longjmp(recovery,3);
}
void* pika_platform_malloc(size_t size) {
 stop_check();
 if(++allocationTicks%1024==0&&host->yield)host->yield(host->context);
 if(size>SIZE_MAX-sizeof(Block)||size+sizeof(Block)>budget-used)longjmp(recovery,2);
 Block* b=malloc(sizeof(Block)+size);
 if(!b)longjmp(recovery,2);
 b->prev=NULL;b->next=blocks;b->size=size;
 if(blocks)blocks->prev=b;
 blocks=b;used+=size+sizeof(Block); if(used>peak)peak=used;
 return b+1;
}
void pika_platform_free(void* pointer) {
 if(!pointer)return;
 Block* b=((Block*)pointer)-1;
 if(b->prev)b->prev->next=b->next;else blocks=b->next;
 if(b->next)b->next->prev=b->prev;
 used-=b->size+sizeof(Block);free(b);
}
void* pika_platform_calloc(size_t n,size_t size) {
 if(size&&n>SIZE_MAX/size)longjmp(recovery,2);
 void* result=pika_platform_malloc(n*size);memset(result,0,n*size);return result;
}
void* pika_platform_realloc(void* pointer,size_t size) {
 if(!pointer)return pika_platform_malloc(size);
 if(!size) { pika_platform_free(pointer);return NULL; }
 Block* b=((Block*)pointer)-1;
 void* next=pika_platform_malloc(size);
 memcpy(next,pointer,b->size<size?b->size:size);pika_platform_free(pointer);return next;
}
void pika_platform_printf(char* fmt,...) {
 char text[256];va_list args;va_start(args,fmt);vsnprintf(text,sizeof(text),fmt,args);va_end(args);
 if(host&&host->log)host->log(host->context,text);
}
void pika_hook_instruct(void) { stop_check();if(host->yield)host->yield(host->context); }
void pika_platform_thread_yield(void) { pika_hook_instruct(); }
void pika_platform_error_handle(void) { fail("Python runtime/syntax error"); }
void pika_platform_panic_handle(void) { fail("Python interpreter panic"); }
char pika_platform_getchar(void) { fail("input()/REPL unavailable; use device.button()");return 0; }
FILE* pika_platform_fopen(const char* filename,const char* mode) { (void)filename;(void)mode;return NULL; }

int hp_python_run(const char* source,const HPythonHost* backend,size_t maximum) {
 host=backend;allocationTicks=0;budget=maximum;used=peak=0;blocks=NULL;active=1;
 memset((void*)&g_PikaVMSignal,0,sizeof(g_PikaVMSignal));
 memset((void*)&g_PikaMemInfo,0,sizeof(g_PikaMemInfo));
 memset((void*)&g_PikaObjState,0,sizeof(g_PikaObjState));__pikaMain=NULL;
 int status=setjmp(recovery);
 if(status==0) {
  PikaObj* root=newRootObj("hacker_pro_python",New_PikaMain);
  obj_newDirectObj(root,"tkinter",New_PikaStdLib_SysObj);
  PikaObj* tk=obj_getObj(root,"tkinter");
  pikaVM_run(tk,(char*)hp_tkinter_source);
  pikaVM_run(root,(char*)source);
  // All allocations are private to this one worker, including cyclic objects.
 }
 active=0;
 while(blocks) { Block* next=blocks->next;free(blocks);blocks=next; }
 used=0;__pikaMain=NULL;
 memset((void*)&g_PikaVMSignal,0,sizeof(g_PikaVMSignal));
 memset((void*)&g_PikaMemInfo,0,sizeof(g_PikaMemInfo));
 memset((void*)&g_PikaObjState,0,sizeof(g_PikaObjState));host=NULL;
 return status;
}
size_t hp_python_peak(void) { return peak; }
static void text_copy(char* dest,size_t capacity,const char* text) {
 if(!text||strlen(text)>=capacity)fail("Python argument too long");
 strcpy(dest,text);
}
static HPythonReply rpc(HPythonRequest* request) {
 stop_check();HPythonReply response={0};
 if(!host->exchange(host->context,request,&response)) { stop_check();fail("Device request failed / timeout"); }
 return response;
}
static int simple(int op) { HPythonRequest request={0};request.op=op;return rpc(&request).value; }
static char* string_reply(PikaObj* self,int op,const char* path) {
 HPythonRequest request={0};request.op=op;
 if(path)text_copy(request.path,sizeof(request.path),path);
 HPythonReply response=rpc(&request);obj_setStr(self,"_reply",response.text);return obj_getStr(self,"_reply");
}
static int with_text(int op,const char* text) {
 HPythonRequest request={0};request.op=op;text_copy(request.text,sizeof(request.text),text);return rpc(&request).value;
}
static int file_op(int op,const char* path,const char* text) {
 HPythonRequest request={0};request.op=op;text_copy(request.path,sizeof(request.path),path);
 if(text)text_copy(request.text,sizeof(request.text),text);
 return rpc(&request).value;
}
int device_alive(PikaObj* self) { (void)self;return simple(HP_ALIVE); }
char* device_button(PikaObj* self) { return string_reply(self,HP_BUTTON,NULL); }
char* device_pressed(PikaObj* self) { return string_reply(self,HP_PRESSED,NULL); }
int device_millis(PikaObj* self) { (void)self;return simple(HP_MILLIS); }
void device_sleep_ms(PikaObj* self,int ms) {
 (void)self;HPythonRequest request={0};request.op=HP_SLEEP;request.args[0]=ms;rpc(&request);
}
void device_title(PikaObj* self,char* text) { (void)self;with_text(HP_TITLE,text); }
int device_label(PikaObj* self,char* text) { (void)self;return with_text(HP_LABEL,text); }
int device_button_widget(PikaObj* self,char* text) { (void)self;return with_text(HP_WIDGET,text); }
void device_set_text(PikaObj* self,int id,char* text) {
 (void)self;HPythonRequest request={0};request.op=HP_SET_TEXT;request.args[0]=id;text_copy(request.text,sizeof(request.text),text);rpc(&request);
}
int device_selected(PikaObj* self) { (void)self;return simple(HP_SELECTED); }
int device_rectangle(PikaObj* self,int x1,int y1,int x2,int y2,int color) {
 (void)self;HPythonRequest request={0};request.op=HP_RECT;request.args[0]=x1;request.args[1]=y1;request.args[2]=x2;request.args[3]=y2;request.args[4]=color;return rpc(&request).value;
}
void device_clear_canvas(PikaObj* self) { (void)self;simple(HP_CLEAR); }
int device_width(PikaObj* self) { (void)self;return simple(HP_WIDTH); }
int device_height(PikaObj* self) { (void)self;return simple(HP_HEIGHT); }
int device_ir_nec(PikaObj* self,int address,int command,int repeats) {
 (void)self;HPythonRequest request={0};request.op=HP_IR;request.args[0]=address;request.args[1]=command;request.args[2]=repeats;return rpc(&request).value;
}
void device_beep(PikaObj* self,int frequency,int duration) {
 (void)self;HPythonRequest request={0};request.op=HP_BEEP;request.args[0]=frequency;request.args[1]=duration;rpc(&request);
}
char* device_read_text(PikaObj* self,char* path) { return string_reply(self,HP_READ,path); }
char* device_listdir(PikaObj* self,char* path) { return string_reply(self,HP_LIST,path); }
int device_write_text(PikaObj* self,char* path,char* text) { (void)self;return file_op(HP_WRITE,path,text); }
int device_append_text(PikaObj* self,char* path,char* text) { (void)self;return file_op(HP_APPEND,path,text); }
int device_mkdir(PikaObj* self,char* path) { (void)self;return file_op(HP_MKDIR,path,NULL); }
int device_remove(PikaObj* self,char* path) { (void)self;return file_op(HP_REMOVE,path,NULL); }
void device_destroy(PikaObj* self) { (void)self;simple(HP_DESTROY); }
