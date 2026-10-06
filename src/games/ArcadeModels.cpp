#include "ArcadeModels.h"
#include <algorithm>
#include <cmath>
namespace Games {
namespace {
constexpr float Pi=3.14159265358979323846f;
bool hit(float x,float y,float radius,float left,float top,float w,float h) {
  float dx=x-std::max(left,std::min(x,left+w)),dy=y-std::max(top,std::min(y,top+h));
  return dx*dx+dy*dy<=radius*radius;
}
float clamp(float x,float low,float high) { return std::max(low,std::min(x,high)); }
float wrap(float x,float max) { return x<0?x+max:x>=max?x-max:x; }
}
const char* title(Board::Kind kind) {
  static const char* names[]={"Snake","Minesweeper","Tetris","2048","Pong","Breakout","FlappyBird","Dino Runner","Space Invaders","Asteroids"};
  return names[static_cast<unsigned>(kind)%10];
}
const char* controls(Board::Kind kind) {
  switch(kind) {
    case Board::Snake:return "Arrows steer  OK pause";
    case Board::Minesweeper:return "OK open  2xOK flag";
    case Board::Tetris:return "UP rotate DOWN fall OK drop";
    case Board::Puzzle2048:return "Arrows slide | Hold OK exit";
    case Board::Pong:return "UP/DOWN paddle  OK pause";
    case Board::Breakout:return "LEFT/RIGHT paddle  OK pause";
    case Board::FlappyBird:return "UP/OK flap";
    case Board::DinoRunner:return "UP/OK jump  DOWN duck";
    case Board::SpaceInvaders:return "LEFT/RIGHT move  UP fire";
    default:return "LEFT/RIGHT turn UP thrust DOWN fire";
  }
}
void Puzzle2048::spawn() {
  std::array<int,16> empty{};int count=0;
  for(int i=0;i<16;i++)if(!cells[i])empty[count++]=i;
  if(count)cells[empty[(random.next()>>8)%count]]=(random.next()>>8)%10?1:2;
}
void Puzzle2048::reset(uint32_t seed) { cells.fill(0);random.state=seed;score=0;over=won=false;spawn();spawn(); }
bool Puzzle2048::move(int dx,int dy) {
  if(over||(dx==0)==(dy==0)||std::abs(dx)+std::abs(dy)!=1)return false;
  auto before=cells;
  for(int line=0;line<4;line++) {
    std::array<int,4> indices{},packed{};int count=0;
    for(int i=0;i<4;i++) {
      int x=dx? (dx<0?i:3-i):line,y=dy?(dy<0?i:3-i):line;
      indices[i]=y*4+x;if(cells[indices[i]])packed[count++]=cells[indices[i]];
    }
    std::array<uint8_t,4> result{};int out=0;
    for(int i=0;i<count;i++) {
      int value=packed[i];if(i+1<count&&packed[i+1]==value) { value++;i++;score+=1u<<value;if(value>=11)won=true; }
      result[out++]=value;
    }
    for(int i=0;i<4;i++)cells[indices[i]]=result[i];
  }
  bool changed=before!=cells;if(changed)spawn();
  bool available=false;
  for(int i=0;i<16;i++)if(!cells[i]||(i%4<3&&cells[i]==cells[i+1])||(i<12&&cells[i]==cells[i+4]))available=true;
  over=won||!available;return changed;
}
void Puzzle2048::board(Board& view)const {
  view=Board{};view.kind=Board::Puzzle2048;view.width=view.height=4;
  std::copy(cells.begin(),cells.end(),view.cells.begin());view.score=score;view.over=over;view.won=won;
}
void Arcade::serve() {
  if(kind==Board::Pong)ball={80,90,(random.next()&1)?2.3f:-2.3f,1.1f,3,true};
  else if(kind==Board::Breakout)ball={player,145,1.4f,-2.4f,3,true};
}
void Arcade::wave() {
  if(kind==Board::SpaceInvaders) { aliens.fill(true);enemyX=15;enemyY=25;enemyDirection=1; }
  else {
    for(auto& r:rocks)r=Body{};
    for(int i=0;i<std::min(3+level,8);i++) {
      auto& r=rocks[i];r={float((random.next()>>8)%160),float((random.next()>>8)%35),float(int((random.next()>>8)%21)-10)/12,.4f+float(i%3)/5,14,true};
    }
  }
}
void Arcade::reset(Board::Kind choice,uint32_t seed) {
  *this=Arcade{};kind=choice;random.state=seed;player=kind==Board::Pong?90:80;
  shipX=80;shipY=90;ship={80,90,0,0,5,true};angle=-90;
  bricks.fill(1);serve();
  if(kind==Board::FlappyBird) {
    ball={40,90,0,0,5,true};for(int i=0;i<3;i++)obstacles[i]={float(180+i*75),float(50+(random.next()>>8)%75),0,0,0,true};
  } else if(kind==Board::DinoRunner) {
    ball={30,154,0,0,7,true};for(int i=0;i<3;i++)obstacles[i]={float(210+i*100),0,0,0,i==1?1:0,true};
  } else if(kind==Board::SpaceInvaders||kind==Board::Asteroids)wave();
}
void Arcade::action() {
  if(over)return;
  if(kind==Board::FlappyBird)ball.vy=-3.1f;
  else if(kind==Board::DinoRunner&&ball.y>=154)ball.vy=-5.2f;
}
void Arcade::shot(float x,float y,float vx,float vy) {
  for(auto& b:bullets)if(!b.active) { b={x,y,vx,vy,2,true};return; }
}
void Arcade::loseLife() {
  if(--lives<=0) { over=true;return; }
  if(kind==Board::Asteroids) { ship={80,90,0,0,5,true};angle=-90;invulnerable=45; }
  else if(kind==Board::Breakout)serve();
}
void Arcade::step(const Controls& input) {
  if(over)return;
  frame++;if(shotCooldown)shotCooldown--;if(invulnerable)invulnerable--;
  if(kind==Board::Pong) {
    player=clamp(player+input.y*3.5f,16,164);
    ai=clamp(ai+clamp(ball.y-ai,-1.8f,1.8f),16,164);
    ball.x+=ball.vx;ball.y+=ball.vy;
    if(ball.y<3||ball.y>177) { ball.y=clamp(ball.y,3,177);ball.vy=-ball.vy; }
    if(ball.vx<0&&hit(ball.x,ball.y,3,5,player-16,4,32)) { ball.x=12;ball.vx=std::min(4.5f,-ball.vx+.12f);ball.vy=(ball.y-player)*.14f; }
    if(ball.vx>0&&hit(ball.x,ball.y,3,151,ai-16,4,32)) { ball.x=148;ball.vx=-ball.vx;ball.vy=(ball.y-ai)*.14f; }
    if(ball.x<0||ball.x>160) {
      if(ball.x>160)score++;else opponent++;
      if(score>=5||opponent>=5) { over=true;won=score>=5; }else serve();
    }
  } else if(kind==Board::Breakout) {
    player=clamp(player+input.x*3.5f,17,143);float oldY=ball.y;
    ball.x+=ball.vx;ball.y+=ball.vy;
    if(ball.x<3||ball.x>157) { ball.x=clamp(ball.x,3,157);ball.vx=-ball.vx; }
    if(ball.y<3) { ball.y=3;ball.vy=-ball.vy; }
    if(ball.vy>0&&hit(ball.x,ball.y,3,player-17,161,34,5)) { ball.y=157;ball.vy=-2.6f;ball.vx=(ball.x-player)*.12f; }
    for(int i=0;i<40;i++)if(bricks[i]&&hit(ball.x,ball.y,3,5+(i%8)*19,15+(i/8)*9,17,7)) {
      bricks[i]=0;score+=10;
      if(oldY<15+(i/8)*9||oldY>22+(i/8)*9)ball.vy=-ball.vy;else ball.vx=-ball.vx;
      break;
    }
    if(ball.y>184)loseLife();
    if(std::none_of(bricks.begin(),bricks.end(),[](uint8_t b){return b!=0;}))over=won=true;
  } else if(kind==Board::FlappyBird) {
    ball.vy+=.22f;ball.y+=ball.vy;
    if(ball.y<5||ball.y>175)over=true;
    for(auto& p:obstacles) {
      float before=p.x;p.x-=1.65f;
      if(before+18>=40&&p.x+18<40)score++;
      if(p.x<-18) { p.x+=225;p.y=50+(random.next()>>8)%75; }
      if(hit(ball.x,ball.y,5,p.x,0,18,p.y-27)||hit(ball.x,ball.y,5,p.x,p.y+27,18,180-p.y-27))over=true;
    }
  } else if(kind==Board::DinoRunner) {
    ball.vy+=.3f;ball.y=std::min(154.f,ball.y+ball.vy);if(ball.y==154)ball.vy=0;
    bool duck=input.y>0&&ball.y==154;
    for(auto& p:obstacles) {
      p.x-=2.f+std::min(2.f,frame/1000.f);
      if(p.x<-16) { p.x+=300;p.size=(random.next()>>8)%3==0?1:0;score++; }
      float top=p.size?132:137,height=p.size?10:18;
      if(hit(p.x+6,top+height/2,5,23,ball.y-(duck?6:18),duck?18:12,duck?6:18))over=true;
    }
    opponent=duck?1:0;
  } else if(kind==Board::SpaceInvaders) {
    player=clamp(player+input.x*3,8,152);
    if(input.fire&&!shotCooldown) { shot(player,158,0,-5);shotCooldown=6; }
    enemyX+=enemyDirection*(.3f+std::min(level,10)*.06f);
    float left=160,right=0;bool any=false;
    for(int i=0;i<18;i++)if(aliens[i]) { any=true;left=std::min(left,enemyX+(i%6)*21);right=std::max(right,enemyX+(i%6)*21+12);if(enemyY+(i/6)*18+10>=155)over=true; }
    if(left<2||right>158) { enemyDirection=-enemyDirection;enemyY+=7; }
    if(any&&frame%std::max(10,35-level*2)==0) {
      int index=(random.next()>>8)%18;for(int n=0;n<18;n++,index=(index+1)%18)if(aliens[index]) { shot(enemyX+(index%6)*21+6,enemyY+(index/6)*18+12,0,2.5f);break; }
    }
    for(auto& b:bullets)if(b.active) {
      b.x+=b.vx;b.y+=b.vy;if(b.y<0||b.y>180)b.active=false;
      if(b.active&&b.vy<0)for(int i=0;i<18;i++)if(aliens[i]&&hit(b.x,b.y,2,enemyX+(i%6)*21,enemyY+(i/6)*18,12,10)) { aliens[i]=false;b.active=false;score+=10;break; }
      if(b.active&&b.vy>0&&!invulnerable&&hit(b.x,b.y,2,player-7,158,14,10)) { b.active=false;loseLife();invulnerable=25; }
    }
    if(!over&&std::none_of(aliens.begin(),aliens.end(),[](bool a){return a;})) { level++;bullets.fill(Body{});wave(); }
  } else if(kind==Board::Asteroids) {
    angle+=input.x*8;angle=angle<-180?angle+360:angle>180?angle-360:angle;
    float radians=angle*Pi/180;
    if(input.thrust) { ship.vx+=std::cos(radians)*.12f;ship.vy+=std::sin(radians)*.12f; }
    ship.vx=clamp(ship.vx*.995f,-3,3);ship.vy=clamp(ship.vy*.995f,-3,3);
    ship.x=wrap(ship.x+ship.vx,160);ship.y=wrap(ship.y+ship.vy,180);
    if(input.fire&&!shotCooldown) { shot(ship.x+std::cos(radians)*8,ship.y+std::sin(radians)*8,ship.vx+std::cos(radians)*4,ship.vy+std::sin(radians)*4);shotCooldown=5; }
    for(auto& b:bullets)if(b.active) { b.x+=b.vx;b.y+=b.vy;if(b.x<0||b.x>160||b.y<0||b.y>180)b.active=false; }
    for(auto& r:rocks)if(r.active) {
      r.x=wrap(r.x+r.vx,160);r.y=wrap(r.y+r.vy,180);
      for(auto& b:bullets)if(b.active) {
        float dx=b.x-r.x,dy=b.y-r.y;
        if(dx*dx+dy*dy<(r.size+2)*(r.size+2)) {
          b.active=false;score+=r.size>7?20:50;
          if(r.size>7) {
            r.size=7;r.vx=-r.vx-.4f;r.vy+=.5f;
            for(auto& child:rocks)if(!child.active) { child={r.x,r.y,-r.vx,-r.vy,7,true};break; }
          } else r.active=false;
          break;
        }
      }
      if(r.active&&!invulnerable) { float dx=ship.x-r.x,dy=ship.y-r.y;if(dx*dx+dy*dy<(r.size+5)*(r.size+5))loseLife(); }
    }
    if(!over&&std::none_of(rocks.begin(),rocks.end(),[](const Body& r){return r.active;})) { level++;wave(); }
  }
}
void Arcade::board(Board& view)const {
  view=Board{};view.kind=kind;view.width=160;view.height=180;view.score=score;view.over=over;view.won=won;
  view.detail=kind==Board::Pong?opponent:kind==Board::FlappyBird||kind==Board::DinoRunner?int(frame/20):lives;
  auto add=[&](Board::Sprite::Type type,float x,float y,int w,int h,int color=1,int direction=0) {
    if(view.spriteCount<view.sprites.size())view.sprites[view.spriteCount++]={type,int16_t(x),int16_t(y),int16_t(direction),uint8_t(w),uint8_t(h),uint8_t(color)};
  };
  if(kind==Board::Pong) {
    add(Board::Sprite::Rect,5,player-16,4,32);add(Board::Sprite::Rect,151,ai-16,4,32,2);add(Board::Sprite::Ball,ball.x-3,ball.y-3,6,6,3);
  } else if(kind==Board::Breakout) {
    for(int i=0;i<40;i++)if(bricks[i])add(Board::Sprite::Rect,5+(i%8)*19,15+(i/8)*9,17,7,2+i/8);
    add(Board::Sprite::Rect,player-17,161,34,5);add(Board::Sprite::Ball,ball.x-3,ball.y-3,6,6,3);
  } else if(kind==Board::FlappyBird) {
    for(const auto& p:obstacles) { add(Board::Sprite::Rect,p.x,0,18,int(p.y-27),2);add(Board::Sprite::Rect,p.x,p.y+27,18,int(180-p.y-27),2); }
    add(Board::Sprite::Bird,ball.x-5,ball.y-5,10,10,3);
  } else if(kind==Board::DinoRunner) {
    add(Board::Sprite::Rect,0,155,160,1,2);
    for(const auto& p:obstacles)add(p.size?Board::Sprite::Bird:Board::Sprite::Rect,p.x,p.size?132:137,12,p.size?10:18,p.size?3:2);
    add(Board::Sprite::Dino,23,ball.y-(opponent?6:18),opponent?18:12,opponent?6:18,1);
  } else if(kind==Board::SpaceInvaders) {
    for(int i=0;i<18;i++)if(aliens[i])add(Board::Sprite::Alien,enemyX+(i%6)*21,enemyY+(i/6)*18,12,10,2+i/6);
    if(!invulnerable||frame%4<2)add(Board::Sprite::Ship,player-7,158,14,10,1,-90);
  } else {
    for(const auto& r:rocks)if(r.active)add(Board::Sprite::Rock,r.x-r.size,r.y-r.size,r.size*2,r.size*2,2);
    if(!invulnerable||frame%4<2)add(Board::Sprite::Ship,ship.x-7,ship.y-7,14,14,1,int(angle));
  }
  for(const auto& b:bullets)if(b.active)add(Board::Sprite::Shot,b.x-1,b.y-2,2,4,b.vy>0&&kind==Board::SpaceInvaders?7:3);
}
}
