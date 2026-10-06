#include "../src/games/ArcadeModels.h"
#include <cassert>
#include <cmath>
#include <iostream>
using namespace Games;
int main() {
  Puzzle2048 p;p.reset(1);int occupied=0;for(auto n:p.cells)occupied+=n!=0;assert(occupied==2);
  p.cells.fill(0);p.cells[0]=p.cells[1]=p.cells[2]=p.cells[3]=1;p.score=0;
  assert(p.move(-1,0)&&p.cells[0]==2&&p.cells[1]==2&&p.score==8);
  p.cells.fill(0);p.cells[0]=p.cells[1]=2;p.cells[2]=3;p.score=0;
  assert(p.move(-1,0)&&p.cells[0]==3&&p.cells[1]==3&&p.score==8);
  p.cells.fill(0);p.cells[0]=1;auto before=p.cells;assert(!p.move(-1,0)&&before==p.cells);
  assert(!p.move(1,1)&&!p.move(0,0));
  p.cells.fill(0);p.cells[0]=p.cells[4]=10;assert(p.move(0,-1)&&p.won&&p.over&&p.cells[0]==11);
  p.reset(1);for(int y=0;y<4;y++)for(int x=0;x<4;x++)p.cells[y*4+x]=1+(x+y)%2;
  assert(!p.move(1,0)&&p.over&&!p.won);
  p.reset(2);p.cells.fill(0);p.cells[0]=p.cells[1]=1;assert(p.move(1,0)&&p.cells[3]==2);
  for(int seed=0;seed<100;seed++) { p.reset(seed);for(int n=0;n<300&&!p.over;n++) { int d=n%4;p.move(d==0?-1:d==1?1:0,d==2?-1:d==3?1:0);for(auto c:p.cells)assert(c<=11); } }
  Arcade a;Controls none;
  a.reset(Board::Pong,1);a.ball={80,2,0,-2,3,true};a.step(none);assert(a.ball.vy>0);
  a.ball={12,90,-3,0,3,true};a.step(none);assert(a.ball.vx>0);
  a.ball={159,90,3,0,3,true};a.step(none);assert(a.score==1);
  a.score=4;a.ball={159,90,3,0,3,true};a.step(none);assert(a.over&&a.won);
  a.reset(Board::Pong,1);a.opponent=4;a.ball={1,5,-3,0,3,true};a.step(none);assert(a.over&&!a.won);
  a.reset(Board::Breakout,1);a.ball={80,160,0,3,3,true};a.step(none);assert(a.ball.vy<0);
  a.ball={10,10,0,6,3,true};a.step(none);assert(a.bricks[0]==0&&a.score==10);
  a.ball={80,183,0,3,3,true};a.step(none);assert(a.lives==2&&!a.over);
  a.lives=1;a.ball={80,183,0,3,3,true};a.step(none);assert(a.over);
  a.reset(Board::Breakout,1);a.bricks.fill(0);a.bricks[0]=1;a.ball={10,10,0,6,3,true};a.step(none);assert(a.over&&a.won);
  a.reset(Board::FlappyBird,1);a.action();assert(a.ball.vy<0);a.step(none);assert(a.ball.y<90);
  a.reset(Board::FlappyBird,1);a.obstacles[0]={23,90,0,0,0,true};a.step(none);assert(a.score==1&&!a.over);
  a.ball.y=20;a.obstacles[0].x=40;a.step(none);assert(a.over);
  a.reset(Board::DinoRunner,1);a.action();a.step(none);assert(a.ball.y<154);
  for(auto& o:a.obstacles)o.x=1000;
  for(int i=0;i<40;i++)a.step(none);
  assert(a.ball.y==154);
  a.reset(Board::DinoRunner,1);a.obstacles[0]={26,0,0,0,1,true};Controls duck;duck.y=1;a.step(duck);assert(!a.over);
  a.reset(Board::DinoRunner,1);a.obstacles[0]={26,0,0,0,1,true};a.step(none);assert(a.over);
  a.reset(Board::DinoRunner,1);a.obstacles[0]={26,0,0,0,0,true};a.step(duck);assert(a.over);
  a.reset(Board::SpaceInvaders,1);Controls fire;fire.fire=true;a.step(fire);assert(a.bullets[0].active&&a.bullets[0].vy<0);
  a.bullets.fill(Body{});a.bullets[0]={21,36,0,-5,2,true};a.step(none);assert(!a.aliens[0]&&a.score==10);
  a.bullets[0]={80,157,0,3,2,true};a.step(none);assert(a.lives==2&&a.invulnerable>0);
  a.aliens.fill(false);a.step(none);assert(a.level==2&&a.aliens[0]);
  a.enemyY=150;a.step(none);assert(a.over);
  a.reset(Board::Asteroids,1);Controls thrust;thrust.thrust=true;a.step(thrust);assert(a.ship.vy<0&&a.ship.y<90);
  a.rocks.fill(Body{});a.rocks[0]={30,30,0,0,14,true};a.bullets[0]={30,30,0,0,2,true};a.step(none);
  occupied=0;for(auto r:a.rocks)occupied+=r.active;assert(occupied==2&&a.rocks[0].size==7&&a.score==20);
  a.ship.x=159.9f;a.ship.vx=2;a.step(none);assert(a.ship.x>=0&&a.ship.x<160);
  a.rocks.fill(Body{});a.step(none);assert(a.level==2);
  a.rocks.fill(Body{});a.rocks[0]={80,90,0,0,14,true};a.ship={80,90,0,0,5,true};a.invulnerable=0;a.lives=1;a.step(none);assert(a.over);
  for(int kind=4;kind<10;kind++) {
    a.reset(static_cast<Board::Kind>(kind),42);
    for(int frame=0;frame<2000;frame++) {
      if(a.over)a.reset(static_cast<Board::Kind>(kind),frame+1);
      Controls c;c.x=frame%60<30?-1:1;c.y=frame%30<15?-1:1;c.fire=frame%3==0;c.thrust=frame%4==0;
      if(frame%12==0)a.action();
      a.step(c);Board b;a.board(b);
      assert(b.spriteCount<=64&&std::isfinite(a.ship.x)&&std::isfinite(a.ball.y));
      for(int i=0;i<b.spriteCount;i++)assert(b.sprites[i].w>0&&b.sprites[i].h>0);
    }
  }
  std::cout<<"Arcade tests passed: 2048 merges/win/no-op/game-over, Pong goals/bounce, bricks/lives/win, pipes/scoring, jump/duck/collision, aliens/waves/lives, asteroid inertia/split/wrap, deterministic stress\n";
}
