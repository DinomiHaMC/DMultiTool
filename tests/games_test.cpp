#include "../src/games/GameModels.h"
#include <cassert>
#include <iostream>
int main(){
 Games::Snake snake;snake.reset(1);int head=snake.body[0];snake.direction(-1,0);snake.step();assert(snake.body[0]==head+1);
 snake.food=snake.body[0]+1;snake.step();assert(snake.length==5);
 for(int i=0;i<30;i++){snake.step();}
 assert(snake.over);
 for(uint32_t seed=0;seed<100;seed++){
  Games::Minesweeper mines;mines.reset(seed);mines.reveal();assert(!mines.over||mines.won);assert(mines.adjacent(40)==0);
  int count=0;for(bool mine:mines.mines)count+=mine;assert(count==10);
  for(int i=0;i<81;i++){if(!mines.mines[i]){mines.cursor=i;mines.reveal();}}
  assert(mines.won);
 }
 Games::Tetris t;t.reset(1);
 for(int p=0;p<7;p++)for(int r=0;r<4;r++){int tiles=0;for(int y=0;y<4;y++)for(int x=0;x<4;x++)tiles+=Games::Tetris::tile(p,r,x,y);assert(tiles==4);}
 t.cells.fill(0);for(int x=0;x<10;x++)if(x<3||x>6)t.cells[190+x]=1;
 t.piece=0;t.rotation=0;t.x=3;t.y=0;t.drop();assert(t.lines==1);for(int x=0;x<10;x++)assert(t.cells[190+x]==0);
 for(int i=0;i<100&&!t.over;i++){t.rotate();t.move(-1,0);t.drop();}assert(t.over);
 std::cout<<"Game tests passed: Snake collision/growth, first-safe mines/win, tetrominoes/line-clear/game-over\n";
}
