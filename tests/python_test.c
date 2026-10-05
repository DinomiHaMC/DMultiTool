#include "../src/python/PythonEngine.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
struct Context { int checks,limit,widgets,ir,writes,labels,rectangles,reads,clock;char text[128];int events; };
static int exchange(void* arg,const HPythonRequest* r,HPythonReply* p) {
 struct Context* c=arg;
 switch(r->op) {
  case HP_LABEL:c->labels++;p->value=c->widgets++;break;
  case HP_WIDGET:p->value=c->widgets++;break;
  case HP_SELECTED:p->value=1;break;
  case HP_ALIVE:p->value=c->events<2;break;
  case HP_BUTTON:if(c->events++==0)strcpy(p->text,"OK");break;
  case HP_IR:assert(r->args[0]==0&&r->args[1]==16&&r->args[2]==0);c->ir++;p->value=1;break;
  case HP_WRITE:assert(strcmp(r->path,"demo.txt")==0);c->writes++;p->value=1;break;
  case HP_MILLIS:p->value=(c->clock+=500);break;
  case HP_RECT:c->rectangles++;p->value=c->rectangles-1;break;
  case HP_PRESSED:strcpy(p->text,"RIGHT");break;
  case HP_READ:c->reads++;strcpy(p->text,"Hello");break;
  case HP_SET_TEXT:snprintf(c->text,sizeof(c->text),"%s",r->text);break;
  case HP_WIDTH:p->value=224;break;
  case HP_HEIGHT:p->value=208;break;
  default:p->value=1;break;
 }
 return 1;
}
static int stop(void* arg){struct Context* c=arg;return c->limit&&++c->checks>c->limit;}
static void log_line(void* arg,const char* text){(void)arg;fputs(text,stderr);}
static int run(struct Context* c,const char* code,size_t budget) {
 HPythonHost host={c,exchange,stop,NULL,log_line};return hp_python_run(code,&host,budget);
}
int main(void) {
 setbuf(stdout,NULL);
 struct Context c={.limit=3000000};
 const char* gui="import tkinter as tk\nimport device\nroot=tk.Tk()\nroot.title('Test')\nlabel=tk.Label(root, text='Ready')\nlabel.pack()\ndef click():\n    label.config(text='Clicked')\n    device.ir_nec(0,16,0)\n    device.write_text('demo.txt','Hello')\ntk.Button(root,text='Run',command=click).pack()\nroot.mainloop()\n";
 assert(run(&c,gui,65536)==0);assert(c.ir==1&&c.writes==1&&strcmp(c.text,"Clicked")==0);
 printf("Python GUI/native tests passed (peak %zu bytes)\n",hp_python_peak());
 c=(struct Context){.limit=200000};assert(run(&c,"while True:\n    x=1\n",65536)==1);
 c=(struct Context){0};assert(run(&c,"x=[]\nwhile True:\n    x.append('0123456789abcdef')\n",20000)==2);
 assert(run(&c,"x = missing_variable\n",65536)==3);
 for(int n=0;n<10;n++)assert(run(&c,"x=2+3\nassert x==5\n",65536)==0);
 assert(run(&c,"def f():\n    f()\nf()\n",65536)==3);
 const char* files[]={"examples/sd/python/buttons.py","examples/sd/python/canvas.py","examples/sd/python/control_panel.py"};
 for(int n=0;n<3;n++) {
  FILE* f=fopen(files[n],"rb");assert(f);char code[4097];size_t size=fread(code,1,4096,f);code[size]=0;fclose(f);
  c=(struct Context){.limit=3000000};assert(run(&c,code,65536)==0);
  if(n==0)assert(strcmp(c.text,"Event: OK")==0);
  if(n==1)assert(c.rectangles>=1);
  if(n==2)assert(c.ir==1);
 }
 assert(run(&c,"import device\nassert device.read_text('demo.txt')=='Hello'\nassert device.pressed()=='RIGHT'\n",65536)==0);
 printf("Python tests passed: infinite-loop cancel, OOM recovery, errors, recursion guard, restart, all SD examples\n");
 return 0;
}
