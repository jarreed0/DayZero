#if __has_include(<SDL2/SDL.h>)
 #include <SDL2/SDL.h>
 #include <SDL2/SDL_image.h>
 #include <SDL2/SDL_ttf.h>
#else
 #include <SDL.h>
 #include <SDL_image.h>
 #include <SDL_ttf.h>
#endif

#include <iostream>

#include <vector>
#include <cmath>

const char * TITLE = "Game";

#define PI 3.14159265359

int setFPS = 120;

int WIDTH = 1800;
int HEIGHT = 900;
int flags = SDL_WINDOW_FULLSCREEN; //not used yet
SDL_Renderer* renderer;
SDL_Window* window;
TTF_Font *font;
SDL_Color font_color;
int font_size;

#define TOP 1
#define WALL 2
#define FLOOR 3
#define TREE 4
#define GATE 5

int frameCount, timerFPS, lastFrame, fps;

bool running = 1;
#define PLAYING 1;
#define PAUSED 2;
#define MAIN 3;
#define SAVING 4;
#define LOADING 5;
#define EXITING 6;
int gamestate = PLAYING;
int seed = time(NULL);

int offsetX, offsetY;
bool up, down, right, left;

int map_width = 150; //500;
int map_height = 150; //500;
int tile_size = 75;

int tilesImgId;

SDL_Color setColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a) { return SDL_Color {r, g, b, a}; }
SDL_Color setColor(Uint8 r, Uint8 g, Uint8 b) { return setColor(r, g, b, 255); }
SDL_Color white = setColor(255, 255, 255, 255);
SDL_Color black = setColor(0, 0, 0, 255);
SDL_Color wallColor = setColor(94, 120, 140);
SDL_Color floorColor = setColor(190, 216, 239);
SDL_Color bkg;
void setBkg(Uint8 r, Uint8 g, Uint8 b) {
 bkg = setColor(r, g, b);
}

struct obj {
 SDL_Point coord;
 SDL_Point center;
 SDL_Rect dest, src;
 double angle = 0;
 double vel = 0;
 double tick = 0;
 int id = FLOOR;
 int img;
 bool flip = 0;
 bool flipV = 0;
 bool rotateOnCenter = 0;
 int frame;
} player, gun, wolf, cursor;

SDL_Point mouse;

obj treeObj;

obj bulletTmp;
std::vector<obj> bullets;
bool fire = 0;

std::vector<SDL_Texture*> images;
int setImage(std::string filename) {
 images.push_back(IMG_LoadTexture(renderer, filename.c_str()));
 return images.size()-1;
}

SDL_Surface *text_surface;
SDL_Texture *text_texture;
SDL_Rect wrect;
void write(std::string t, int x, int y) {
 const char *text = t.c_str();
 if (font == NULL) {
  fprintf(stderr, "error: font not found\n");
  exit(EXIT_FAILURE);
 }
 text_surface = TTF_RenderText_Solid(font, text, font_color);
 text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
 wrect.w = text_surface->w;
 wrect.h = text_surface->h;
 wrect.x = x-wrect.w;
 wrect.y = y-wrect.h;
 SDL_FreeSurface(text_surface);
 SDL_RenderCopy(renderer, text_texture, NULL, &wrect);
 SDL_DestroyTexture(text_texture);
}

std::vector<obj> map;

void draw(obj* o) {
 //SDL_RenderSetScale(renderer, zoom, zoom); //int zoom = 1
 if(o->flip) {
  if(o->rotateOnCenter) {
   SDL_RenderCopyEx(renderer, images[o->img], &o->src, &o->dest, o->angle, &o->center, SDL_FLIP_HORIZONTAL);
  } else {
   SDL_RenderCopyEx(renderer, images[o->img], &o->src, &o->dest, o->angle, NULL, SDL_FLIP_HORIZONTAL);
  }
 } else if(o->flipV) {
  if(o->rotateOnCenter) {
   SDL_RenderCopyEx(renderer, images[o->img], &o->src, &o->dest, o->angle, &o->center, SDL_FLIP_VERTICAL);
  } else {
   SDL_RenderCopyEx(renderer, images[o->img], &o->src, &o->dest, o->angle, NULL, SDL_FLIP_VERTICAL);
  }
 } else {
  if(o->rotateOnCenter) {
   SDL_RenderCopyEx(renderer, images[o->img], &o->src, &o->dest, o->angle, &o->center, SDL_FLIP_NONE);
  } else {
   SDL_RenderCopyEx(renderer, images[o->img], &o->src, &o->dest, o->angle, NULL, SDL_FLIP_NONE);
  }
 }
}
void draw(std::vector<obj> os) {
 for(auto o : os) {
  draw(&o);
 }
}
void drawRect(SDL_Rect r, SDL_Color c) {
 SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
 SDL_RenderFillRect(renderer, &r);
}
void drawOutline(SDL_Rect r, SDL_Color c) {
 SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
 SDL_RenderDrawRect(renderer, &r);
}
bool inScreen(obj o) {
 if ((WIDTH < (o.dest.x + o.dest.w)) && (0 > o.dest.x) && (0 > o.dest.y) && (HEIGHT < (o.dest.y+o.dest.h))) {
  return true;
 }
 return true;
}
void drawMap() {
 //draw(map);
 SDL_Rect tmp;
 obj tmp2;
 for(auto tile : map) {
  tmp2 = tile;
  tmp2.dest.x -= offsetX;
  tmp2.dest.y -= offsetY;
  tmp2.img = tilesImgId;
  tmp2.src.x=tmp2.src.y=0;
  tmp2.src.w=20;tmp2.src.h=20;
  /*if(inScreen(tile)) {
   if(tile.id == WALL) {
    drawRect(tmp, wallColor);
   } else if(tile.id == TOP) {
    //drawRect(tmp, setColor(51, 73, 95));
    drawRect(tmp, bkg);
   } else if(tile.id == FLOOR || tile.id == TREE) {
    //drawRect(tmp, setColor(50, 143, 200));
    //drawRect(tmp, setColor(tile.tick*50, tile.tick*20, tile.tick*4));
    //drawRect(tmp, setColor(150, 150, 255));
    drawRect(tmp, floorColor);
    //drawRect(tmp, setColor(154, 167, 170));
   } else if(tile.id == GATE) {
    drawRect(tmp, setColor(200, 90, 90));
   } //else if(tile.id == TREE) {
    //drawRect(tmp, setColor(19, 32, 29));
    //drawRect(tmp, setColor(190, 216, 239));
   //}
  }*/
  //if(inScreen(tile)) {
  if(inScreen(tmp2)) {
   if(tile.id == WALL || tile.id == FLOOR || tile.id == TREE || tile.id == TOP) {
    tmp2.src.x = tmp2.frame * tmp2.src.w;
    draw(&tmp2);
   } else if(tile.id == GATE) {
    drawRect(tmp2.dest, setColor(200, 90, 90));
   }
  }
 }
 int count = 0;
 for(auto tile : map) {
  tmp = tile.dest;
  tmp.x -= offsetX;
  tmp.y -= offsetY;
  if(inScreen(tile)) {
   if(tile.id == TREE && map[count+1].id != TOP) {
    treeObj.dest.x = tmp.x + ((count/map_width)%2)*42;
    treeObj.dest.y = tmp.y - (treeObj.dest.h-tile_size) - 3;
    //treeObj.flip = rand() % 2;
    //treeObj.src.x = rand() % 4 * treeObj.src.w;
    treeObj.flip = tile.flip;
    treeObj.src.x = tile.tick * treeObj.src.w;
    //std::cout << tile.frame << std::endl;
    draw(&treeObj);
   }
   /*if(SDL_PointInRect(&mouse, &tmp)) {
    drawRect(tmp, setColor(0, 255, 255, 100));
    //write(std::to_string(tile.id), mouse.x, mouse.y);
    write(std::to_string(gun.angle), mouse.x, mouse.y);
   }*/
  }
  count++;
 }
}

int fpc = 0;
int floorPer() {
 fpc++;
 if(fpc > 8) return 40;
 int c = 0;
 int wc = 0;
 for(auto tile : map) {
  if(tile.id == FLOOR) c++;
  if(tile.id == WALL) wc++;
 }
 if(wc == 0 || ((c*100)/(map.size()+1) + 1) < 30) {
  seed = rand() % time(NULL);
  srand(seed);
  std::cout << "SEED: " << seed << std::endl;
 }
 if(wc == 0) return 0;
 //std::cout << (c*100)/(map.size()+1) + 1 << std::endl;
 return (c*100)/(map.size()+1) + 1;
}

int floodCount = 1;
bool flood(int x, int y, int tick) {
 if(map[y*map_width + x].id == FLOOR && map[y*map_width + x].tick == 0) {
  map[y*map_width + x].tick = tick;
  flood(x-1, y-1, tick);
  flood(x+1, y-1, tick);
  flood(x, y-1, tick);

  flood(x-1, y+1, tick);
  flood(x+1, y+1, tick);
  flood(x, y+1, tick);

  flood(x-1, y, tick);
  flood(x+1, y, tick);
  //if(r) floodCount++;
  return true;
 }
 return false;
}

void plantTree(int x, int y, int t) {
 if(map[y*map_width + x].id == FLOOR and t > 0) {
  map[y*map_width + x].id = TREE;
  map[y*map_width + x].flip = rand() % 2;
  map[y*map_width + x].tick = rand() % 4;
  //map[y*map_width + x].src.x = rand() % 4 * treeObj.src.w;
  plantTree(x-1,y-1,t-1);
  plantTree(x+1,y-1,t-1);
  plantTree(x,y-1,t-1);

  plantTree(x-1,y+1,t-1);
  plantTree(x+1,y+1,t-1);
  plantTree(x,y+1,t-1);

  plantTree(x-1,y,t-1);
  plantTree(x+1,y,t-1);
 }
}

bool spawnSet;
SDL_Point spawn;
void genMap() {
 map.clear();
 obj tile_tmp;
 tile_tmp.dest.w = tile_tmp.dest.h = tile_size;
 for(int y = 0; y < map_height; y++) {
  tile_tmp.dest.y = tile_size * y;
  for(int x = 0; x < map_width; x++) {
   int of = rand() % 100;
   tile_tmp.dest.x = tile_size * x;
   tile_tmp.id = TOP;
   if(of > 40) tile_tmp.id = FLOOR;
   map.push_back(tile_tmp);
  }
 }

 int oc, tc;
 for(int i = 0; i < 6; i++) {
  for(int y = 2; y < map_height-2; y++) {
   for(int x = 2; x < map_width-2; x++) {
    oc = tc = 0;

    if(map[((y-1)*map_width) + (x-1)].id == FLOOR) oc++;
    if(map[((y-1)*map_width) + x].id == FLOOR) oc++;
    if(map[((y-1)*map_width) + (x+1)].id == FLOOR) oc++;
    if(map[(y*map_width) + (x-1)].id == FLOOR) oc++;
    if(map[(y*map_width) + (x+1)].id == FLOOR) oc++;
    if(map[((y+1)*map_width) + (x-1)].id == FLOOR) oc++;
    if(map[((y+1)*map_width) + x].id == FLOOR) oc++;
    if(map[((y+1)*map_width) + (x+1)].id == FLOOR) oc++;

    if(map[((y-2)*map_width) + (x-2)].id == FLOOR) tc++;
    if(map[((y-2)*map_width) + (x-1)].id == FLOOR) tc++;
    if(map[((y-2)*map_width) + (x)].id == FLOOR) tc++;
    if(map[((y-2)*map_width) + (x+1)].id == FLOOR) tc++;
    if(map[((y-2)*map_width) + (x+2)].id == FLOOR) tc++;

    if(map[((y-1)*map_width) + (x-2)].id == FLOOR) tc++;
    if(map[((y)*map_width) + (x-2)].id == FLOOR) tc++;
    if(map[((y+1)*map_width) + (x-2)].id == FLOOR) tc++;

    if(map[((y-1)*map_width) + (x+2)].id == FLOOR) tc++;
    if(map[((y)*map_width) + (x+2)].id == FLOOR) tc++;
    if(map[((y+1)*map_width) + (x+2)].id == FLOOR) tc++;

    if(map[((y+2)*map_width) + (x-2)].id == FLOOR) tc++;
    if(map[((y+2)*map_width) + (x-1)].id == FLOOR) tc++;
    if(map[((y+2)*map_width) + (x)].id == FLOOR) tc++;
    if(map[((y+2)*map_width) + (x+1)].id == FLOOR) tc++;
    if(map[((y+2)*map_width) + (x+2)].id == FLOOR) tc++;

    if(i < 1) {
     if(oc>=5 || tc<=7) {
      map[y*map_width + x].id=FLOOR;
     } else {
      map[y*map_width + x].id=TOP;
     }
    } else {
     if(oc >= 5) {
      map[y*map_width + x].id=FLOOR;
     } else {
      map[y*map_width + x].id=TOP;
     }
    }
   }
  }
 }
 for(int y = 0; y < map_height; y++) {
  map[y*map_width].id = TOP;
  map[y*map_width + 1].id = TOP;
  map[y*map_width + map_width-2].id = TOP;
  map[y*map_width + map_width-1].id = TOP;
  int gt = rand() % 300;
  if(gt == 1) {map[y*map_width + map_width-2].id = GATE; map[y*map_width + map_width-1].id = GATE;}
  if(gt == 2) {map[y*map_width + 1].id = GATE; map[y*map_width + 1].id = GATE;}
 }
 for(int x = 0; x < map_width; x++) {
  map[x].id = TOP;
  map[map_width + x].id = TOP;
  map[x + ((map_width-1)*(map_height-1))].id = TOP;
  map[x + ((map_width)*(map_height-1))].id = TOP;
  int gt = rand() % 300;
  if(gt == 1) {map[x].id = GATE; map[x + ((map_width-1)*(map_height-1))].id = GATE;}
  if(gt == 2) {map[map_width + x].id = GATE; map[x + ((map_width)*(map_height-1))].id = GATE;}
 }

 for(int y = 2; y < map_height-2; y++) {
  for(int x = 2; x < map_width-2; x++) {
   if(flood(x, y, floodCount)) floodCount++;
  }
 }

 int fc=0;
 int maxfc=0;
 for(int i = 0; i < floodCount; i++) {
  int count = 0;
  for(int y = 2; y < map_height-2; y++) {
   for(int x = 2; x < map_width-2; x++) {
    if(map[y*map_width + x].id == FLOOR) {
     if(count > maxfc) {fc=i;maxfc=count;}
     count++;
    }
   }
  }
 }
 for(int y = 2; y < map_height-2; y++) {
  for(int x = 2; x < map_width-2; x++) {
   if(map[y*map_width + x].tick != fc+1) map[y*map_width + x].id = TOP;
  }
 }

 for(int y = 1; y < map_height-2; y++) {
  for(int x = 2; x < map_width-2; x++) {
   if(map[y*map_width + x].id == TOP && map[((y+1)*map_width) + x].id == FLOOR && map[((y-1)*map_width) + x].id == TOP) {
    map[y*map_width + x].id = WALL;
   } else if (map[y*map_width + x].id == TOP && map[((y+1)*map_width) + x].id == FLOOR && map[((y+2)*map_width) + x].id == FLOOR) {
    map[((y+1)*map_width) + x].id = WALL;
   }
  }
 }

 for(int y = 1; y < map_height-2; y++) {
  for(int x = 2; x < map_width-2; x++) {
   int t = rand() % 2000;
   if(map[y*map_width + x].id == FLOOR && t < 4) plantTree(x, y, rand() % 5 + 2);//map[y*map_width + x].id = TREE;
  }
 }
 for(int y = 2; y < map_height-2; y++) {
  for(int x = 2; x < map_width-2; x++) {
   if(!spawnSet) {
    if(map[y*map_width + x].id == FLOOR) {
     if(map[y*map_width + x-1].id == FLOOR && map[y+1*map_width + x].id == FLOOR && map[y+1*map_width + x-1].id == FLOOR) {
      if(rand() % 100 == 1) {
       spawn.x=x;spawn.y=y;
       spawnSet=true;
      }
     }
    }
   }
  }
 }
 for(int y = 0; y < map_height; y++) {
  for(int x = 0; x < map_width; x++) {
   if(map[y*map_width + x].id == FLOOR || map[y*map_width + x].id == TREE) {
    if(y>0 && map[(y-1)*map_width + x].id == WALL) {
     if(y>0 && x>0 && map[(y-1)*map_width + x-1].id != WALL && map[(y-1)*map_width + x-1].id != TOP) {
      map[y*map_width + x].frame = 12;
     } else if(y>0 && x<map_width && map[(y-1)*map_width + x+1].id != WALL && map[(y-1)*map_width + x+1].id != TOP) {
      map[y*map_width + x].frame = 14;
     } else {
      map[y*map_width + x].frame = 13;
     }
    } else {
     map[y*map_width + x].frame = 15;
    }
   } else if(map[y*map_width + x].id == WALL) {
    if(x>0 && map[y*map_width + x-1].id == FLOOR || map[y*map_width + x-1].id == TREE) {
     map[y*map_width + x].frame = 9;
    } else if(x<map_width && map[y*map_width + x+1].id == FLOOR || map[y*map_width + x+1].id == TREE) {
     map[y*map_width + x].frame = 11;
    } else {
     map[y*map_width + x].frame = 10;
    }
   } else if(map[y*map_width + x].id == TOP) {
    if(y>0 && map[(y-1)*map_width + x].id != TOP) {
     if(x>0 && map[y*map_width + x-1].id != TOP) {
      map[y*map_width + x].frame = 0;
     } else if(x<map_width && map[y*map_width + x+1].id != TOP) {
      map[y*map_width + x].frame = 2;
     } else {
      map[y*map_width + x].frame = 1;
     }
    } else if(y<map_height && map[(y+1)*map_width + x].id != TOP) {
     if(x>0 && map[y*map_width + x-1].id != TOP) {
      map[y*map_width + x].frame = 6;
     } else if(x<map_width && map[y*map_width + x+1].id != TOP) {
      map[y*map_width + x].frame = 8;
     } else {
      map[y*map_width + x].frame = 7;
     }
    } else {
     if(x>0 && map[y*map_width + x-1].id != TOP) {
      map[y*map_width + x].frame = 3;
     } else if(x<map_width && map[y*map_width + x+1].id != TOP) {
      map[y*map_width + x].frame = 5;
     } else {
      map[y*map_width + x].frame = 4;
     }
    }
   }
   if(x>0 && y>0 and x<map_width && y<map_height && map[y*map_width + x].id == TOP) {
    if(map[(y-1)*map_width + x].id != TOP && map[(y+1)*map_width + x].id != TOP) {
     if(map[y*map_width + x-1].id != TOP) {
      map[y*map_width + x].frame = 16;
     } else if(map[y*map_width + x+1].id != TOP) {
      map[y*map_width + x].frame = 17;
     }
    } else if (map[y*map_width + x-1].id != TOP && map[y*map_width + x+1].id != TOP) {
     if(map[(y-1)*map_width + x].id != TOP) {
      map[y*map_width + x].frame = 19;
     } else if(map[(y+1)*map_width + x].id != TOP) {
      map[y*map_width + x].frame = 18;
     }
    }
   }
  }
 }
}

void fireBullet(int x, int y, double vel, double angle, int id, int type) {
 bulletTmp.id = id;
 bulletTmp.frame = type;
 bulletTmp.dest.x = x;
 bulletTmp.dest.y = y;
 bulletTmp.vel = vel;
 bulletTmp.angle = angle;
 bullets.push_back(bulletTmp);
 bulletTmp.angle = angle-.4;
 bullets.push_back(bulletTmp);
 bulletTmp.angle = angle+.4;
 bullets.push_back(bulletTmp);
 bulletTmp.angle = angle-.2;
 bullets.push_back(bulletTmp);
 bulletTmp.angle = angle+.2;
 bullets.push_back(bulletTmp);
}
void updateBullets() {
 for(int i=0; i<bullets.size(); i++) {
  //std::cout << bullet.vel * cos(bullet.angle) << std::endl;
  bullets[i].dest.x += bullets[i].vel * cos(bullets[i].angle);
  bullets[i].dest.y += bullets[i].vel * sin(bullets[i].angle);
  bullets[i].tick--;
  //std::cout << bullets[i].tick << std::endl;
  if(bullets[i].tick<0) {
   bullets.erase(bullets.begin()+i);
   i--;
  }
 }
}
void drawBullets() {
 /*SDL_Rect tmp;
 for(auto bullet : bullets) {
  tmp = bullet.dest;
  tmp.x -= offsetX;
  tmp.y -= offsetY;
  drawRect(tmp, setColor(255, 0, 0));
 }*/
 obj tmp;
 for(auto bullet : bullets) {
  tmp = bullet;
  tmp.dest.x -= offsetX;
  tmp.dest.y -= offsetY;
  tmp.dest.x -= tmp.dest.w/2;
  tmp.dest.y -= tmp.dest.h/2;
  if(inScreen(tmp)) {
   tmp.angle = bullet.angle  * 180 / PI;
   tmp.src.x = tmp.src.w * tmp.frame;
   draw(&tmp);
  }
 }
}
void initBullet() {
 bulletTmp.tick=800;
 bulletTmp.dest.w=40;
 //bulletTmp.dest.w=25;
 bulletTmp.dest.h=25;
 bulletTmp.src.w=8;
 bulletTmp.src.h=6;
 bulletTmp.src.y=0;
 bulletTmp.src.x=0;
 bulletTmp.img = setImage("res/bullets.png");
 bulletTmp.center.x = bulletTmp.dest.w/2;
 bulletTmp.center.y = bulletTmp.dest.h/2;
 bulletTmp.rotateOnCenter = true;
}

const Uint8 *keystates;
void input() {
    left=right=down=up=fire=0;
    SDL_Event e;
    keystates = SDL_GetKeyboardState(NULL);
    while(SDL_PollEvent(&e)) {
     if(e.type == SDL_QUIT) running=false;
     if(e.type == SDL_MOUSEBUTTONDOWN) {
      if(e.button.button == SDL_BUTTON_LEFT) fire=1;
     }
    }
    if(keystates[SDL_SCANCODE_ESCAPE]) running=false;
    if(keystates[SDL_SCANCODE_W] || keystates[SDL_SCANCODE_UP]) up=1;
    if(keystates[SDL_SCANCODE_S] || keystates[SDL_SCANCODE_DOWN]) down=1;
    if(keystates[SDL_SCANCODE_A] || keystates[SDL_SCANCODE_LEFT]) left=1;
    if(keystates[SDL_SCANCODE_D] || keystates[SDL_SCANCODE_RIGHT]) right=1;
    //if(keystates[SDL_SCANCODE_P]) {srand(time(NULL));genMap();}

    SDL_GetMouseState(&mouse.x, &mouse.y);
}

int lengthSquare(int x1, int x2, int y1, int y2){
//int lengthSquare(int x1, int y1, int x2, int y2){
//int lengthSquare(pair<int,int> X, pair<int,int> Y){
    int xDiff = x1 - x2;
    int yDiff = y1 - y2;
    return xDiff*xDiff + yDiff*yDiff;
}

bool collide;
int speed = 16;
bool lu, ld, ll, lr;
void update() {
 if(collide) {
  if(lu) player.dest.y+=speed;
  if(ld) player.dest.y-=speed;
  if(ll) player.dest.x+=speed;
  if(lr) player.dest.x-=speed;
 }
 if(up) player.dest.y-=speed;
 if(down) player.dest.y+=speed;
 if(left) player.dest.x-=speed;
 if(right) player.dest.x+=speed;
 offsetX = player.dest.x - WIDTH/2 - player.dest.w/2;
 offsetY = player.dest.y - HEIGHT/2 - player.dest.h/2;
 //player.dest.x = player.coord.x - offsetX;
 //player.dest.y = player.coord.y - offsetY;
 //if(mouse.x) player.flip = 1;
 //if(right) player.flip = 0;
 player.tick+=3;
 if(player.tick>199) player.tick=0;
 player.src.x = round(player.tick/200) * player.src.w;
 //if(left || right) player.src.x = player.src.x + 14;
 gun.center.x=0;
 //if(gun.flipV) gun.center.x=gun.dest.w;
 gun.flipV = player.flip;
 gun.tick++;
 if(gun.tick > 499) gun.tick=0;
 lu=up;ld=down;ll=left;lr=right;
 updateBullets();
}

bool lfire = 0;
void render() {
 SDL_SetRenderDrawColor(renderer, bkg.r, bkg.g, bkg.b, bkg.a);
 SDL_RenderClear(renderer);

 frameCount++;
 timerFPS = SDL_GetTicks()-lastFrame;
 if(timerFPS<(1000/setFPS)) {
  SDL_Delay((1000/setFPS)-timerFPS);
 }
 std::cout << fps << std::endl;

 drawMap();
 //treeObj.dest.x=100;
 //treeObj.dest.y=100;
 //draw(&treeObj);
 obj tmp, tmp2, tmp3;
 tmp = player;
 tmp.dest.x -= offsetX;
 tmp.dest.y -= offsetY;
 player.flip=1;
 if(mouse.x > tmp.dest.x+(tmp.dest.w/2)) player.flip=0;
 //drawRect(tmp, setColor(0, 255, 0));
 draw(&tmp);//player);

 collide = false;
 for(auto m : map) {
  SDL_Rect tmp4;
  tmp4 = m.dest;
  tmp4.x -= offsetX;
  tmp4.y -= offsetY;
  if(m.id == TOP && SDL_HasIntersection(&tmp4, &tmp.dest)) {
   drawRect(tmp4, setColor(255, 0, 0, 100));
   drawOutline(tmp4, setColor(255, 0, 0, 255));
   collide = true;
  }
  for(int g=0; g<bullets.size(); g++) {
   SDL_Rect tmp5 = bullets[g].dest;
   tmp5.x -= offsetX;
   tmp5.y -= offsetY;
   double turn = rand() % 16 - 8;
   //std::cout << turn << std::endl;
   if(m.id == TOP && SDL_HasIntersection(&tmp4, &tmp5)) bullets[g].tick-=50;
   if(m.id == TOP && SDL_HasIntersection(&tmp4, &tmp5)) { bullets[g].vel=-bullets[g].vel; bullets[g].angle+=(turn/10); }
  }
 }

 SDL_Point tmpMouse;
 tmpMouse.x = cursor.dest.x + bulletTmp.dest.w;
 tmpMouse.y = cursor.dest.y + bulletTmp.dest.h;
 //SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255);
 if((mouse.x<(WIDTH/2) && mouse.y<(HEIGHT/2)) || (mouse.x>(WIDTH/2) && mouse.y>(HEIGHT/2))) {
  //SDL_SetRenderDrawColor(renderer, 0, 165, 255, 255);
  tmpMouse.y-=bulletTmp.dest.h;
 }


 tmp2 = gun;
 tmp2.dest.x = tmp.dest.x + 30;
 if(tmp2.flipV) tmp2.dest.x += 12;
 double dot = tmp2.dest.x*tmpMouse.x + tmp2.dest.y*tmpMouse.y;
 double det = tmp2.dest.y*tmpMouse.y - tmp2.dest.x*tmpMouse.x;
 tmp2.dest.y = tmp.dest.y + tmp.dest.h/2.3 + (round(tmp2.tick/200)*2);
 if(fire && !lfire) {
  tmp2.dest.y-=4;
  if(tmp2.flipV) {
   tmp2.dest.x+=16;
  } else {
   tmp2.dest.x-=16;
  }
 } else if(lfire && !fire) {
  tmp2.dest.y-=4;
  if(tmp2.flipV) {
   tmp2.dest.x+=8;
  } else {
   tmp2.dest.x-=8;
  }
 }
 draw(&tmp2);

 float xDistance = mouse.x - tmp2.dest.x;
 float yDistance = mouse.y - tmp2.dest.y;
 double angleToTurn = (atan2(yDistance, xDistance)) * 180 / PI;
 gun.angle=tmp2.angle=angleToTurn;

 xDistance = tmpMouse.x - tmp2.dest.x;
 yDistance = tmpMouse.y - tmp2.dest.y + 15;
 angleToTurn = (atan2(yDistance, xDistance)) * 180 / PI;

 int px = tmp2.dest.x + gun.dest.w * cos(atan2(yDistance, xDistance));
 int py = (tmp2.dest.y+15) + gun.dest.w * sin(atan2(yDistance, xDistance));
 //SDL_RenderDrawLine(renderer, px, py, tmpMouse.x, tmpMouse.y);

 if(fire && !lfire) {
  int bType = rand() % 5;
  int bX, bY;
  double bA = (atan2(yDistance, xDistance));
  int bV = 44;
  bX = (bV * cos(bA));
  bY = (bV * sin(bA));
  fireBullet(px + offsetX, py + offsetY, bV, bA, 1, bType);
  cursor.frame=1;
 } else {
  if(cursor.frame==2) cursor.frame=0;
  if(cursor.frame==1) cursor.frame=2;
 }
 lfire = fire;


 wolf.dest.x=player.dest.x+100-offsetX;
 wolf.dest.y=player.dest.y+100-offsetY;
 wolf.flip = player.flip;
 draw(&wolf);

 drawBullets();

 cursor.dest.x = mouse.x - cursor.dest.w/2; //+ bulletTmp.dest.w;
 cursor.dest.y = mouse.y - cursor.dest.h/2; //+ bulletTmp.dest.h;
 cursor.src.x = cursor.frame * cursor.src.w;
 draw(&cursor);


 write(std::to_string(offsetX) + ", " + std::to_string(offsetY), cursor.dest.x+cursor.dest.w+50, cursor.dest.y+cursor.dest.h+50);

 SDL_RenderPresent(renderer);
}

void init() {
 std::cout << "SEED: " << seed << std::endl;
 srand(seed);
 SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
 if(SDL_Init(SDL_INIT_EVERYTHING) < 0) std::cout << "Failed at SDL_Init()" << std::endl;
 window = SDL_CreateWindow(TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
 renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
 SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
 SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
 TTF_Init();
 font_size = 16;
 font = TTF_OpenFont("res/font.ttf", font_size);
 if(font == NULL) std::cout << "Failed to load font" << std::endl;
 SDL_ShowCursor(SDL_DISABLE);
 setBkg(51, 73, 95);
 //setBkg(255, 0, 0);
 font_color = black; //setColor(0, 255, 255);
 while(floorPer() < 30) genMap();
 offsetX=map_width/2 * tile_size - WIDTH/2;
 offsetY=map_height/2 * tile_size - HEIGHT/2;
 treeObj.dest.w=tile_size;treeObj.src.w=43;
 treeObj.dest.h=tile_size*1.5;treeObj.src.h=76;
 treeObj.src.x=treeObj.src.y=0;
 treeObj.img = setImage("res/tree.png");
 treeObj.flip=1;
 player.dest.x = spawn.x * tile_size;
 player.dest.y = spawn.y * tile_size;
 player.dest.w = tile_size-8;
 player.dest.h = tile_size*1.2;
 player.src.x=player.src.y=0;
 player.src.w=7;
 player.src.h=10;
 player.img = setImage("res/player.png");
 //std::cout << player.dest.w << std::endl;
 //std::cout << player.dest.h << std::endl;
 gun.img = setImage("res/gun.png");
 gun.src.x=gun.src.y=0;
 gun.src.w=9;
 gun.src.h=3;
 gun.dest.w=tile_size*1.1;
 gun.dest.h=gun.dest.w/3;
 gun.center.y=gun.dest.h/2;
 gun.rotateOnCenter = true;
 wolf.img = setImage("res/wolf.png");
 wolf.src.x=wolf.src.y=0;
 wolf.src.w=14;
 wolf.src.h=8;
 wolf.dest.w=tile_size*1.3;
 wolf.dest.h=wolf.dest.w*.7;
 tilesImgId = setImage("res/tiles.png");
 initBullet();
 cursor.img = setImage("res/cursor.png");
 cursor.src.x=cursor.src.y=0;
 cursor.src.w=cursor.src.h=18;
 cursor.dest.w=cursor.dest.h=tile_size;
 cursor.frame=0;
}
void quit() {
 TTF_CloseFont(font);
 SDL_DestroyRenderer(renderer);
 SDL_DestroyWindow(window);
 SDL_Quit();
}

int main() {
 init();
 static int lastTime = 0;
 while(running) {
  lastFrame=SDL_GetTicks();
  if(lastFrame>=(lastTime+1000)) {
   lastTime=lastFrame;
   fps=frameCount;
   frameCount=0;
  }
  input();
  update();
  render();
 }
 quit();
 return 1;
}
