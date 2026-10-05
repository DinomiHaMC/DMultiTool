#pragma once
#include "MenuApp.h"
#include "../games/GameModels.h"
class GamesApp:public MenuApp {
  Games::Snake snake;
  Games::Minesweeper mines;
  Games::Tetris tetris;
  Games::Board board;
  bool playing=false,confirming=false,pendingReveal=false,paused=false;
  uint8_t game=0;
  uint32_t tickAt=0,heldAt=0,revealAt=0;
  Key heldBefore=Key::None;
  void home()override;
  void start(uint8_t choice);
  void compose();
  void requestExit();
public:
  using MenuApp::MenuApp;
  const char* name()const override { return "Games"; }
  Icon icon()const override { return Icon::Game; }
  bool ownsNavigation()const override { return playing; }
  void update()override;
  void draw()override;
  void handleInput(InputEvent event)override;
  void onClose()override { playing=confirming=pendingReveal=false; }
};
