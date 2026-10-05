#pragma once
#include <array>
#include <cstdint>
namespace Games {
struct Random {
  uint32_t state=1;
  uint32_t next() { state=state*1664525u+1013904223u; return state; }
};
struct Board {
  enum Kind { Snake, Minesweeper, Tetris } kind=Snake;
  uint8_t width=0,height=0;
  std::array<uint8_t,400> cells{};
  uint32_t score=0;
  int detail=0,cursor=-1;
  bool over=false,won=false;
};
class Snake {
public:
  static constexpr int Width=18,Height=20,Capacity=Width*Height;
  std::array<uint16_t,Capacity> body{};
  int length=4,food=0,dx=1,dy=0,nextDx=1,nextDy=0;
  bool over=false,won=false,turned=false;
  Random random;
  void reset(uint32_t seed);
  void direction(int x,int y);
  void step();
  void board(Board& view)const;
private:
  void placeFood();
};
class Minesweeper {
public:
  static constexpr int Width=9,Height=9,Count=81,Bombs=10;
  std::array<bool,Count> mines{},revealed{},flags{};
  int cursor=40;
  bool planted=false,over=false,won=false;
  Random random;
  void reset(uint32_t seed);
  void move(int dx,int dy);
  void reveal();
  void flag();
  int adjacent(int index)const;
  void board(Board& view)const;
private:
  void plant();
};
class Tetris {
public:
  static constexpr int Width=10,Height=20;
  std::array<uint8_t,Width*Height> cells{};
  int piece=0,next=0,rotation=0,x=3,y=0,lines=0;
  uint32_t score=0;
  bool over=false;
  Random random;
  void reset(uint32_t seed);
  static bool tile(int piece,int rotation,int col,int row);
  bool fits(int xx,int yy,int rot)const;
  bool move(int dx,int dy);
  void rotate();
  void drop();
  void tick();
  void board(Board& view)const;
private:
  void spawn();
  void lock();
};
}
