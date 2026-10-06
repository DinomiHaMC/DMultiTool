#include "GamesApp.h"
#include <esp_system.h>
void GamesApp::home() {
  auto p=menu("Games");
  item(p,"Snake",[this]{start(0);},"Arrows steer | OK pause");
  item(p,"Minesweeper",[this]{start(1);},"OK open | double OK flag");
  item(p,"Tetris",[this]{start(2);},"UP rotate | DOWN fall | OK drop");
  for(int i=3;i<10;i++) {
    auto kind=static_cast<Games::Board::Kind>(i);
    item(p,Games::title(kind),[this,i]{start(i);},Games::controls(kind));
  }
  p.hint="OK Play | Hold OK in game to exit";
  ui.page(std::move(p),false);
}
void GamesApp::start(uint8_t choice) {
  game=choice;playing=true;confirming=pendingReveal=paused=false;
  if(game==0)snake.reset(esp_random());
  else if(game==1)mines.reset(esp_random());
  else if(game==2)tetris.reset(esp_random());
  else if(game==3)puzzle.reset(esp_random());
  else arcade.reset(static_cast<Games::Board::Kind>(game),esp_random());
  heldBefore=Key::None;tickAt=millis();compose();s.display.invalidate();
}
void GamesApp::compose() {
  if(game==0)snake.board(board);
  else if(game==1)mines.board(board);
  else if(game==2)tetris.board(board);
  else if(game==3)puzzle.board(board);
  else arcade.board(board);
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
  if((game==0||game==4||game==5)&&e==InputEvent::Ok&&!board.over) { paused=!paused;tickAt=millis();s.display.invalidate();ui.dirty=true;return; }
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
  } else if(game==2) {
    if(e==InputEvent::Up)tetris.rotate();
    if(e==InputEvent::Down) { if(tetris.move(0,1))tetris.score++; }
    if(e==InputEvent::Ok)tetris.drop();
  } else if(game==3) {
    if(e==InputEvent::Up)puzzle.move(0,-1);
    if(e==InputEvent::Down)puzzle.move(0,1);
    if(e==InputEvent::Right)puzzle.move(1,0);
  } else if((game==6||game==7)&&(e==InputEvent::Up||e==InputEvent::Ok))arcade.action();
  else if((game==8||game==9)&&e==InputEvent::Ok) {
    Games::Controls c;c.fire=true;arcade.step(c);
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
  if(game==3&&held==Key::Left&&heldBefore!=held&&!board.over) { puzzle.move(-1,0);compose(); }
  heldBefore=held;
  if(game==1&&pendingReveal&&millis()-revealAt>=300) { pendingReveal=false;mines.reveal();compose(); }
  if(game>=4) {
    if(!board.over&&millis()-tickAt>=50) {
      tickAt=millis();Games::Controls c;
      c.x=held==Key::Left?-1:held==Key::Right?1:0;
      c.y=held==Key::Up?-1:held==Key::Down?1:0;
      c.fire=(game==8&&held==Key::Up)||(game==9&&held==Key::Down);
      c.thrust=held==Key::Up;arcade.step(c);compose();
    }
    return;
  }
  uint32_t interval=game==0?max(65,220-(snake.length-4)*3):max(100,700-(tetris.lines/10)*70);
  if((game==0||game==2)&&!board.over&&millis()-tickAt>=interval) {
    tickAt=millis();if(game==0)snake.step();else tetris.tick();compose();
  }
}
void GamesApp::draw() {
  if(!playing||confirming)ui.draw();
  else { s.display.renderGame(board,s.config.values.theme,paused);ui.dirty=false; }
}
