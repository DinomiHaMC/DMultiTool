#include "../src/ui/DisplayManager.h"
#include "../src/games/ArcadeModels.h"
#include <cassert>
#include <iostream>
int main() {
  DisplayManager display;display.begin(0);
  for(int rotation=0;rotation<2;rotation++) {
    display.rotation(rotation);
    for(int kind=0;kind<10;kind++) {
      Games::Board b;Games::Arcade arcade;Games::Puzzle2048 puzzle;
      if(kind==3)puzzle.reset(12);
      if(kind>=4)arcade.reset(static_cast<Games::Board::Kind>(kind),12);
      for(int frame=0;frame<30;frame++) {
        if(kind>=4) {
          if(arcade.over)arcade.reset(static_cast<Games::Board::Kind>(kind),frame+1);
          Games::Controls c;c.x=frame%10<5?-1:1;c.y=c.x;c.fire=true;c.thrust=true;
          if(frame%7==0)arcade.action();
          arcade.step(c);
          if(arcade.over)arcade.reset(static_cast<Games::Board::Kind>(kind),frame+1);
          arcade.board(b);
        } else if(kind==3) {
          int d=frame%4;puzzle.move(d==0?-1:d==1?1:0,d==2?-1:d==3?1:0);puzzle.board(b);
        } else {
          b.kind=static_cast<Games::Board::Kind>(kind);
          b.width=kind==0?18:kind==1?9:10;b.height=kind==1?9:20;
          int i=(frame*17)%(b.width*b.height);
          b.cells[i]=kind==0?1+frame%3:kind==1?frame%13:1+frame%7;
          b.cursor=kind==1?i:-1;
        }
        if(frame==0)display.invalidate();
        size_t fills=MockTFT::directFills.size();display.renderGame(b,4,false);
        if(frame>0)for(size_t i=fills;i<MockTFT::directFills.size();i++)assert(MockTFT::directFills[i].y<60);
        auto incremental=MockTFT::pixels;
        display.invalidate();display.renderGame(b,4,false);
        assert(incremental==MockTFT::pixels);
        size_t writes=MockTFT::bitmapWrites.size(),shapes=MockTFT::shapes.size();
        display.renderGame(b,4,false);assert(writes==MockTFT::bitmapWrites.size()&&shapes==MockTFT::shapes.size());
      }
    }
    // A moving ball must not blank/repaint the static brick field or paddles.
    Games::Arcade arcade;arcade.reset(Games::Board::Breakout,1);Games::Board b;arcade.board(b);
    display.invalidate();display.renderGame(b,4,false);size_t writes=MockTFT::bitmapWrites.size();
    arcade.step({});arcade.board(b);display.renderGame(b,4,false);
    assert(MockTFT::bitmapWrites.size()>writes&&MockTFT::bitmapWrites.size()-writes<12);
    for(size_t i=writes;i<MockTFT::bitmapWrites.size();i++)assert(MockTFT::bitmapWrites[i].y>130);
    // Overlapping, clipped, removed, compacted and rotated sprites across patch edges.
    b=Games::Board{};b.kind=Games::Board::Asteroids;b.width=160;b.height=180;b.spriteCount=3;
    b.sprites[0]={Games::Board::Sprite::Rock,52,47,0,28,28,2};
    b.sprites[1]={Games::Board::Sprite::Ship,62,56,-90,14,14,1};
    b.sprites[2]={Games::Board::Sprite::Shot,64,57,0,2,4,3};
    display.invalidate();display.renderGame(b,4,false);
    for(int frame=0;frame<60;frame++) {
      b.sprites[0].x=frame-12;b.sprites[0].y=frame*3-15;
      b.sprites[1].angle=frame*11;b.sprites[1].x=frame+57;b.sprites[1].y=frame+50;
      b.spriteCount=frame%3?3:2;
      display.renderGame(b,4,false);auto incremental=MockTFT::pixels;
      display.invalidate();display.renderGame(b,4,false);assert(incremental==MockTFT::pixels);
    }
  }
  std::cout<<"Game renderer passed: incremental pixels equal full composition for all ten games in both rotations; no direct viewport erase; unchanged frames/static bricks untouched; overlap/rotation/clipping/removal; partial patch stride\n";
}
