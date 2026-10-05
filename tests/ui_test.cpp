#include <cassert>
#include <iostream>
#include "../src/ui/UI.h"
int main(){
 DisplayManager display;Settings settings;UI ui(display,settings);int called=0;bool yes=false;
 MenuPage root;root.title="Root";root.items={{"One","",Icon::App,[&]{called++;}},{"Two","",Icon::App}};ui.page(root,false);
 ui.handle(InputEvent::Up);assert(ui.model().selected==1);ui.handle(InputEvent::Down);assert(ui.model().selected==0);ui.handle(InputEvent::Ok);assert(called==1);
 ui.confirm("Delete?","Test file",[&]{yes=true;});assert(ui.model().selected==0);ui.handle(InputEvent::Ok);assert(!yes&&ui.model().title=="Root");
 ui.confirm("Delete?","Test file",[&]{yes=true;});ui.handle(InputEvent::Down);ui.handle(InputEvent::Ok);assert(yes&&ui.model().title=="Root");
 String result;ui.textInput("Name","abc",[&](String text){result=text;});ui.handle(InputEvent::Up);ui.handle(InputEvent::Right);ui.handle(InputEvent::Right);ui.handle(InputEvent::Ok);assert(ui.inputModel().value=="ab");
 assert(ui.model().title=="Root"); // Input is not converted to menu rows.
 ui.handle(InputEvent::Left); // Backspace -> Enter
 ui.handle(InputEvent::Ok);assert(result=="ab"&&!ui.isInput()&&ui.model().title=="Root");
 int cancelled=0;ui.progress("Work","Waiting",[&]{cancelled++;});ui.handle(InputEvent::OkLong);assert(cancelled==1&&ui.model().title=="Root");
 ui.progress("Work","Waiting",[&]{cancelled++;});ui.handle(InputEvent::Down);ui.handle(InputEvent::Ok);assert(cancelled==2&&ui.model().title=="Root");
 KeyboardModel keyboard;
 keyboard.begin("Name","",12,false,0);
 keyboard.move(KeyboardModel::Direction::Up);keyboard.press(); // Russian
 assert(keyboard.language==KeyboardModel::Language::Russian);
 keyboard.move(KeyboardModel::Direction::Down);keyboard.press();
 assert(keyboard.value=="б"); // globe's center aligns with column 1
 keyboard.erase();assert(keyboard.value.empty());
 keyboard.selected=6;keyboard.press();assert(keyboard.value=="ё");
 keyboard.selected=keyboard.characters()+KeyboardModel::Shift;keyboard.press();
 keyboard.selected=6;keyboard.press();assert(keyboard.value=="ёЁ");
 keyboard.selected=keyboard.characters()+KeyboardModel::Lang;keyboard.press();
 assert(keyboard.language==KeyboardModel::Language::Symbols);
 std::string symbols;for(int i=0;i<keyboard.characters();i++)symbols+=char(keyboard.character(i));
 for(char c:std::string("0123456789 !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"))assert(symbols.find(c)!=std::string::npos);
 keyboard.press();assert(keyboard.language==KeyboardModel::Language::English);
 keyboard.begin("Limited","",3,false,0);keyboard.selected=keyboard.characters();keyboard.press();
 keyboard.selected=0;keyboard.press();keyboard.press();assert(keyboard.value=="а"); // 2-byte letter cannot overflow byte limit
 ui.textInput("Russian","Привет",[&](String text){result=text;});ui.handle(InputEvent::Up);ui.handle(InputEvent::Right);ui.handle(InputEvent::Right);ui.handle(InputEvent::Ok);
 assert(ui.inputModel().value=="Приве");ui.draw();
 MockTFT::write("docs/screenshots/keyboard-russian.svg",240,320);
 ui.handle(InputEvent::Left);ui.handle(InputEvent::Ok);assert(result=="Приве");
 ui.textInput("SSID","a",[&](String text){result=text;});ui.handle(InputEvent::Up);ui.handle(InputEvent::Ok); // RU layout
 ui.draw();MockTFT::write("docs/screenshots/keyboard-layout-ru.svg",240,320);
 size_t shapes=MockTFT::shapes.size();ui.draw();assert(MockTFT::shapes.size()==shapes);
 ui.handle(InputEvent::Ok);ui.draw();MockTFT::write("docs/screenshots/keyboard-symbols.svg",240,320);
 ui.handle(InputEvent::Ok);ui.draw();MockTFT::write("docs/screenshots/keyboard-english.svg",240,320);
 display.rotation(1);ui.draw();MockTFT::write("docs/screenshots/keyboard-landscape.svg",320,240);
 ui.back();assert(!ui.isInput());
 ui.textInput("Password","Секрет",[&](String text){result=text;},64,true);ui.draw();
 for(const auto& shape:MockTFT::shapes)assert(shape.find("Секрет")==std::string::npos);
 assert(ui.inputModel().value=="Секрет");ui.handle(InputEvent::Up);ui.handle(InputEvent::Right);ui.handle(InputEvent::Right);ui.handle(InputEvent::Ok);assert(ui.inputModel().value=="Секре");
 ui.handle(InputEvent::OkLong);assert(!ui.isInput());
 std::cout<<"UI tests passed: wrap, confirmation defaults, keyboard edit/submit, cancel\n";
}
