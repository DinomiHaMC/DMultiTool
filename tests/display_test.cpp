#include <cassert>
#include <iostream>
#include <cstring>
#include "../src/ui/DisplayManager.h"
int main(){
 DisplayManager display;display.begin(0);MenuPage launcher;launcher.title="Hacker Pro 2000";launcher.launcher=true;
 const char* names[]={"WiFi","Bluetooth","NFC","Infrared","Files","Scripts","Tools","Settings","About"};
 Icon icons[]={Icon::WiFi,Icon::BLE,Icon::NFC,Icon::IR,Icon::Folder,Icon::Script,Icon::Tool,Icon::Settings,Icon::Info};
 for(int i=0;i<9;i++)launcher.items.push_back({names[i],"Open application",icons[i]});
 Status status;status.sd=status.wifi=status.nfc=true;status.heap=120000;
 for(unsigned theme=0;theme<4;theme++){
  display.invalidate();display.render(launcher,status,"",ToastType::Info,theme,true);
  std::string path="docs/screenshots/launcher-"+std::to_string(theme)+".svg";MockTFT::write(path.c_str(),240,320);
 }
 size_t count=MockTFT::shapes.size();display.render(launcher,status,"",ToastType::Info,3,true);assert(MockTFT::shapes.size()==count);
 launcher.selected=1;display.render(launcher,status,"",ToastType::Info,3,true);assert(MockTFT::shapes.size()>count);
 count=MockTFT::shapes.size();display.sleep(true);display.render(launcher,status,"",ToastType::Info,3,true);assert(MockTFT::shapes.size()==count);display.sleep(false);
 display.rotation(1);display.render(launcher,status,"",ToastType::Info,0,true);MockTFT::write("docs/screenshots/launcher-landscape.svg",320,240);
 display.rotation(0);
 Games::Board board;board.kind=Games::Board::Snake;board.width=18;board.height=20;board.cells[100]=2;board.cells[101]=1;board.cells[60]=3;
 display.renderGame(board,0,false);MockTFT::write("docs/screenshots/game-snake.svg",240,320);
 count=MockTFT::shapes.size();display.renderGame(board,0,false);assert(MockTFT::shapes.size()==count);
 board.cells[99]=2;board.cells[100]=1;display.renderGame(board,0,false);assert(MockTFT::shapes.size()>count);
 display.renderGame(board,0,true);count=MockTFT::shapes.size();display.renderGame(board,0,true);assert(MockTFT::shapes.size()==count);
 PythonView view;strcpy(view.title,"IR + microSD");strcpy(view.status,"Running");view.count=3;view.selected=1;
 strcpy(view.widgets[0].text,"Own hardware demo");strcpy(view.widgets[1].text,"Send NEC 0000/10");view.widgets[1].button=true;
 strcpy(view.widgets[2].text,"Save /python/demo.txt");view.widgets[2].button=true;
 display.renderPython(view,0);MockTFT::write("docs/screenshots/python.svg",240,320);
 count=MockTFT::shapes.size();display.renderPython(view,0);assert(MockTFT::shapes.size()==count);
 view.selected=2;display.renderPython(view,0);assert(MockTFT::shapes.size()>count);
 std::cout<<"Display tests passed: dirty rows, themes, portrait/landscape, sleep, games/Python dirty redraw\n";
}
