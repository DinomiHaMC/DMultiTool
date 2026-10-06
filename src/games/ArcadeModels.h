#pragma once
#include "GameModels.h"
namespace Games {
const char* title(Board::Kind kind);
const char* controls(Board::Kind kind);
class Puzzle2048 {
  void spawn();
public:
  std::array<uint8_t,16> cells{};
  uint32_t score=0;bool over=false,won=false;Random random;
  void reset(uint32_t seed);
  bool move(int dx,int dy);
  void board(Board& view)const;
};
struct Body { float x=0,y=0,vx=0,vy=0;int size=0;bool active=false; };
struct Controls { int x=0,y=0;bool fire=false,thrust=false; };
class Arcade {
  void serve();
  void wave();
  void shot(float x,float y,float vx,float vy);
  void loseLife();
public:
  Board::Kind kind=Board::Pong;
  Random random;
  uint32_t score=0,frame=0;int lives=3,opponent=0,level=1;
  bool over=false,won=false;
  float player=90,ai=90,shipX=80,shipY=90,angle=-90;
  Body ball,ship;
  std::array<Body,12> bullets{};
  std::array<Body,10> rocks{};
  std::array<Body,3> obstacles{};
  std::array<uint8_t,40> bricks{};
  std::array<bool,18> aliens{};
  float enemyX=15,enemyY=25;int enemyDirection=1,shotCooldown=0,invulnerable=0;
  void reset(Board::Kind choice,uint32_t seed);
  void action();
  void step(const Controls& input);
  void board(Board& view)const;
};
}
