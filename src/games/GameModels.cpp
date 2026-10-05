#include "GameModels.h"
#include <algorithm>
namespace Games {
void Snake::reset(uint32_t seed) {
  random.state=seed;length=4;dx=nextDx=1;dy=nextDy=0;over=won=turned=false;
  int start=(Height/2)*Width+Width/2;
  for(int i=0;i<length;i++)body[i]=start-i;
  placeFood();
}
void Snake::placeFood() {
  if(length==Capacity) { won=over=true; return; }
  int start=random.next()%Capacity;
  for(int n=0;n<Capacity;n++) {
    int candidate=(start+n)%Capacity;bool occupied=false;
    for(int i=0;i<length;i++)if(body[i]==candidate) { occupied=true;break; }
    if(!occupied) { food=candidate;return; }
  }
}
void Snake::direction(int xx,int yy) {
  if(over||turned||(xx==-dx&&yy==-dy)||(xx==dx&&yy==dy))return;
  if((xx==0&& (yy==1||yy==-1))||(yy==0&&(xx==1||xx==-1))) {
    nextDx=xx;nextDy=yy;turned=true;
  }
}
void Snake::step() {
  if(over)return;
  dx=nextDx;dy=nextDy;turned=false;
  int xx=body[0]%Width+dx,yy=body[0]/Width+dy;
  if(xx<0||xx>=Width||yy<0||yy>=Height) { over=true;return; }
  int next=yy*Width+xx;bool growing=next==food;
  for(int i=0;i<length-(growing?0:1);i++)if(body[i]==next) { over=true;return; }
  if(growing)length++;
  for(int i=length-1;i>0;i--)body[i]=body[i-1];
  body[0]=next;
  if(growing)placeFood();
}
void Snake::board(Board& v)const {
  v={};v.kind=Board::Snake;v.width=Width;v.height=Height;v.over=over;v.won=won;v.score=(length-4)*10;
  if(!won)v.cells[food]=3;
  for(int i=length-1;i>=0;i--)v.cells[body[i]]=i?1:2;
}
void Minesweeper::reset(uint32_t seed) {
  random.state=seed;mines.fill(false);revealed.fill(false);flags.fill(false);cursor=40;planted=over=won=false;
}
void Minesweeper::move(int dx,int dy) {
  int x=std::clamp(cursor%Width+dx,0,Width-1),y=std::clamp(cursor/Width+dy,0,Height-1);
  cursor=y*Width+x;
}
void Minesweeper::plant() {
  int placed=0;
  while(placed<Bombs) {
    int pos=random.next()%Count;
    if(mines[pos]||(std::abs(pos%Width-cursor%Width)<=1&&std::abs(pos/Width-cursor/Width)<=1))continue;
    mines[pos]=true;placed++;
  }
  planted=true;
}
int Minesweeper::adjacent(int pos)const {
  int total=0,x=pos%Width,y=pos/Width;
  for(int yy=std::max(0,y-1);yy<=std::min(Height-1,y+1);yy++)
    for(int xx=std::max(0,x-1);xx<=std::min(Width-1,x+1);xx++)if(mines[yy*Width+xx])total++;
  return total;
}
void Minesweeper::reveal() {
  if(over||flags[cursor]||revealed[cursor])return;
  if(!planted)plant();
  if(mines[cursor]) { revealed[cursor]=true;over=true;return; }
  std::array<int,Count> pending{};int head=0,tail=0;
  pending[tail++]=cursor;revealed[cursor]=true;
  while(head<tail) {
    int pos=pending[head++];if(adjacent(pos))continue;
    int x=pos%Width,y=pos/Width;
    for(int yy=std::max(0,y-1);yy<=std::min(Height-1,y+1);yy++)
      for(int xx=std::max(0,x-1);xx<=std::min(Width-1,x+1);xx++) {
        int n=yy*Width+xx;
        if(!revealed[n]&&!flags[n]&&!mines[n]) { revealed[n]=true;pending[tail++]=n; }
      }
  }
  int safe=0;for(int i=0;i<Count;i++)if(revealed[i]&&!mines[i])safe++;
  if(safe==Count-Bombs)won=over=true;
}
void Minesweeper::flag() { if(!over&&!revealed[cursor])flags[cursor]=!flags[cursor]; }
void Minesweeper::board(Board& v)const {
  v={};v.kind=Board::Minesweeper;v.width=Width;v.height=Height;v.over=over;v.won=won;v.cursor=cursor;
  int used=0;
  for(int i=0;i<Count;i++) {
    if(flags[i])used++;
    v.cells[i]=over&&mines[i]?12:revealed[i]?2+adjacent(i):flags[i]?1:0;
    if(revealed[i]&&!mines[i])v.score++;
  }
  v.detail=Bombs-used;
}
static constexpr uint16_t pieces[]={0x00F0,0x0066,0x0072,0x0071,0x0074,0x0036,0x0063};
bool Tetris::tile(int p,int rot,int col,int row) {
  if(p<0||p>=7||col<0||col>3||row<0||row>3)return false;
  if(p==1)rot=0;
  for(int n=0;n<(rot&3);n++) { int old=col;col=row;row=3-old; }
  return pieces[p]&(1<<(row*4+col));
}
void Tetris::reset(uint32_t seed) {
  random.state=seed;cells.fill(0);lines=score=0;over=false;next=random.next()%7;spawn();
}
void Tetris::spawn() {
  piece=next;next=random.next()%7;rotation=0;x=3;y=0;
  if(!fits(x,y,rotation))over=true;
}
bool Tetris::fits(int xx,int yy,int rot)const {
  for(int row=0;row<4;row++)for(int col=0;col<4;col++)if(tile(piece,rot,col,row)) {
    int tx=xx+col,ty=yy+row;
    if(tx<0||tx>=Width||ty<0||ty>=Height||cells[ty*Width+tx])return false;
  }
  return true;
}
bool Tetris::move(int dx,int dy) {
  if(over||!fits(x+dx,y+dy,rotation))return false;
  x+=dx;y+=dy;return true;
}
void Tetris::rotate() {
  if(over)return;
  for(int kick:{0,-1,1,-2,2})if(fits(x+kick,y,(rotation+1)&3)) { x+=kick;rotation=(rotation+1)&3;return; }
}
void Tetris::lock() {
  for(int row=0;row<4;row++)for(int col=0;col<4;col++)if(tile(piece,rotation,col,row))cells[(y+row)*Width+x+col]=piece+1;
  int cleared=0;
  for(int row=Height-1;row>=0;) {
    bool full=true;for(int col=0;col<Width;col++)if(!cells[row*Width+col])full=false;
    if(!full) { row--;continue; }
    for(int r=row;r>0;r--)for(int c=0;c<Width;c++)cells[r*Width+c]=cells[(r-1)*Width+c];
    for(int c=0;c<Width;c++)cells[c]=0;
    cleared++;
  }
  lines+=cleared;score+=100*cleared*cleared;spawn();
}
void Tetris::tick() { if(!over&&!move(0,1))lock(); }
void Tetris::drop() { if(over)return;while(move(0,1))score+=2;lock(); }
void Tetris::board(Board& v)const {
  v={};v.kind=Board::Tetris;v.width=Width;v.height=Height;v.cells.fill(0);v.score=score;v.detail=lines;v.over=over;
  std::copy(cells.begin(),cells.end(),v.cells.begin());
  if(!over)for(int row=0;row<4;row++)for(int col=0;col<4;col++)if(tile(piece,rotation,col,row))v.cells[(y+row)*Width+x+col]=piece+1;
}
}
