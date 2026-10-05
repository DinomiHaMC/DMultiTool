#include "GamesApp.h"
#include <esp_system.h>
void GamesApp::home() {
  auto p=menu("Games");
  item(p,"Snake",[this]{start(0);},"Arrows steer | OK pause");
  item(p,"Minesweeper",[this]{start(1);},"OK open | double OK flag");
  item(p,"Tetris",[this]{start(2);},"UP rotate | DOWN fall | OK drop");
  p.hint="OK Play | Hold OK in game to exit";
  ui.page(std::move(p),false);
}
void GamesApp::start(uint8_t choice) {
  game=choice;playing=true;confirming=pendingReveal=paused=false;
  if(game==0)snake.reset(esp_random());
  else if(game==1)mines.reset(esp_random());
  else tetris.reset(esp_random());
  heldBefore=Key::None;tickAt=millis();compose();s.display.invalidate();
}
void GamesApp::compose() {
  if(game==0)snake.board(board);
  else if(game==1)mines.board(board);
  else tetris.board(board);
  ui.dirty=true;
}
void GamesApp::requestExit() {
  confirming=true;pendingReveal=false;
  ui.confirm("Exit game?","Progress will be lost",[this]{playing=confirming=false;s.display.invalidate();home();});
  s.display.invalidate();
}
void GamesApp::handleInput(InputEvent e) {
  if(!playing) { ui.handle(e);return; }
  if(confirming) {
    if(e==InputEvent::BackLong)return;
    ui.handle(e);
    if(ui.model().title!="Exit game?") { confirming=false;tickAt=millis();s.display.invalidate();ui.dirty=true; }
    return;
  }
  if(e==InputEvent::OkLong) { requestExit();return; }
  if(e==InputEvent::BackLong)return;
  if(game==0&&e==InputEvent::Ok&&!board.over) { paused=!paused;tickAt=millis();s.display.invalidate();ui.dirty=true;return; }
  if(paused)return;
  if(board.over) { if(e==InputEvent::Ok)start(game);return; }
  if(game==0) {
    if(e==InputEvent::Up)snake.direction(0,-1);
    if(e==InputEvent::Down)snake.direction(0,1);
    if(e==InputEvent::Right)snake.direction(1,0);
    if(e==InputEvent::Left)snake.direction(-1,0);
  } else if(game==1) {
    if(pendingReveal&&e!=InputEvent::Ok) { pendingReveal=false;mines.reveal(); }
    if(e==InputEvent::Up)mines.move(0,-1);
    if(e==InputEvent::Down)mines.move(0,1);
    if(e==InputEvent::Left)mines.move(-1,0);
    if(e==InputEvent::Right)mines.move(1,0);
    if(e==InputEvent::Ok) {
      if(pendingReveal&&millis()-revealAt<300) { pendingReveal=false;mines.flag(); }
      else { pendingReveal=true;revealAt=millis(); }
    }
  } else {
    if(e==InputEvent::Up)tetris.rotate();
    if(e==InputEvent::Down) { if(tetris.move(0,1))tetris.score++; }
    if(e==InputEvent::Ok)tetris.drop();
  }
  compose();
}
void GamesApp::update() {
  if(!playing||confirming||paused)return;
  Key held=s.input.held();
  if(game==0&&held==Key::Left)snake.direction(-1,0);
  if(game==2&&(held==Key::Left||held==Key::Right)&&!board.over) {
    if(held!=heldBefore||millis()-heldAt>150) { tetris.move(held==Key::Left?-1:1,0);heldAt=millis();compose(); }
  }
  heldBefore=held;
  if(game==1&&pendingReveal&&millis()-revealAt>=300) { pendingReveal=false;mines.reveal();compose(); }
  uint32_t interval=game==0?max(65,220-(snake.length-4)*3):max(100,700-(tetris.lines/10)*70);
  if(game!=1&&!board.over&&millis()-tickAt>=interval) {
    tickAt=millis();if(game==0)snake.step();else tetris.tick();compose();
  }
}
void GamesApp::draw() {
  if(!playing||confirming)ui.draw();
  else { s.display.renderGame(board,s.config.values.theme,paused);ui.dirty=false; }
}
