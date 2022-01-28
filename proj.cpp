#if __has_include(<SDL2/SDL.h>)
 #include <SDL2/SDL.h>
 #include <SDL2/SDL_image.h>
 #include <SDL2/SDL_ttf.h>
 #include <SDL2/SDL_mixer.h>
#else
 #include <SDL.h>
 #include <SDL_image.h>
 #include <SDL_ttf.h>
 #include <SDL_mixer.h>
#endif

#include <cstdlib>
#include <iostream>

#include <fstream>
#include <iterator>

#include <vector>
#include <cmath>

const char * TITLE = "Nuclear Winter";

#define PI 3.14159265359

int setFPS = 60;

int WIDTH = 1800;
int HEIGHT = 900;
int flags = SDL_WINDOW_FULLSCREEN; //not used yet
SDL_Renderer* renderer;
SDL_Window* window;
TTF_Font *font;
SDL_Color font_color;
int font_size;
SDL_Rect screenRect;

#define TOP 1
#define WALL 2
#define FLOOR 3
#define TREE 4
#define GATE 5
#define SNOW 6

int frameCount, timerFPS, lastFrame, fps;

bool running = 1;
bool paused, lpaused;
#define PLAYING 1;
#define PAUSED 2;
#define MAIN 3;
#define SAVING 4;
#define LOADING 5;
#define EXITING 6;
int gamestate = PLAYING;
int seed;

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


int gunSound, ricSound, shotSound;
std::vector<Mix_Chunk*> sounds;
int loadSound(const char* filename) {
 Mix_Chunk *s = NULL;
 s = Mix_LoadWAV(filename);
 if(s == NULL) {
  printf( "Failed to load wav! SDL_mixer Error: %s\n", Mix_GetError() );
  return -1;
 }
 sounds.push_back(s);
 return sounds.size()-1;
}
int playSound(int s) {
 Mix_PlayChannel(-1, sounds[s], 0);
 return 0;
}
void quitSounds() {
 for(int s=0; s<sounds.size(); s++) {
  Mix_FreeChunk(sounds[s]);
  sounds[s]=NULL;
 }
 Mix_Quit();
}
int initAudio() {
 SDL_Init(SDL_INIT_AUDIO);
 if(Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 ) < 0) {
  printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
  return -1;
 }
 gunSound = loadSound("res/gun.wav");
 ricSound = loadSound("res/ricochet.wav");
 shotSound = loadSound("res/shot.wav");
 return 0;
}

struct obj;
struct obj {
 SDL_Point coord;
 SDL_Point center;
 SDL_Rect dest, src;
 double angle = 0;
 double vel = 0;
 double lastVel = 0;
 double tick = 0;
 int id = FLOOR;
 int img;
 bool flip = 0;
 bool flipV = 0;
 bool rotateOnCenter = 0;
 int frame;
 obj* child;
 bool parent;
 int health;
 int extra;
 bool extraBool;
 //int alpha=255;
} player, gun, wolf, cursor, gunUI, shellIcon, UI, pauseUI, shellSelect, modSelect;

obj tmpEnemy;
std::vector<obj> enemies;

std::vector<obj> footprints;
int footTick = 0;
obj footTmp;

SDL_Point mouse;

obj treeObj;

obj bulletTmp;
std::vector<obj> bullets;
bool fire = 0;
obj shellTmp;
std::vector<obj> shells;

std::vector<SDL_Texture*> images;
int setImage(std::string filename) {
 images.push_back(IMG_LoadTexture(renderer, filename.c_str()));
 return images.size()-1;
}

SDL_Surface *text_surface;
SDL_Texture *text_texture;
SDL_Rect wrect;
/*void write(std::string t, int x, int y) {
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
}*/
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
 wrect.x = x;
 wrect.y = y;
 SDL_FreeSurface(text_surface);
 SDL_RenderCopy(renderer, text_texture, NULL, &wrect);
 SDL_DestroyTexture(text_texture);
}

std::vector<obj> map, tmpMap;

void draw(obj* o) {
 if(o->img <= sizeof images && o->src.x<1000) {
 //std::cout << renderer << " " << o->img << " src:" << o->src.x << "," << o->src.y << " " << o->src.w << "x" << o->src.h << " dest:" << o->dest.x << "," << o->dest.y << " " << o->dest.w << "x" << o->dest.h << " " << o->angle << "* " << o->center.x << "," << o->center.y << " " << o->flip << " " << o->flipV << " " << o->rotateOnCenter << std::endl;
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
}
void drawDebug(obj* o) {
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
 SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
 SDL_RenderDrawRect(renderer, &o->dest);
 write(std::to_string(o->dest.x) + ", " +  std::to_string(o->dest.y), o->dest.x + 20, o->dest.y + 20);
}

void draw(std::vector<obj*> os) {
 //std::cout << " : " << os.size() << std::endl;
 //for(auto o : os) {
 for(int o=0; o<os.size(); o++) {
  //drawDebug(os[o]);
  draw(os[o]);
  //std::cout << o << std::endl;
 }
}
void drawWithOffset(std::vector<obj> os) {
 obj tmp;
 for(int o=0; o<os.size(); o++) {
  tmp = os[o];
  tmp.dest.x -= offsetX;
  tmp.dest.y -= offsetY;
  //std::cout << tmp.dest.x << ", " << tmp.dest.y<<std::endl;
  draw(&tmp);
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
 return ((o.dest.x+o.dest.w)>-200) && ((o.dest.y+o.dest.h)>-200) && (o.dest.x-(o.dest.w*4)<WIDTH+200) && (o.dest.y-(o.dest.h*4)<HEIGHT+200);
 //return ((o.dest.x+o.dest.w)>0) && ((o.dest.y+o.dest.h)>0) && (o.dest.x-(o.dest.w*4)<WIDTH) && (o.dest.y-(o.dest.h*4)<HEIGHT);
}
bool inScreen(obj* o) {
 return ((o->dest.x+o->dest.w)>-200) && ((o->dest.y+o->dest.h)>-200) && (o->dest.x-(o->dest.w*4)<WIDTH+200) && (o->dest.y-(o->dest.h*4)<HEIGHT+200);
}
std::vector<obj> buffer, buffer2;
int bufLow, bufHigh;
int drawToBuffer(obj o) {
 //std::cout << buffer.size() << std::endl;
 if(o.dest.y+o.dest.h > bufHigh) bufHigh=o.dest.y+o.dest.h;
 if(o.dest.y+o.dest.h < bufLow || bufLow==-99) bufLow=o.dest.y+o.dest.h;
 buffer.push_back(o);
 return buffer.size();
}
void drawBuffer() {
 //rearange buffer by y-axis
 //std::cout << bufLow << " - " << bufHigh << std::endl;
 //std::cout << bufLow << " - " << bufHigh << std::endl;
 //std::cout << buffer.size() << std::endl;
 for(int y=bufLow-500; y<bufHigh+500; y++) {
  for(int b=0; b<buffer.size(); b++) {
   //std::cout << b << " "  <<buffer[b]->dest.x << "," << buffer[b]->dest.y << std::endl;
   if(buffer[b].dest.y+buffer[b].dest.h==y) {
    buffer2.push_back(buffer[b]);
    if(buffer[b].parent) buffer2.push_back(*buffer[b].child);
    //buffer.erase(buffer.begin()+b);
    //b--;
   }
  }
 }
 draw(buffer2);
 buffer.clear();
 buffer2.clear();
 bufLow=bufHigh=-99;
}

int ammo = 0;
int mod = 0;
int mod2 = 0;
bool changingMod = false;
int ammoCount [5] = {999, 999, 999, 999, 999};
std::string mods [6] = {"None", "Velocity", "Damage", "Burst", "Wave", "Bounce"};
std::string mods2 [6] = {"None", "Velocity", "Damage", "Bounce", "Random"};
void drawUI() {
 gunUI.src=gun.src;
 shellSelect.src.y = shellSelect.src.h * ammo;
 draw(&UI);
 draw(&gunUI);
 draw(&shellSelect);
 if(changingMod) draw(&modSelect);
 for(int a=0; a < 5; a++) {
  write(std::to_string(ammoCount[a]), 220, a*23 + 35);
  //write("Ammo #" + std::to_string(a+1) + ": ", 40, a*40 + 40);
 }
 //write("Holding: " + std::to_string(ammo+1), 40, 5*40 + 40);
 write(mods[mod], 193, 6*23 + 16);
 write(mods2[mod2], 193, 7*23 + 20);
 write(std::to_string(fps), 22, 5*40 + 28);
}


obj merge(obj o1, obj o2) {
 obj mo;
 mo=o1;
 if(mo.dest.x>o2.dest.x) mo.dest.x=o2.dest.x;
 if(mo.dest.y>o2.dest.x) mo.dest.y=o2.dest.y;
 if(mo.dest.w<o2.dest.x) mo.dest.w=o2.dest.w;
 if(mo.dest.h<o2.dest.x) mo.dest.h=o2.dest.h;
 if(mo.src.x>o2.dest.x) mo.dest.x=o2.src.x;
 if(mo.src.y>o2.dest.x) mo.dest.y=o2.src.y;
 if(mo.src.w<o2.dest.x) mo.dest.w=o2.src.w;
 if(mo.src.h<o2.dest.x) mo.dest.h=o2.src.h;
 mo.angle=0;
 SDL_Texture* tar;// = SDL_CreateTexture();
 SDL_SetRenderTarget(renderer, tar);
 draw(&o1);
 draw(&o2);
 SDL_SetRenderTarget(renderer, NULL);
 //SDL_RenderPresent(renderer);
 //SDL_RenderClear(renderer);
 //SDL_RenderCopy(renderer, tar, NULL, NULL);
 //SDL_RenderPresent(renderer);
 return mo;
}


int floodCount = 1;
bool flood(int x, int y, int tick) {
 if(tmpMap[y*map_width + x].id == FLOOR && tmpMap[y*map_width + x].tick == 0) {
  tmpMap[y*map_width + x].tick = tick;
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
 if(tmpMap[y*map_width + x].id == FLOOR and t > 0) {
  tmpMap[y*map_width + x].id = TREE;
  tmpMap[y*map_width + x].flip = rand() % 2;
  tmpMap[y*map_width + x].tick = rand() % 4;
  //tmpMap[y*map_width + x].src.x = rand() % 4 * treeObj.src.w;
  plantTree(x-1,y-1,t-1);
  plantTree(x+1,y-1,t-1);
  plantTree(x,y-1,t-1);

  plantTree(x-1,y+1,t-1);
  plantTree(x+1,y+1,t-1);
  plantTree(x,y+1,t-1);

  plantTree(x-1,y,t-1);
  plantTree(x+1,y,t-1);
  //std::cout << "plant" << std::endl;
 }
}

void laySnow(int x, int y, int t) {
 if(tmpMap[y*map_width + x].id == FLOOR and t > 0) {
  tmpMap[y*map_width + x].id = SNOW;

  laySnow(x-1,y-1,t-1);
  laySnow(x+1,y-1,t-1);
  laySnow(x,y-1,t-1);

  laySnow(x-1,y+1,t-1);
  laySnow(x+1,y+1,t-1);
  laySnow(x,y+1,t-1);

  laySnow(x-1,y,t-1);
  laySnow(x+1,y,t-1);
 }
}

bool spawnSet;
SDL_Point spawn;
void genMap() {
 map.clear();
 tmpMap.clear();
 obj tile_tmp;
 tile_tmp.dest.w = tile_tmp.dest.h = tile_size;
 for(int y = 0; y < map_height; y++) {
  tile_tmp.dest.y = tile_size * y;
  for(int x = 0; x < map_width; x++) {
   int of = rand() % 100;
   tile_tmp.dest.x = tile_size * x;
   tile_tmp.id = TOP;
   if(of > 40) tile_tmp.id = FLOOR;
   //std::cout << tile_tmp.id << " ";
   tmpMap.push_back(tile_tmp);
  }
  //std::cout << std::endl;
 }

 int oc, tc;
 for(int i = 0; i < 6; i++) {
  for(int y = 2; y < map_height-2; y++) {
   for(int x = 2; x < map_width-2; x++) {
    oc = tc = 0;

    if(tmpMap[((y-1)*map_width) + (x-1)].id == FLOOR) oc++;
    if(tmpMap[((y-1)*map_width) + x].id == FLOOR) oc++;
    if(tmpMap[((y-1)*map_width) + (x+1)].id == FLOOR) oc++;
    if(tmpMap[(y*map_width) + (x-1)].id == FLOOR) oc++;
    if(tmpMap[(y*map_width) + (x+1)].id == FLOOR) oc++;
    if(tmpMap[((y+1)*map_width) + (x-1)].id == FLOOR) oc++;
    if(tmpMap[((y+1)*map_width) + x].id == FLOOR) oc++;
    if(tmpMap[((y+1)*map_width) + (x+1)].id == FLOOR) oc++;

    if(tmpMap[((y-2)*map_width) + (x-2)].id == FLOOR) tc++;
    if(tmpMap[((y-2)*map_width) + (x-1)].id == FLOOR) tc++;
    if(tmpMap[((y-2)*map_width) + (x)].id == FLOOR) tc++;
    if(tmpMap[((y-2)*map_width) + (x+1)].id == FLOOR) tc++;
    if(tmpMap[((y-2)*map_width) + (x+2)].id == FLOOR) tc++;

    if(tmpMap[((y-1)*map_width) + (x-2)].id == FLOOR) tc++;
    if(tmpMap[((y)*map_width) + (x-2)].id == FLOOR) tc++;
    if(tmpMap[((y+1)*map_width) + (x-2)].id == FLOOR) tc++;

    if(tmpMap[((y-1)*map_width) + (x+2)].id == FLOOR) tc++;
    if(tmpMap[((y)*map_width) + (x+2)].id == FLOOR) tc++;
    if(tmpMap[((y+1)*map_width) + (x+2)].id == FLOOR) tc++;

    if(tmpMap[((y+2)*map_width) + (x-2)].id == FLOOR) tc++;
    if(tmpMap[((y+2)*map_width) + (x-1)].id == FLOOR) tc++;
    if(tmpMap[((y+2)*map_width) + (x)].id == FLOOR) tc++;
    if(tmpMap[((y+2)*map_width) + (x+1)].id == FLOOR) tc++;
    if(tmpMap[((y+2)*map_width) + (x+2)].id == FLOOR) tc++;

    if(i < 1) {
     if(oc>=5 || tc<=7) {
      tmpMap[y*map_width + x].id=FLOOR;
     } else {
      tmpMap[y*map_width + x].id=TOP;
     }
    } else {
     if(oc >= 5) {
      tmpMap[y*map_width + x].id=FLOOR;
     } else {
      tmpMap[y*map_width + x].id=TOP;
     }
    }
   }
  }
 }
 for(int y = 0; y < map_height; y++) {
  tmpMap[y*map_width].id = TOP;
  tmpMap[y*map_width + 1].id = TOP;
  tmpMap[y*map_width + map_width-2].id = TOP;
  tmpMap[y*map_width + map_width-1].id = TOP;
  int gt = rand() % 300;
  if(gt == 1) {tmpMap[y*map_width + map_width-2].id = GATE; tmpMap[y*map_width + map_width-1].id = GATE;}
  if(gt == 2) {tmpMap[y*map_width + 1].id = GATE; tmpMap[y*map_width + 1].id = GATE;}
 }
 for(int x = 0; x < map_width; x++) {
  tmpMap[x].id = TOP;
  tmpMap[map_width + x].id = TOP;
  tmpMap[x + ((map_width-1)*(map_height-1))].id = TOP;
  tmpMap[x + ((map_width)*(map_height-1))].id = TOP;
  int gt = rand() % 300;
  if(gt == 1) {tmpMap[x].id = GATE; tmpMap[x + ((map_width-1)*(map_height-1))].id = GATE;}
  if(gt == 2) {tmpMap[map_width + x].id = GATE; tmpMap[x + ((map_width)*(map_height-1))].id = GATE;}
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
    if(tmpMap[y*map_width + x].id == FLOOR) {
     if(count > maxfc) {fc=i;maxfc=count;}
     count++;
    }
   }
  }
 }
 for(int y = 2; y < map_height-2; y++) {
  for(int x = 2; x < map_width-2; x++) {
   if(tmpMap[y*map_width + x].tick != fc+1) tmpMap[y*map_width + x].id = TOP;
  }
 }

 for(int y = 1; y < map_height-2; y++) {
  for(int x = 2; x < map_width-2; x++) {
   if(tmpMap[y*map_width + x].id == TOP && tmpMap[((y+1)*map_width) + x].id == FLOOR && tmpMap[((y-1)*map_width) + x].id == TOP) {
    tmpMap[y*map_width + x].id = WALL;
   } else if (tmpMap[y*map_width + x].id == TOP && tmpMap[((y+1)*map_width) + x].id == FLOOR && tmpMap[((y+2)*map_width) + x].id == FLOOR) {
    tmpMap[((y+1)*map_width) + x].id = WALL;
   }
  }
 }

 for(int y = 1; y < map_height-2; y++) {
  for(int x = 2; x < map_width-2; x++) {
   int t = rand() % 2000;
   if(tmpMap[y*map_width + x].id == FLOOR && t < 4) plantTree(x, y, rand() % 5 + 2);//tmpMap[y*map_width + x].id = TREE;
   t = rand() % 2000;
   if(tmpMap[y*map_width + x].id == FLOOR && t < 4) laySnow(x, y, rand() % 5 + 2);//tmpMap[y*map_width + x].id = TREE;
  }
 }
 for(int y = 2; y < map_height-2; y++) {
  for(int x = 2; x < map_width-2; x++) {
   if(!spawnSet) {
    if(tmpMap[y*map_width + x].id == FLOOR) {
     if(tmpMap[y*map_width + x-1].id == FLOOR && tmpMap[y+1*map_width + x].id == FLOOR && tmpMap[y+1*map_width + x-1].id == FLOOR) {
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
   if(tmpMap[y*map_width + x].id == FLOOR || tmpMap[y*map_width + x].id == TREE) {
    if(y>0 && tmpMap[(y-1)*map_width + x].id == WALL) {
     if(y>0 && x>0 && tmpMap[(y-1)*map_width + x-1].id != WALL && tmpMap[(y-1)*map_width + x-1].id != TOP) {
      tmpMap[y*map_width + x].frame = 12;
     } else if(y>0 && x<map_width && tmpMap[(y-1)*map_width + x+1].id != WALL && tmpMap[(y-1)*map_width + x+1].id != TOP) {
      tmpMap[y*map_width + x].frame = 14;
     } else {
      tmpMap[y*map_width + x].frame = 13;
     }
    } else {
     tmpMap[y*map_width + x].frame = 15;
    }
    if(tmpMap[(y-1)*map_width + x].frame == 21) tmpMap[y*map_width + x].frame=22;
   } else if(tmpMap[y*map_width + x].id == SNOW) {
    tmpMap[y*map_width + x].frame = 23;
   } else if(tmpMap[y*map_width + x].id == WALL) {
    if(x>0 && tmpMap[y*map_width + x-1].id == FLOOR || tmpMap[y*map_width + x-1].id == TREE) {
     tmpMap[y*map_width + x].frame = 9;
    } else if(x<map_width && tmpMap[y*map_width + x+1].id == FLOOR || tmpMap[y*map_width + x+1].id == TREE) {
     tmpMap[y*map_width + x].frame = 11;
    } else {
     tmpMap[y*map_width + x].frame = 10;
    }
    if(tmpMap[(y-1)*map_width + x].frame == 20) tmpMap[y*map_width + x].frame=21;
   } else if(tmpMap[y*map_width + x].id == TOP) {
    if(y>0 && tmpMap[(y-1)*map_width + x].id != TOP) {
     if(x>0 && tmpMap[y*map_width + x-1].id != TOP) {
      tmpMap[y*map_width + x].frame = 0;
     } else if(x<map_width && tmpMap[y*map_width + x+1].id != TOP) {
      tmpMap[y*map_width + x].frame = 2;
     } else {
      tmpMap[y*map_width + x].frame = 1;
     }
    } else if(y<map_height && tmpMap[(y+1)*map_width + x].id != TOP) {
     if(x>0 && tmpMap[y*map_width + x-1].id != TOP) {
      tmpMap[y*map_width + x].frame = 6;
     } else if(x<map_width && tmpMap[y*map_width + x+1].id != TOP) {
      tmpMap[y*map_width + x].frame = 8;
     } else {
      tmpMap[y*map_width + x].frame = 7;
     }
    } else {
     if(x>0 && tmpMap[y*map_width + x-1].id != TOP) {
      tmpMap[y*map_width + x].frame = 3;
     } else if(x<map_width && tmpMap[y*map_width + x+1].id != TOP) {
      tmpMap[y*map_width + x].frame = 5;
     } else {
      tmpMap[y*map_width + x].frame = 4;
     }
    }
   }
   if(x>0 && y>0 and x<map_width && y<map_height && tmpMap[y*map_width + x].id == TOP) {
    if(tmpMap[(y-1)*map_width + x].id != TOP && tmpMap[(y+1)*map_width + x].id != TOP && tmpMap[y*map_width + x-1].id != TOP && tmpMap[y*map_width + x+1].id != TOP) {
      tmpMap[y*map_width + x].frame = 20;
      //if(y-1>0){ std::cout<<"x"<<std::endl;tmpMap[((y-1)*map_width) + x].frame = 21;}
      //if(y-2>2) {std::cout<<"x"<<std::endl;tmpMap[((y-2)*map_width) + x].frame = 22;}
      //std::cout << x  << " " << y << std::endl;
    } else if(tmpMap[(y-1)*map_width + x].id != TOP && tmpMap[(y+1)*map_width + x].id != TOP) {
     if(tmpMap[y*map_width + x-1].id != TOP) {
      tmpMap[y*map_width + x].frame = 16;
     } else if(tmpMap[y*map_width + x+1].id != TOP) {
      tmpMap[y*map_width + x].frame = 17;
     }
    } else if (tmpMap[y*map_width + x-1].id != TOP && tmpMap[y*map_width + x+1].id != TOP) {
     if(tmpMap[(y-1)*map_width + x].id != TOP) {
      tmpMap[y*map_width + x].frame = 19;
     } else if(tmpMap[(y+1)*map_width + x].id != TOP) {
      tmpMap[y*map_width + x].frame = 18;
     }
    }
   }
  }
 }
}


void floorPer() {
 int c = 0;
 int wc = 0;
 for(auto tile : tmpMap) {
  if(tile.id == FLOOR) c++;
  if(tile.id == WALL) wc++;
 }
 //std::cout << ((c*100)/(tmpMap.size()+1) + 1) << std::endl;
 if(wc == 0 || ((c*100)/(tmpMap.size()+1) + 1) < 30) {
  std::vector<int> seeds;
  std::ifstream inFile;
  inFile.open("res/seeds.txt");
  if (inFile.is_open()) {
   std::copy(std::istream_iterator<double>(inFile), std::istream_iterator<double>(), std::back_inserter(seeds));
   inFile.close();
   seed = seeds[rand() % seeds.size()];
   srand(seed);
   std::cout << "SEED: " << seed << std::endl;
   map.clear();
   tmpMap.clear();
   genMap();
   //floorPer();
  }
 //} else {
 }
 map = tmpMap;
}

void dropShell(int cx, int cy, double angle, int type) {
 shellTmp.id = 5;
 shellTmp.dest.x = cx;
 shellTmp.dest.y = cy;
 shellTmp.angle = angle;
 shellTmp.tick = 17;
 shells.push_back(shellTmp);
 ammoCount[type]--;
}
//bType, bDmg, bnc);
void fireBullet(int x, int y, double vel, double angle, int id, int type, int dmg, int bnc) {
 playSound(gunSound);
 bulletTmp.id = id;
 bulletTmp.frame = type;
 bulletTmp.dest.x = x;
 bulletTmp.dest.y = y;
 bulletTmp.vel = vel;
 bulletTmp.angle = angle;
 bulletTmp.extra = dmg;
 bulletTmp.extraBool = bnc;
 bullets.push_back(bulletTmp);
}
//void fireBullet(int x, int y, double vel, double angle, int id, int type, int speed, int dmg, int bnc) {
void fireBullet(int x, int y, double vel, double angle, int id, int type, int cx, int cy, int dmg, int bnc) {
 fireBullet(x,y,vel,angle,id,type,dmg,bnc);
 dropShell(cx, cy, angle, type);
}
void updateBullets() {
 /*for(int i=0; i<bullets.size(); i++) {
  //std::cout << bullet.vel * cos(bullet.angle) << std::endl;
  bullets[i].dest.x += bullets[i].vel * cos(bullets[i].angle);
  bullets[i].dest.y += bullets[i].vel * sin(bullets[i].angle);
  bullets[i].tick--;
  //std::cout << bullets[i].tick << std::endl;
  if(bullets[i].tick<0) {
   bullets.erase(bullets.begin()+i);
   i--;
  }
 }*/
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
 for(int i=0; i<shells.size(); i++) {
  tmp = shells[i];
  tmp.dest.x -= offsetX;
  tmp.dest.y -= offsetY;
  tmp.dest.x -= tmp.dest.w/2;
  tmp.dest.y -= tmp.dest.h/2;
  //tmp.dest.x=tmp.dest.y=0;
  tmp.dest.y -= sin((tmp.tick)/6) * (player.dest.h/2);
  int ang = tmp.angle * 180 / PI;// % 360;
  if(ang<0) ang+=360;
  if(ang > 90 && ang < 270) {
   tmp.dest.x -= tmp.tick * 8 - 20;// - player.dest.w;
  } else {
   tmp.dest.x += tmp.tick * 8 - 240;// - player.dest.w;
  }
  //std::cout << tmp.angle * 180 / PI << std::endl;//int(tmp.angle * 180 / PI) % 360 << std::endl;
  tmp.src.x = tmp.id * tmp.src.w;
  if(inScreen(tmp)) {
   tmp.angle = shells[i].angle  * 180 / PI;
   draw(&tmp);
   //drawToBuffer(tmp);
  }
  shells[i].tick--;
  if(shells[i].tick<-2) {
   shells.erase(shells.begin()+i);
   i--;
  }
  //std::cout << tmp.dest.x << " " << tmp.dest.y << " " << tmp.dest.w << " " << tmp.dest.h << " " << tmp.src.x << " " << tmp.src.y << " " << tmp.src.w << " " << tmp.src.h << std::endl;
 }
 for(int i=0; i<bullets.size(); i++) {
  bullets[i].dest.x += bullets[i].vel * cos(bullets[i].angle);
  bullets[i].dest.y += bullets[i].vel * sin(bullets[i].angle);
  bullets[i].tick--;
  if(bullets[i].tick<0) {
   //playSound(shotSound);
   bullets.erase(bullets.begin()+i);
   i--;
  }

  tmp = bullets[i];
  tmp.dest.x -= offsetX;
  tmp.dest.y -= offsetY;
  tmp.dest.x -= tmp.dest.w/2;
  tmp.dest.y -= tmp.dest.h/2;
  if(inScreen(tmp)) {
   tmp.angle = bullets[i].angle  * 180 / PI;
   tmp.src.x = tmp.src.w * tmp.frame;
   draw(&tmp);
   //drawToBuffer(tmp);
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
 shellTmp.src.w=8;
 shellTmp.src.h=6;
 shellTmp.dest.w=20;
 shellTmp.dest.h=12;
 shellTmp.src.x=shellTmp.src.y=0;
 shellTmp.img = setImage("res/shells.png");
}

const Uint8 *keystates;
Uint32 mousestate;
SDL_Event e;
void input() {
    left=right=down=up=fire=0;
    int scroll = 0;
    int select = -1;
    keystates = SDL_GetKeyboardState(NULL);
    while(SDL_PollEvent(&e)) {
     if(e.type == SDL_QUIT) running=false;
     /*if(e.type == SDL_MOUSEBUTTONDOWN) {
      if(e.button.button == SDL_BUTTON_LEFT) fire=1;
     }*/
     if(e.type == SDL_MOUSEWHEEL) {
      if(e.wheel.y>0) scroll=1;
      if(e.wheel.y<0) scroll=-1;
     }
    }
    if(keystates[SDL_SCANCODE_ESCAPE]) running=false;
    if(keystates[SDL_SCANCODE_W] || keystates[SDL_SCANCODE_UP]) up=1;
    if(keystates[SDL_SCANCODE_S] || keystates[SDL_SCANCODE_DOWN]) down=1;
    if(keystates[SDL_SCANCODE_A] || keystates[SDL_SCANCODE_LEFT]) left=1;
    if(keystates[SDL_SCANCODE_D] || keystates[SDL_SCANCODE_RIGHT]) right=1;
    if(keystates[SDL_SCANCODE_1]) select=0;
    if(keystates[SDL_SCANCODE_2]) select=1;
    if(keystates[SDL_SCANCODE_3]) select=2;
    if(keystates[SDL_SCANCODE_4]) select=3;
    if(keystates[SDL_SCANCODE_5]) select=4;
    if(keystates[SDL_SCANCODE_6]) select=5;
    if(keystates[SDL_SCANCODE_7]) select=6;
    if(keystates[SDL_SCANCODE_P]) {
     if(!lpaused){paused=true;}lpaused=1;
    } else{
     lpaused=0;
    }
    changingMod = false;
    if(keystates[SDL_SCANCODE_LSHIFT]) {
     if(select != -1) mod = select;
     mod-=scroll;
     changingMod = true;
     modSelect.src.y = modSelect.src.h * 5;
    } else if(keystates[SDL_SCANCODE_LCTRL]) {
     if(select != -1) mod2 = select;
     mod2-=scroll;
     changingMod = true;
     modSelect.src.y = modSelect.src.h * 6;
   } else {
     if(select != -1) ammo = select;
     ammo-=scroll;
    }

    mousestate = SDL_GetMouseState(&mouse.x, &mouse.y);
    if(mousestate == SDL_BUTTON_LEFT) fire=1;

    if(ammo>4) ammo=0;
    if(ammo<0) ammo=4;
    if(mod>5) mod=0;
    if(mod<0) mod=5;
    if(mod2>4) mod2=0;
    if(mod2<0) mod2=4;
}

int lengthSquare(int x1, int x2, int y1, int y2){
//int lengthSquare(int x1, int y1, int x2, int y2){
//int lengthSquare(pair<int,int> X, pair<int,int> Y){
    int xDiff = x1 - x2;
    int yDiff = y1 - y2;
    return xDiff*xDiff + yDiff*yDiff;
}

bool collideH, collideV, inSnow, wallCollide;
int speed;
bool lu, ld, ll, lr;
void update() {
 speed = 16;
 if(inSnow) speed=4;
 player.lastVel=speed;
 if(collideV) {
  if(lu) player.dest.y+=speed;
  if(ld) player.dest.y-=speed;
  //if(ll) player.dest.x+=speed;
  //if(lr) player.dest.x-=speed;
 }
 if(collideH) {
  if(ll) player.dest.x+=speed;
  if(lr) player.dest.x-=speed;
 }
 //movement
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
 //updateBullets();
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
 //std::cout << fps << std::endl;
 //drawMap();
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

 collideV = collideH = inSnow = wallCollide = false;

 obj tmp20;
 std::vector<obj> tmpTrees;
 std::vector<int> tmpTreesCnt;
 int count = 0;
 for(auto tile : map) {
  tmp20 = tile;
  tmp20.dest.x -= offsetX;
  tmp20.dest.y -= offsetY;

  if(inScreen(tmp20)) {
   tmp20.img = tilesImgId;
   tmp20.src.x=tmp20.src.y=0;
   tmp20.src.w=20;tmp20.src.h=20;
   if(tile.id == WALL || tile.id == FLOOR || tile.id == TREE || tile.id == TOP || tile.id == SNOW) {
    if(tile.id == WALL && SDL_HasIntersection(&tmp20.dest, &tmp.dest)) wallCollide = true;
    tmp20.src.x = tmp20.frame * tmp20.src.w;
    //if(tile.frame >= 3) {
     draw(&tmp20);
    /*} else {
     drawToBuffer(tmp20);
    }*/
   } else if(tile.id == GATE) {
    drawRect(tmp20.dest, setColor(200, 90, 90));
   }
   if(tile.id == TREE) {
    tmpTrees.push_back(tile);
    tmpTreesCnt.push_back(count);
   }
   //collideV=collideH=1;
   //if(tile.id == TOP && tile.frame >= 3 && SDL_HasIntersection(&tmp20.dest, &tmp.dest)) {
   if(tile.id == SNOW && SDL_HasIntersection(&tmp20.dest, &tmp.dest)) inSnow=true;
   if(tile.id == TOP && SDL_HasIntersection(&tmp20.dest, &tmp.dest)) {
    obj otmp, otmp2;
    otmp=tmp;
    if(ll) otmp.dest.x+=player.lastVel;
    if(lr) otmp.dest.x-=player.lastVel;
    if(lu) otmp.dest.y+=player.lastVel;
    if(ld) otmp.dest.y-=player.lastVel;
    otmp2=otmp;
    if(ll) {
     otmp = otmp2;
     otmp.dest.x-=player.lastVel;
     if(SDL_HasIntersection(&tmp20.dest, &otmp.dest)) collideH = true;
    } else if(lr) {
     otmp = otmp2;
     otmp.dest.x+=player.lastVel;
     if(SDL_HasIntersection(&tmp20.dest, &otmp.dest)) collideH = true;
    }
    if(ld) {
     otmp = otmp2;
     otmp.dest.y+=player.lastVel;
     if(SDL_HasIntersection(&tmp20.dest, &otmp.dest)) collideV = true;
    } else if(lu) {
     otmp = otmp2;
     otmp.dest.y-=player.lastVel;
     if(SDL_HasIntersection(&tmp20.dest, &otmp.dest)) collideV = true;
    }
    //drawRect(tmp20.dest, setColor(255, 0, 0, 100));
    //drawOutline(tmp20.dest, setColor(255, 0, 0, 255));
   }
  for(int g=0; g<bullets.size(); g++) {
   SDL_Rect tmp5 = bullets[g].dest;
   tmp5.x -= offsetX;
   tmp5.y -= offsetY;
   double turn = rand() % 16 - 8;
   //std::cout << turn << std::endl;
   if(tile.id == TOP && SDL_HasIntersection(&tmp20.dest, &tmp5)) bullets[g].tick-=80;
   if(tile.id == TOP && SDL_HasIntersection(&tmp20.dest, &tmp5) && bullets[g].extraBool) { bullets[g].vel=-bullets[g].vel; bullets[g].angle+=(turn/10);}//playSound(ricSound); }
  }
  }// 
  count++;
 }
 drawWithOffset(footprints);
 for(int i=0; i<tmpTrees.size(); i++) {
  tmpTrees[i].dest.x -= offsetX;
  tmpTrees[i].dest.y -= offsetY;
  if(tmpTrees[i].id == TREE && map[tmpTreesCnt[i]+1].id != TOP) {
   treeObj.dest.x = tmpTrees[i].dest.x + ((tmpTreesCnt[i]/map_width)%2)*42;
   treeObj.dest.y = tmpTrees[i].dest.y - (treeObj.dest.h-tile_size) - 3;
   //if(i==0)std::cout << treeObj.dest.y << std::endl;

   treeObj.flip = tmpTrees[i].flip;
   treeObj.src.x = tmpTrees[i].tick * treeObj.src.w;
   //draw(&treeObj);
   drawToBuffer(treeObj);
   //std::cout << i << std::endl;
  }

 }

 //draw(&tmp);//player);
 //drawToBuffer(tmp);//player);

 SDL_Point tmpMouse;
 tmpMouse.x = cursor.dest.x + bulletTmp.dest.w;
 tmpMouse.y = cursor.dest.y + bulletTmp.dest.h;
 //SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255);
 if((mouse.x<(WIDTH/2) && mouse.y<(HEIGHT/2)) || (mouse.x>(WIDTH/2) && mouse.y>(HEIGHT/2))) {
  //SDL_SetRenderDrawColor(renderer, 0, 165, 255, 255);
  tmpMouse.y-=bulletTmp.dest.h;
 }


 gun.src.y = gun.src.h * ammo;
 gun.src.x = gun.src.w * mod;
 tmp2 = gun;
 tmp2.dest.x = tmp.dest.x + 30;
 if(tmp2.flipV) tmp2.dest.x += 12;
 double dot = tmp2.dest.x*tmpMouse.x + tmp2.dest.y*tmpMouse.y;
 double det = tmp2.dest.y*tmpMouse.y - tmp2.dest.x*tmpMouse.x;
 tmp2.dest.y = tmp.dest.y + tmp.dest.h/3 + (round(tmp2.tick/200)*2);
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
 //draw(&tmp2);
 //drawToBuffer(tmp2);
 tmp.parent=1;
 tmp.child=&tmp2;
 drawToBuffer(tmp);//player);
 //merge(tmp, tmp2); //////////////////////////////////////////////////////////////////////////////////

 float xDistance = mouse.x - tmp2.dest.x;
 float yDistance = mouse.y - tmp2.dest.y;
 double angleToTurn = (atan2(yDistance, xDistance)) * 180 / PI;
 gun.angle=tmp2.angle=angleToTurn;

 if(lu || ld || ll || lr) {
  footTick++;
 } else {
  footTick--;
 }
 if(footTick>3) {
  footTick=0;
  footTmp.dest.x = player.dest.x + (player.dest.w/2) - (footTmp.dest.w/2);
  footTmp.dest.y = player.dest.y + player.dest.h - footTmp.dest.h;
  footTmp.angle = angleToTurn;
  if(footTmp.src.x==0) {
   footTmp.src.x=footTmp.src.w;
  } else {
   footTmp.src.x=0;
  }
  //make sure not touching wall/top
  if(!wallCollide && !collideV && !collideH) footprints.push_back(footTmp);
 }
 if(footTick<0)footTick=0;
 for(int f=0; f<footprints.size(); f++) {
  footprints[f].tick--;
  if(footprints[f].tick<200)footprints[f].src.y=footprints[f].src.h * 1;
  if(footprints[f].tick<100)footprints[f].src.y=footprints[f].src.h * 2;
  if(footprints[f].tick<40)footprints[f].src.y=footprints[f].src.h * 3;
  if(footprints[f].tick<0) {
   footprints.erase(footprints.begin()+f);
   f--;
  }
 }


 xDistance = tmpMouse.x - tmp2.dest.x;
 yDistance = tmpMouse.y - tmp2.dest.y + 15;
 //angleToTurn = (atan2(yDistance, xDistance)) * 180 / PI;

 int px = tmp2.dest.x + gun.dest.w * cos(atan2(yDistance, xDistance));
 int py = (tmp2.dest.y+15) + gun.dest.w * sin(atan2(yDistance, xDistance));
 //SDL_RenderDrawLine(renderer, px, py, tmpMouse.x, tmpMouse.y);

 //if(fire) {// && !lfire) {
 if(ammoCount[ammo] <= 0) fire=0;
 if(fire) {// && !lfire) {
  int bX, bY;
  double bA = (atan2(yDistance, xDistance));
  int bV = 40;
  bX = (bV * cos(bA));
  bY = (bV * sin(bA));
  //int bType = rand() % 5;
  int bType = ammo;
  //std::string mods [6] = {"None", "Velocity", "Damage", "Burst", "Wave", "Bounce"};
  //std::string mods2 [6] = {"None", "Velocity", "Damage", "Bounce"};
  int bDmg = 0;
  bool bnc = 0;
  if(mods[mod] == "Velocity") bV+=10;
  if(mods2[mod2] == "Velocity") bV+=10;
  if(mods[mod] == "Damage") bDmg+=8;
  if(mods2[mod2] == "Damage") bDmg+=8;
  if(mods[mod] == "Bounce" || mods2[mod2] == "Bounce") bnc=1;
  if(mods2[mod2] == "Random") ammo = rand() % 5;
  if(bType == 1) bnc=1;
  if(bType == 2) bDmg+=8;
  if(bType == 3) {bDmg+=4;bV+=4;}
  if(bType == 3) bV+=8;
  fireBullet(px + offsetX, py + offsetY, bV, bA, 1, bType, bDmg, bnc);
  if(mods[mod] == "Burst") {
   fireBullet(px + offsetX, py + offsetY, bV, bA-.2, 1, bType, bDmg, bnc);
   fireBullet(px + offsetX, py + offsetY, bV, bA-.4, 1, bType, bDmg, bnc);
   fireBullet(px + offsetX, py + offsetY, bV, bA+.2, 1, bType, bDmg, bnc);
   fireBullet(px + offsetX, py + offsetY, bV, bA+.4, 1, bType, bDmg, bnc);
  }
  if(mods[mod] == "Wave") {
   //fireBullet(px + offsetX, py + offsetY, bV, bA-.3, 1, bType, bDmg, bnc);
   fireBullet(px + offsetX, py + offsetY, bV, bA-5, 1, bType, bDmg, bnc);
   fireBullet(px + offsetX, py + offsetY, bV, bA+5, 1, bType, bDmg, bnc);
   fireBullet(px + offsetX, py + offsetY, bV, bA-10, 1, bType, bDmg, bnc);
   fireBullet(px + offsetX, py + offsetY, bV, bA+10, 1, bType, bDmg, bnc);
  }
  dropShell(tmp2.dest.x + offsetX + (player.dest.w*1.7), tmp2.dest.y+15 + offsetY, bA, bType);
  cursor.frame=1;
 } else {
  if(cursor.frame==2) cursor.frame=0;
  if(cursor.frame==1) cursor.frame=2;
 }
 lfire = fire;


 wolf.dest.x=player.dest.x+100-offsetX;
 wolf.dest.y=player.dest.y+100-offsetY;
 wolf.flip = player.flip;
 drawToBuffer(wolf);

 //draw(buffer);buffer.clear();
 drawBuffer();

 drawBullets();

 drawUI();

 cursor.dest.x = mouse.x - cursor.dest.w/2; //+ bulletTmp.dest.w;
 cursor.dest.y = mouse.y - cursor.dest.h/2; //+ bulletTmp.dest.h;
 cursor.src.x = cursor.frame * cursor.src.w;
 draw(&cursor);


 //write(std::to_string(offsetX) + ", " + std::to_string(offsetY), cursor.dest.x+cursor.dest.w+50, cursor.dest.y+cursor.dest.h+50);


 SDL_RenderPresent(renderer);
}

void init() {
 std::cout << "SEED: " << seed << std::endl;
 srand(seed);
 SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
 if(SDL_Init(SDL_INIT_EVERYTHING) < 0) std::cout << "Failed at SDL_Init()" << std::endl;
 SDL_DisplayMode DM;
 SDL_GetCurrentDisplayMode(0, &DM);
 WIDTH=DM.w;
 HEIGHT=DM.h;
 window = SDL_CreateWindow(TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
 renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
 SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
 SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
 SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
 TTF_Init();
 font_size = 16;
 font = TTF_OpenFont("res/font.ttf", font_size);
 if(font == NULL) std::cout << "Failed to load font" << std::endl;
 SDL_ShowCursor(SDL_DISABLE);
 setBkg(51, 73, 95);
 //setBkg(255, 0, 0);
 font_color = white; //setColor(0, 255, 255);
 initAudio();
 genMap();
 floorPer();
 std::ofstream out;
 out.open("res/seeds.txt", std::ios::app);
 std::string str = std::to_string(seed) + "\n";
 out << str;
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
 gun.img = setImage("res/raygun.png");//gun.png");
 gun.src.x=gun.src.y=0;
 gun.src.w=14;//9;
 gun.src.h=5;//3;
 gun.dest.w=tile_size*1.1;
 gun.dest.h=gun.dest.w/2;
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
 screenRect.x=screenRect.y=0;
 screenRect.x=WIDTH;screenRect.y=HEIGHT;
 footTmp.dest.w=50;
 footTmp.dest.h=60;
 footTmp.src.x=0;
 footTmp.src.y=0;
 footTmp.src.w=10;
 footTmp.src.h=15;
 footTmp.img = setImage("res/footprints.png");
 footTmp.tick = 300;
 gunUI=gun;
 gunUI.angle = -20;
 //gunUI.flip=1;
 gunUI.dest.x=43;gunUI.dest.y=96;
 gunUI.dest.w=gun.dest.w*1.5;gunUI.dest.h=gun.dest.h*1.5;
 UI.img = setImage("res/UI.png");
 UI.src.x=UI.src.y=0;
 UI.src.w=220;UI.src.h=154;
 UI.dest.x=UI.dest.y=20;
 UI.dest.w=250;UI.dest.h=200;
 pauseUI.img = setImage("res/pause.png");
 pauseUI.src.x=pauseUI.src.y=0;
 pauseUI.src.w=1280;pauseUI.src.h=72;
 pauseUI.dest.x=pauseUI.dest.y=0;
 pauseUI.dest.w=DM.w;pauseUI.dest.h=DM.h;
 paused = 1;
 shellSelect = UI;
 shellSelect.img = setImage("res/select.png");
 modSelect = shellSelect;
}

void quit() {
 quitSounds();
 TTF_CloseFont(font);
 SDL_DestroyRenderer(renderer);
 SDL_DestroyWindow(window);
 SDL_Quit();
}

int main(int argc, char **argv) {
 seed = time(NULL);
 //std::cout << argv[1] << "\n";
 if(argc>=2) seed = atoi(argv[1]);
 init();
 static int lastTime = 0;
 while(running) {
  lastFrame=SDL_GetTicks();
  if(lastFrame>=(lastTime+1000)) {
   lastTime=lastFrame;
   fps=frameCount;
   frameCount=0;
  }
  if(!paused) {
  input();
  update();
  render();
  } else {
   SDL_SetRenderDrawColor(renderer, bkg.r, bkg.g, bkg.b, bkg.a);
   SDL_RenderClear(renderer);
   frameCount++;
   timerFPS = SDL_GetTicks()-lastFrame;
   if(timerFPS<(1000/setFPS)) {
    SDL_Delay((1000/setFPS)-timerFPS);
   }
   SDL_GetMouseState(&mouse.x, &mouse.y);
   cursor.dest.x = mouse.x - cursor.dest.w/2;
   cursor.dest.y = mouse.y - cursor.dest.h/2;
   cursor.src.x = cursor.frame * cursor.src.w;
   draw(&pauseUI);
   draw(&cursor);
   SDL_RenderPresent(renderer);
   keystates = SDL_GetKeyboardState(NULL);
   if(keystates[SDL_SCANCODE_ESCAPE]) running=false;
   while(SDL_PollEvent(&e)) {
     if(e.type == SDL_QUIT) running=false;
   }
   if(keystates[SDL_SCANCODE_P]) {if(!lpaused){paused=false;}lpaused=1;
   }else{lpaused=0;}
  }
 }
 quit();
 return 1;
}

