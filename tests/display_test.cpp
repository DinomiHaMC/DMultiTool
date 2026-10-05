#include <cassert>
#include <iostream>
#include <cstring>
#include "../src/ui/DisplayManager.h"
#include "../src/ui/Theme.h"
int main(){
 DisplayManager display;display.begin(0);MenuPage launcher;launcher.title="DMultiTool";launcher.launcher=true;launcher.grid=true;launcher.columns=3;
 const char* names[]={"WiFi","Bluetooth","NFC","Infrared","Files","Scripts","Tools","Settings","Games","Utils"};
 Icon icons[]={Icon::WiFi,Icon::BLE,Icon::NFC,Icon::IR,Icon::Folder,Icon::Script,Icon::Tool,Icon::Settings,Icon::Game,Icon::Tool};
 for(int i=0;i<10;i++)launcher.items.push_back({names[i],"Open application",icons[i]});
 Status status;status.sd=status.wifi=status.nfc=true;status.heap=120000;
 for(unsigned theme=0;theme<ThemeManager::Count;theme++){
  display.invalidate();display.render(launcher,status,"",ToastType::Info,theme,true);
  std::string path="docs/screenshots/launcher-"+std::to_string(theme)+".svg";MockTFT::write(path.c_str(),240,320);
 }
 size_t count=MockTFT::shapes.size();display.render(launcher,status,"",ToastType::Info,ThemeManager::Custom,true);assert(MockTFT::shapes.size()==count);
 launcher.selected=1;display.render(launcher,status,"",ToastType::Info,ThemeManager::Custom,true);assert(MockTFT::shapes.size()>count);
 count=MockTFT::shapes.size();display.sleep(true);display.render(launcher,status,"",ToastType::Info,ThemeManager::Custom,true);assert(MockTFT::shapes.size()==count);display.sleep(false);
 display.rotation(1);display.render(launcher,status,"",ToastType::Info,0,true);MockTFT::write("docs/screenshots/launcher-landscape.svg",320,240);
 display.rotation(0);
 CalculatorModel calc;calc.expression="sin(30)+sqrt(81)";calc.result="9.5";calc.scientific=true;
 display.invalidate();display.renderCalculator(calc,4);MockTFT::write("docs/screenshots/calculator-scientific.svg",240,320);
 count=MockTFT::shapes.size();display.renderCalculator(calc,4);assert(count==MockTFT::shapes.size());
 display.rotation(1);display.renderCalculator(calc,5);MockTFT::write("docs/screenshots/calculator-landscape.svg",320,240);display.rotation(0);
 for(int mode=1;mode<=3;mode++) { display.invalidate();for(int frame=1;frame<=80;frame++)display.screensaver(mode,frame*40+mode*4000,mode+3);std::string path="docs/screenshots/screensaver-"+std::to_string(mode)+".svg";MockTFT::write(path.c_str(),240,320); }
 display.invalidate();
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
