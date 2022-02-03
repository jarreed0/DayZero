//import
#include "include.h"

//def
const char * TITLE = "Day One";

bool running;
int WIDTH = 1800;
int HEIGHT = 900;
int flags = SDL_WINDOW_FULLSCREEN; //not used yet
SDL_Renderer* renderer;
SDL_Texture* screen;
SDL_Window* window;
TTF_Font *font;
SDL_Color font_color;
int font_size;
SDL_DisplayMode DM;
int seed;

#define PI 3.14159265359

#define PLAYER_ID 0
#define ENEMY_ID 1

#define TOP 1
#define WALL 2
#define FLOOR 3
#define TREE 4
#define GATE 5
#define SNOW 6

int frameCount, timerFPS, lastFrame, fps, lastTime;
int setFPS = 60;

bool shake, shaking;
SDL_Rect screensrc, screendest;
int shaketick;

const Uint8 *keystates;
Uint32 mousestate;
SDL_Event event;

SDL_Point camera;
SDL_Rect lens;

bool left, right, down, up, fire, lfire;

bool debug = 0;
bool freeroam = 0;

int ammo = 0;
int mod = 0;
int mod2 = 0;
bool changingMod = false;
int ammoCount[5] = {100,50,0,0,0};
std::string mods[6] = {"None", "Velocity", "Damage", "Burst", "Wave", "Bounce"};
std::string mods2[6] = {"None", "Velocity", "Damage", "Bounce", "Random"};
bool modsUnlocked[6] = {1,0,0,0,0,0};
bool mods2Unlocked[5] = {1,0,0,0,0};

//obj
struct obj;
struct tile {
 SDL_Rect loc;
 int id;
 int frame;
 int tick;
 int flip;
};
struct obj {
    int id;
    SDL_Rect src, loc;
    SDL_Point center;
    bool flipV, flipH;
    double angle;
    double vel, lastVel;
    bool rotateOnCenter;
    int frame;
    double health, maxHealth;
    int img;
    double tick = 0;
    int extra;
    bool bExtra;
    bool parent;
    std::vector<obj*> children;
    bool collideV, collideH;
    bool alive;
} player, gun, treeObj;

obj lighting;
bool dayCycle;
int dayClock;

obj enemy, enemyShadow;
std::vector<obj> enemies;

obj footTmp;
std::vector<obj> footPrints;
int footTick = 0;

obj snowTmp;
std::vector<obj> snowFall;

obj chess, shell, heart;
std::vector<obj> collectables;

obj bullet;
std::vector<obj> bullets;

SDL_Point mouse;
obj cursor;

obj UI, gunUI, modSelect, shellSelect;
SDL_Rect healthBar;
double maxHealthBar;

//map
int map_width = 150;
int map_height = 150;
int tile_size = 75;
std::vector<tile> map;
int tileImgId;

//extra
SDL_Color setColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a) { return SDL_Color {r, g, b, a}; }
SDL_Color setColor(Uint8 r, Uint8 g, Uint8 b) { return setColor(r, g, b, 255); }
SDL_Color white = setColor(255, 255, 255, 255);
SDL_Color black = setColor(0, 0, 0, 255);
SDL_Color red = setColor(255, 90, 90, 255);
SDL_Color blue = setColor(18, 35, 94, 255);
SDL_Color healthColor = red;
SDL_Color wallColor = setColor(94, 120, 140);
SDL_Color floorColor = setColor(190, 216, 239);
SDL_Color bkg;
void setBkg(Uint8 r, Uint8 g, Uint8 b) {
 bkg = setColor(r, g, b);
}

void setCamera(obj o) {
    camera.x = o.loc.x + o.loc.w/2;
    camera.y = o.loc.y + o.loc.h/2;
}

SDL_Rect initRect(int x, int y, int w, int h) {
    SDL_Rect r;
    r.x=x;
    r.y=y;
    r.w=w;
    r.h=h;
    return r;
}

//sound
int gunSound;
int song;
std::vector<Mix_Chunk*> sounds;
std::vector<Mix_Music*> music;
int loadMusic(const char* filename) {
 Mix_Music *m = NULL;
 m = Mix_LoadMUS(filename);
 if(m == NULL) {
  printf( "Failed to load wav! SDL_mixer Error: %s\n", Mix_GetError() );
  return -1;
 }
 music.push_back(m);
 return music.size()-1;
}
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
 Mix_Volume(-1, 22);
 Mix_PlayChannel(-1, sounds[s], 0);
 return 0;
}
int playMusic(int m) {
 if(Mix_PlayingMusic() == 0) {
  Mix_Volume(1, MIX_MAX_VOLUME);
  Mix_PlayMusic(music[m], -1);
 }
 return 0;
}

//world gen
//flood
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

  return true;
 }
 return false;
}
void dropChess(int x, int y) {
    chess.id = 6;
    chess.loc.x = x*tile_size + chess.loc.w/2;
    chess.loc.y = y*tile_size + chess.loc.h/2;
    collectables.push_back(chess);
}
void dropShell(int x, int y, int type) {
    shell.id = type;
    shell.src.x = shell.src.w * type;
    shell.loc.x = x*tile_size + shell.loc.w/2;
    shell.loc.y = y*tile_size + shell.loc.h/2;
    shell.tick = 50;
    collectables.push_back(shell);
}
void dropHeart(int x, int y, int type) {
    heart.id = 7 + type;
    heart.src.x = heart.src.w * type;
    heart.loc.x = x*tile_size + heart.loc.w/2;
    heart.loc.y = y*tile_size + heart.loc.h/2;
    collectables.push_back(heart);
}
void spawnEnemy(int cx, int cy, int type, int type2, int health) {
 enemy.id = type;
 enemy.frame = type2;
 enemy.src.x=enemy.src.w*type;
 enemy.src.y=enemy.src.h*type2;
 enemy.loc.x = cx;
 enemy.loc.y = cy;
 enemy.loc.w=enemy.loc.h=rand() % tile_size + tile_size*.5;
 enemy.tick=50;
 enemy.angle=0;
 //enemyShadows.push_back(enemyShadow);
 //enemy.child=enemyShadows.size()-1;
 enemy.health=health;
 enemies.push_back(enemy);
}
void spawnEnemies(int cx, int cy, int cnt) {
 for(int x=cx-(cnt/2); x<cx+(cnt/2); x++) {
  for(int y=cy-(cnt/2); y<cy+(cnt/2); y++) {
   //if(x < lens.x/tile_size && x > (lens.x+WIDTH)/tile_size && y < lens.y/tile_size && y > (lens.y+HEIGHT)/tile_size) {
    int type = rand() % 10;
    if(type!=1) type=0;
    if(map[y*map_width + x].id == FLOOR && rand() % 10 > 3) {spawnEnemy((x)*tile_size + (enemy.loc.w/2), (y)*tile_size + (enemy.loc.h/2), rand() % 5, type, 60+(140*type));}
   //}
  }
 }
}
//spread, snow, tree
void spread(int x, int y, int t, int type) {
 if(map[y*map_width + x].id == FLOOR and t > 0) {
  if((type == TREE && (map[y*map_width + x+1].id != WALL && map[y*map_width + x+1].id != TOP) && (map[y*map_width + x+2].id != WALL && map[y*map_width + x+2].id != TOP)) || type != TREE) {
    map[y*map_width + x].id = type;
    map[y*map_width + x].flip = rand() % 2;
    map[y*map_width + x].tick = rand() % 4;

    spread(x-1,y-1,t-1, type);
    spread(x+1,y-1,t-1, type);
    spread(x,y-1,t-1, type);

    spread(x-1,y+1,t-1, type);
    spread(x+1,y+1,t-1, type);
    spread(x,y+1,t-1, type);

    spread(x-1,y,t-1, type);
    spread(x+1,y,t-1, type);
  }
 }
}
//shuffle
void shuffleMap() {
 map.clear();
 tile tile_tmp;
 tile_tmp.loc.w = tile_tmp.loc.h = tile_size;
 for(int y = 0; y < map_height; y++) {
  tile_tmp.loc.y = tile_size * y;
  for(int x = 0; x < map_width; x++) {
   int of = rand() % 100;
   tile_tmp.loc.x = tile_size * x;
   tile_tmp.id = TOP;
   if(of > 40) tile_tmp.id = FLOOR;
   map.push_back(tile_tmp);
  }
 }
}
//gen
void genMap() {
 shuffleMap();

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
 }
 for(int x = 0; x < map_width; x++) {
  map[x].id = TOP;
  map[map_width + x].id = TOP;
  map[x + ((map_width-1)*(map_height-1))].id = TOP;
  map[x + ((map_width)*(map_height-1))].id = TOP;
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
   if(map[y*map_width + x].id == FLOOR && t < 4) spread(x, y, rand() % 5 + 2, TREE);
   t = rand() % 2000;
   if(map[y*map_width + x].id == FLOOR && t < 4) spread(x, y, rand() % 5 + 2, SNOW);
  }
 }
 player.bExtra=0;
 for(int y = 2; y < map_height-2; y++) {
  for(int x = 2; x < map_width-2; x++) {
   if(!player.bExtra) {
    if(map[y*map_width + x].id == FLOOR) {
     if(map[y*map_width + x-1].id == FLOOR && map[y+1*map_width + x].id == FLOOR && map[y+1*map_width + x-1].id == FLOOR) {
      if(rand() % 100 == 1) {
       player.loc.x=x*tile_size;player.loc.y=y*tile_size;
       player.bExtra=true;
      }
     }
    }
   }
  }
 }
 lens.x=camera.x-(lens.w/2);
 lens.y=camera.y-(lens.h/2);
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
    if(map[(y-1)*map_width + x].frame == 21) map[y*map_width + x].frame=22;
   } else if(map[y*map_width + x].id == SNOW) {
    map[y*map_width + x].frame = 23;
   } else if(map[y*map_width + x].id == WALL) {
    if(x>0 && map[y*map_width + x-1].id == FLOOR || map[y*map_width + x-1].id == TREE) {
     map[y*map_width + x].frame = 9;
    } else if(x<map_width && map[y*map_width + x+1].id == FLOOR || map[y*map_width + x+1].id == TREE) {
     map[y*map_width + x].frame = 11;
    } else {
     map[y*map_width + x].frame = 10;
    }
    if(map[(y-1)*map_width + x].frame == 20) map[y*map_width + x].frame=21;
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
    if(map[(y-1)*map_width + x].id != TOP && map[(y+1)*map_width + x].id != TOP && map[y*map_width + x-1].id != TOP && map[y*map_width + x+1].id != TOP) {
      map[y*map_width + x].frame = 20;
    } else if(map[(y-1)*map_width + x].id != TOP && map[(y+1)*map_width + x].id != TOP) {
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
//floor
void floorPer() {
 int c = 0;
 int wc = 0;
 for(auto tile : map) {
  if(tile.id == FLOOR) c++;
  if(tile.id == WALL) wc++;
 }
 if(wc == 0 || ((c*100)/(map.size()+1) + 1) < 30) {
  /*std::vector<int> seeds;
  std::ifstream inFile;
  inFile.open("res/seeds.txt");
  if (inFile.is_open()) {
   std::copy(std::istream_iterator<double>(inFile), std::istream_iterator<double>(), std::back_inserter(seeds));
   inFile.close();
   seed = seeds[rand() % seeds.size()];
   srand(seed);
   std::cout << "SEED: " << seed << std::endl;
   genMap();
  }*/
 } else {
  std::ofstream out;
  out.open("res/seeds.txt", std::ios::app);
  std::string str = std::to_string(seed) + "\n";
  out << str;
 }
 for(int y = 0; y < map_height; y++) {
  for(int x = 0; x < map_width; x++) {
   if(map[y*map_width + x].id == FLOOR && rand() % 150 == 5) { dropShell(x, y, rand() % 5);
   } else if(map[y*map_width + x].id == SNOW && rand() % 200 == 5) { dropChess(x, y);
   } else if(map[y*map_width + x].id == FLOOR && rand() % 500 == 5) { dropHeart(x, y, rand() % 2); }
  }
 }
 for(int y = 3; y < map_height-4; y++) {
  for(int x = 3; x < map_width-4; x++) {
   if(map[y*map_width + x].id == FLOOR && rand() % 500 == 5) {spawnEnemies(x, y, rand() % 9);}
  }
 }
}


//input
void input() {
    left=right=down=up=fire=0;
    int scroll = 0;
    int select = -1;
    keystates = SDL_GetKeyboardState(NULL);
    while(SDL_PollEvent(&event)) {
     if(event.type == SDL_QUIT) running=false;
     if(event.type == SDL_MOUSEWHEEL) {
      if(event.wheel.y>0) scroll=1;
      if(event.wheel.y<0) scroll=-1;
     }
    }
    if(keystates[SDL_SCANCODE_ESCAPE]) running=false;
    if(keystates[SDL_SCANCODE_W] || keystates[SDL_SCANCODE_UP]) up=1;
    if(keystates[SDL_SCANCODE_S] || keystates[SDL_SCANCODE_DOWN]) down=1;
    if(keystates[SDL_SCANCODE_A] || keystates[SDL_SCANCODE_LEFT]) left=1;
    if(keystates[SDL_SCANCODE_D] || keystates[SDL_SCANCODE_RIGHT]) right=1;
    //if(keystates[SDL_SCANCODE_Q]) freeroam=!freeroam;
    if(keystates[SDL_SCANCODE_1]) select=0;
    if(keystates[SDL_SCANCODE_2]) select=1;
    if(keystates[SDL_SCANCODE_3]) select=2;
    if(keystates[SDL_SCANCODE_4]) select=3;
    if(keystates[SDL_SCANCODE_5]) select=4;
    if(keystates[SDL_SCANCODE_6]) select=5;
    if(keystates[SDL_SCANCODE_7]) select=6;
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

    //hard coded these but could use sizes but dont care atm
    if(ammo>4) ammo=0;
    if(ammo<0) ammo=4;
    if(mod>5) mod=0;
    if(mod<0) mod=5;
    if(mod2>4) mod2=0;
    if(mod2<0) mod2=4;
}

//update
double radToDeg(double a) {
    return a * (180 / PI);
}
double degToRad(double a) {
    return a * (PI / 180);
}
double pointAt(double ax, double ay, double bx, double by) {
    float xDistance = bx - ax;
    float yDistance = by - ay;
    return atan2(yDistance, xDistance);
}
void fireBullet(int id, double sx, double sy, double a, double type, double sw, double v, double dmg, bool bnc) {
    playSound(gunSound);
    bullet.id = id;
    bullet.frame = type;
    bullet.loc.x = sx;
    bullet.loc.y = sy;
    bullet.angle = a;
    bullet.vel = v;
    bullet.src.x = bullet.src.w * type;
    double r = degToRad(a);
    bullet.loc.x += sw * cos(r);
    bullet.loc.y += sw * sin(r);
    bullet.extra = dmg;
    bullet.bExtra = bnc;
    bullet.tick=500;
    bullets.push_back(bullet);
}
void updateBullets() {
    if(fire) {
        if(ammoCount[ammo] > 0) {
            //std::string mods [6] = {"None", "Velocity", "Damage", "Burst", "Wave", "Bounce"};
            //std::string mods2 [6] = {"None", "Velocity", "Damage", "Bounce"};
            int bDmg = 6;
            bool bnc = 0;
            double bVel = 40;
            if(modsUnlocked[mod]) {
                if(mods[mod] == "Velocity") bVel+=10;
                if(mods[mod] == "Damage") bDmg+=8;
                if(mods[mod] == "Bounce") bnc=1;
            }
            if(mods2Unlocked[mod2]) {
            if(mods2[mod2] == "Bounce") bnc=1;
            if(mods2[mod2] == "Velocity") bVel+=10;
            if(mods2[mod2] == "Damage") bDmg+=8;
            if(mods2[mod2] == "Random") {
                int load;
                bool jammed=1;
                while(jammed) {
                    load = rand() % 5;
                    if(ammoCount[load]) {
                        ammo = load;
                        jammed=0;
                    }
                }
            }
            }
            if(ammo == 1) bnc=1;
            if(ammo == 2) bDmg+=8;
            if(ammo == 3) {bDmg+=4;bVel+=4;}
            if(ammo == 3) bVel+=8;
            double px = gun.loc.x + gun.center.x - (bullet.loc.w/2);
            double py = gun.loc.y + gun.center.y - (bullet.loc.h/2);
            fireBullet(PLAYER_ID, px, py, gun.angle, ammo, gun.loc.w*.8, bVel, bDmg, bnc);
            if(mods[mod] == "Burst" && modsUnlocked[mod]) {
                fireBullet(PLAYER_ID, px, py, gun.angle-4, ammo, gun.loc.w*.8, bVel, bDmg, bnc);
                fireBullet(PLAYER_ID, px, py, gun.angle-8, ammo, gun.loc.w*.8, bVel, bDmg, bnc);
                fireBullet(PLAYER_ID, px, py, gun.angle+4, ammo, gun.loc.w*.8, bVel, bDmg, bnc);
                fireBullet(PLAYER_ID, px, py, gun.angle+8, ammo, gun.loc.w*.8, bVel, bDmg, bnc);
            }
            if(mods[mod] == "Wave" && modsUnlocked[mod]) {
                fireBullet(PLAYER_ID, px, py, gun.angle-40, ammo, gun.loc.w*.8, bVel, bDmg, bnc);
                fireBullet(PLAYER_ID, px, py, gun.angle-80, ammo, gun.loc.w*.8, bVel, bDmg, bnc);
                fireBullet(PLAYER_ID, px, py, gun.angle+40, ammo, gun.loc.w*.8, bVel, bDmg, bnc);
                fireBullet(PLAYER_ID, px, py, gun.angle+80, ammo, gun.loc.w*.8, bVel, bDmg, bnc);
            }
            shaking=1;
            ammoCount[ammo]--;
        }
        cursor.frame=1;
    } else {
        if(cursor.frame==2) cursor.frame=0;
        if(cursor.frame==1) cursor.frame=2;
    }
    for(int b=0; b<bullets.size(); b++) {
        double r = degToRad(bullets[b].angle);
        bullets[b].loc.x += bullets[b].vel * cos(r);
        bullets[b].loc.y += bullets[b].vel * sin(r);
        bullets[b].tick--;
        for(auto m:map) {
            if(m.id == TOP && SDL_HasIntersection(&bullets[b].loc, &m.loc)) {
                bullets[b].tick -= 50;
                if(bullets[b].bExtra) {
                    bullets[b].angle+=180;
                    bullets[b].angle+=rand() % 40 - 20;
                }
            }
        }
        if(bullets[b].id != PLAYER_ID && SDL_HasIntersection(&player.loc, &bullets[b].loc)) {
            player.health-=bullets[b].extra;
            bullets[b].tick = -100;
        }
        if(bullets[b].tick < 0) {
            bullets.erase(bullets.begin()+b);
            b--;
        }
    }
    if(fire && !lfire) {
        gun.loc.y-=4;
        if(gun.flipV) {
            gun.loc.x+=16;
        } else {
            gun.loc.x-=16;
        }
    } else if(lfire && !fire) {
        gun.loc.y-=4;
        if(gun.flipV) {
            gun.loc.x+=8;
        } else {
            gun.loc.x-=8;
        }
    }
    lfire = fire;
}
bool inCamView(SDL_Rect loc) {
    return SDL_HasIntersection(&loc, &lens);
}
//enemies
void updateEnemies() {
 /*if(enemies.size() == 0) {
  for(int y = 3; y < map_height-4; y++) {
   for(int x = 3; x < map_width-4; x++) {
    if(map[y*map_width + x].id == FLOOR && rand() % 600 == 5) {spawnEnemies(x, y, rand() % 8);}
   }
  }
 }
 if(shells.size() == 0) {
  for(int y = 0; y < map_height; y++) {
   for(int x = 0; x < map_width; x++) {
    if(map[y*map_width + x].id == FLOOR && rand() % 150 == 5) {dropShell((x)*tile_size - (tile_size/2) + tile_size, (y)*tile_size - (tile_size/2), rand() % 5);
    } else if(map[y*map_width + x].id == FLOOR && rand() % 1000 == 5) {dropChess((x)*tile_size - (tile_size/2) + tile_size, (y)*tile_size - (tile_size/2), 1);
    } else if(map[y*map_width + x].id == FLOOR && rand() % 200 == 5) {dropHeart((x)*tile_size - (tile_size/2) + tile_size, (y)*tile_size - (tile_size/2), rand() % 2);}
   }
  }
 }*/
 for(int e=0; e<enemies.size(); e++) {
  if(enemies[e].health<=0) {
   int dropX = enemies[e].loc.x / tile_size;
   int dropY = enemies[e].loc.y / tile_size;
   dropShell(dropX, dropY, enemies[e].id);
   if(enemies[e].frame == 1) dropChess(dropX, dropY);
   if(rand() % 6 == 1) dropHeart(dropX, dropY, rand() % 2);
   enemies.erase(enemies.begin()+e);
   e--;
  }
  if(inCamView(enemies[e].loc)) {
   float xDistance = round(enemies[e].loc.x + enemy.loc.w/2 - player.loc.x - (player.loc.w/2));
   float yDistance = round(enemies[e].loc.y + enemy.loc.h/2 - player.loc.y - (player.loc.h/2));
   float ang = (atan2(yDistance, xDistance));// * 180 / PI;
   int sp = 10;
   if(enemies[e].frame == 1) sp=20;
   enemies[e].loc.x += sp * cos(ang * 180 / PI);
   enemies[e].loc.y += sp * sin(ang * 180 / PI);
   int bType = enemies[e].id;
   double bnc=0;
   int bDmg = 5;
   int bV = 40;
   if(enemies[e].frame == 1) bnc=1;
   if(bType == 1) bnc=1;
   if(bType == 2) bDmg+=3;
   if(bType == 3) {bDmg+=3;bV+=4;}
   if(bType == 3) bV+=8;
   if(rand() % 30 == 1) {
    //fireBullet(int id, double sx, double sy, double a, double type, double sw, double v, double dmg, bool bnc)
    fireBullet(ENEMY_ID, enemies[e].loc.x + enemy.loc.w/2 + bullet.loc.w/2, 
    enemies[e].loc.y + enemy.loc.h/2 + bullet.loc.h/2, radToDeg(ang)+180, enemies[e].id, 0, bV, bDmg, bnc);
    /*if(enemies[e].frame == 1) {
     int f = rand() % 12;
     if(f==0) {
      fireBullet(enemies[e].loc.x + enemy.loc.w/2 + bulletTmp.loc.w/2, enemies[e].loc.y + enemy.loc.h/2 + bulletTmp.loc.h/2, bV, ang-PI-.04, 2, enemies[e].id, bDmg, bnc);
      fireBullet(enemies[e].loc.x + enemy.loc.w/2 + bulletTmp.loc.w/2, enemies[e].loc.y + enemy.loc.h/2 + bulletTmp.loc.h/2, bV, ang-PI+.04, 2, enemies[e].id, bDmg, bnc);
     } else if(f==1) {
      fireBullet(enemies[e].loc.x + enemy.loc.w/2 + bulletTmp.loc.w/2, enemies[e].loc.y + enemy.loc.h/2 + bulletTmp.loc.h/2, bV, ang, 2, enemies[e].id, bDmg, bnc);
      fireBullet(enemies[e].loc.x + enemy.loc.w/2 + bulletTmp.loc.w/2, enemies[e].loc.y + enemy.loc.h/2 + bulletTmp.loc.h/2, bV, ang-PI/2, 2, enemies[e].id, bDmg, bnc);
      fireBullet(enemies[e].loc.x + enemy.loc.w/2 + bulletTmp.loc.w/2, enemies[e].loc.y + enemy.loc.h/2 + bulletTmp.loc.h/2, bV, ang+PI/2, 2, enemies[e].id, bDmg, bnc);
     }
    }*/
   }
   for(int i=0; i<bullets.size(); i++) {
    enemies[e].src.x=enemies[e].src.w*enemies[e].id;
    if(bullets[i].id==PLAYER_ID && bullets[i].frame!=enemies[e].id) {
     if(SDL_HasIntersection(&bullets[i].loc, &enemies[e].loc)) {bullets[i].tick=0; enemies[e].health-=bullets[i].extra; enemies[e].src.x=enemies[e].src.w*5;}
    }
   }
  }
 }
}
//
void update() {
    if(modsUnlocked[mod]) {gun.src.x = mod * gun.src.w;} else {gun.src.x = 0;}
    gun.src.y = ammo * gun.src.h;
    for(auto c = 0; c < collectables.size(); c++) {
        bool del = 1;
        if(SDL_HasIntersection(&player.loc, &collectables[c].loc)) {
            if(collectables[c].id < 5) { //ammo
                ammoCount[collectables[c].id] += collectables[c].tick;
                if(ammoCount[ammo] == 0) ammo = collectables[c].id;
            } else if(collectables[c].id == 6) { //chess mod 1
                if(rand() % 2) {
                    bool unlock=1;
                    int cnt = 0;
                    bool stuck = 0;
                    while(unlock) {
                        int u = rand() % 5 + 1;
                        if(!modsUnlocked[u]) {modsUnlocked[u]=1; mod=u; unlock=0;}
                        cnt++;
                        if(cnt > 10) {
                            stuck=1;
                            unlock=0;
                        }
                    }
                    if(stuck) {
                            for(int i=0; i<6; i++) {
                                if(!modsUnlocked[i]) {modsUnlocked[i]=1; mod=i;i=1000;}
                            }
                    }
                } else { //chess mod 2
                    bool unlock=1;
                    int cnt = 0;
                    bool stuck = 0;
                    while(unlock) {
                        int u = rand() % 4 + 1;
                        if(!mods2Unlocked[u]) {mods2Unlocked[u]=1; mod2=u; unlock=0;}
                        cnt++;
                        if(cnt > 10) {
                            stuck=1;
                            unlock=0;
                        }
                    }
                    if(stuck) {
                            for(int i=0; i<5; i++) {
                                if(!mods2Unlocked[i]) {mods2Unlocked[i]=1; mod2=i;i=1000;}
                            }
                    }
                }
            } else { //heart
                if(player.health != player.maxHealth) {
                    player.health += (collectables[c].id - 6) * 50;
                } else {
                    del=false;
                }
            }
            if(del) {
                collectables.erase(collectables.begin()+c);
                c--;
            }
        }
    }
    for(int i=0; i < snowFall.size(); i++) {
        snowFall[i].loc.y+=snowFall[i].vel;
        snowFall[i].loc.x-=1.4;
        snowFall[i].tick-=snowFall[i].vel;
        if(snowFall[i].tick<0) {
            snowFall.erase(snowFall.begin()+i);
            i--;
        }
    }
    if(rand() % 500 > 100) {
        snowTmp.loc.x = rand() % WIDTH + lens.x;
        snowTmp.loc.w = rand() % 10 + 7;
        snowTmp.loc.h = snowTmp.loc.w * 1.3;
        snowTmp.loc.y = 0 - snowTmp.loc.h + lens.y;
        snowTmp.vel = rand() % 12 + 6;
        snowTmp.tick = rand() % HEIGHT*3 + 220;
        snowFall.push_back(snowTmp);
    }
    //setCamera(collectables[0]);
    lens.x=camera.x-(lens.w/2);
    lens.y=camera.y-(lens.h/2);
    //playMusic(song);
    bool inSnow = 0;
    if(player.health > player.maxHealth) player.health=player.maxHealth;
    if(player.health <=0) {player.health=0;player.alive=false;}
    if(freeroam) {
        if(up) camera.y-=20;
        if(down) camera.y+=20;
        if(left) camera.x-=20;
        if(right) camera.x+=20;
    } else {
        healthColor = red;
        player.vel = 16;
        for(auto m : map) {
            if(m.id == SNOW && SDL_HasIntersection(&player.loc, &m.loc)) {
                player.vel = 4;
                player.health-=0.05;
                healthColor = blue;
                inSnow=1;
            }
        }
        if(left || right || up || down) {
            footTick++;
            player.collideH=player.collideV=player.bExtra=false;
            if(left) player.loc.x-=player.vel;
            if(right) player.loc.x+=player.vel;
            double slide;
            for(auto m : map) {
                if(m.id == TOP && SDL_HasIntersection(&player.loc, &m.loc)) {
                    player.collideH = true;
                    slide = player.loc.x + player.loc.w - m.loc.x;
                    if(left) slide = m.loc.x + m.loc.w - player.loc.x;
                }
                if(m.id == WALL && SDL_HasIntersection(&player.loc, &m.loc)) player.bExtra=true; //wallcollision
            }
            if(player.collideH) {
                if(left) player.loc.x+=slide;
                if(right) player.loc.x-=slide;
            }
            if(up) player.loc.y-=player.vel;
            if(down) player.loc.y+=player.vel;
            slide = 0;
            for(auto m : map) {
                if(m.id == TOP && SDL_HasIntersection(&player.loc, &m.loc)) {
                    player.collideV = true;
                    slide = player.loc.y + player.loc.h - m.loc.y;
                    if(up) slide = m.loc.y + m.loc.h - player.loc.y;
                }
            }
            if(player.collideV) {
                if(up) player.loc.y+=slide+1;
                if(down) player.loc.y-=slide;
            }
            //walking/running animation
            int animCycle=3;//0//1//2//3
            int animLength=6;//8//3//3//6
            player.src.y=player.src.h*animCycle;
            player.tick+=28;
            if(inSnow)player.tick-=16;
            if(player.tick>animLength*100-101) player.tick=0;
            player.src.x = (round(player.tick/100)+3) * player.src.w;
            if((right && player.flipH) || (left && !player.flipH)) player.src.x = (player.src.w*(animLength+3)) - ((round(player.tick/100)+3) * player.src.w);
        } else {
            footTick--;
            //idle animation
            player.tick+=3;
            if(player.tick>199) player.tick=0;
            player.src.x = round(player.tick/200) * player.src.w;
        }
        if(footTick>3) {
            footTick=0;
            footTmp.loc.x = player.loc.x + (player.loc.w/2) - (footTmp.loc.w/2);
            footTmp.loc.y = player.loc.y + player.loc.h - footTmp.loc.h;
            footTmp.angle = gun.angle;
            if(footTmp.src.x==0) {
                footTmp.src.x=footTmp.src.w;
            } else {
                footTmp.src.x=0;
            }
            if(!player.bExtra && !player.collideV && !player.collideH) footPrints.push_back(footTmp);
        }
        if(footTick<0)footTick=0;
        for(int f=0; f<footPrints.size(); f++) {
            footPrints[f].tick--;
            if(footPrints[f].tick<200)footPrints[f].src.y=footPrints[f].src.h * 1;
            if(footPrints[f].tick<100)footPrints[f].src.y=footPrints[f].src.h * 2;
            if(footPrints[f].tick<40)footPrints[f].src.y=footPrints[f].src.h * 3;
            if(footPrints[f].tick<0) {
                footPrints.erase(footPrints.begin()+f);
                f--;
            }
        }
        //setCamera(player);
    }

    cursor.loc.x=mouse.x-cursor.loc.w/2;cursor.loc.y=mouse.y-cursor.loc.h/2;
    player.flipH = 1;
    if(mouse.x > player.loc.x - lens.x) player.flipH=0;
    gun.loc = initRect(player.loc.x, player.loc.y + player.loc.h*.35, gun.loc.w, gun.loc.h);
    gun.flipH = player.flipH;
    if(gun.flipH) {
        gun.loc.x += player.loc.w*.6;
        gun.center.x = 0;
        gun.flipV = 1;
    } else {
        gun.loc.x += player.loc.h*.3;
        gun.center.x = 0;
        gun.flipV = 0;
    }
    gun.angle = radToDeg(pointAt(gun.loc.x + gun.center.x - (bullet.loc.w/2) - lens.x, gun.loc.y + gun.center.y - (bullet.loc.h/2) - lens.y, mouse.x, mouse.y));
    cursor.angle = gun.angle;
    updateEnemies();
    updateBullets();
    if(dayCycle) {
        dayClock+=4;
        if(dayClock>WIDTH*2*4) {
            dayCycle=!dayCycle;
        }
    } else {
        dayClock-=4;
        if(dayClock<=-WIDTH*4) {
            dayCycle=!dayCycle;
        }
    }
    lighting.loc = lens;
    lighting.loc.x-=dayClock;
    lighting.loc.y-=dayClock/2;
    lighting.loc.w=WIDTH + (2*(dayClock));
    lighting.loc.h=HEIGHT + (1*(dayClock));
    if(lighting.loc.w<WIDTH) lighting.loc = lens;
    setCamera(player);
}
//bullets
//enemies
//player
//snow
//world
//pickups

//render
//write
void write(std::string t, int x, int y) {
 SDL_Surface *text_surface;
 SDL_Texture *text_texture;
 SDL_Rect wrect;
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
//images
std::vector<SDL_Texture*> images;
int setImage(std::string filename) {
 images.push_back(IMG_LoadTexture(renderer, filename.c_str()));
 return images.size()-1;
}
//drawing

void draw(tile t) {
    if(inCamView(t.loc)) {
        SDL_Rect dest, src;
        dest = t.loc;
        dest.x-=lens.x;
        dest.y-=lens.y;
        src.w=src.h=20;
        src.y=0;
        src.x=src.w*t.frame;
        if(t.flip) {
            SDL_RenderCopyEx(renderer, images[tileImgId], &src, &dest, 0, NULL, SDL_FLIP_HORIZONTAL);
        } else {
            SDL_RenderCopyEx(renderer, images[tileImgId], &src, &dest, 0, NULL, SDL_FLIP_NONE);
        }
        if(debug) {
            SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &dest);
            write(std::to_string(src.x) + ", " +  std::to_string(src.y), dest.x + 20, dest.y + 20);
            write(std::to_string(t.id) + " - " +  std::to_string(t.frame), dest.x + 20, dest.y + 40);
        }
    }
}
void draw(obj o) {
    if(inCamView(o.loc)) {
        SDL_Rect dest;
        dest = o.loc;
        dest.x-=lens.x;
        dest.y-=lens.y;
        SDL_RendererFlip flip = SDL_FLIP_NONE;
        if(o.flipH) flip = SDL_FLIP_HORIZONTAL;
        if(o.flipV) flip = SDL_FLIP_VERTICAL;
        if(o.rotateOnCenter) {
            SDL_RenderCopyEx(renderer, images[o.img], &o.src, &dest, o.angle, &o.center, flip);
        } else {
            SDL_RenderCopyEx(renderer, images[o.img], &o.src, &dest, o.angle, NULL, flip);
        }
        if(o.parent) {
            for(int c=0; c<o.children.size(); c++) {
                draw(*o.children[c]);
            }
        }
        if(debug) {
            SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &dest);
            write(std::to_string(o.src.x) + ", " +  std::to_string(o.src.y), dest.x + 20, dest.y + 20);
            write(std::to_string(o.id) + " - " +  std::to_string(o.frame), dest.x + 20, dest.y + 40);
        }
    }
}
void drawDebug(obj o) {
    debug=1;
    draw(o);
    debug=0;
}
void draw(std::vector<obj> os) {
    for(auto o:os) draw(o);
}
void drawDebug(std::vector<obj> os) {
    for(auto o:os) drawDebug(o);
}
void drawRectUpfront(SDL_Rect r, SDL_Color c) {
 SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
 SDL_RenderFillRect(renderer, &r);
}
void drawUpfront(obj o) {
    SDL_RendererFlip flip = SDL_FLIP_NONE;
    if(o.flipH) flip = SDL_FLIP_HORIZONTAL;
    if(o.flipV) flip = SDL_FLIP_VERTICAL;
    if(o.rotateOnCenter) {
        SDL_RenderCopyEx(renderer, images[o.img], &o.src, &o.loc, o.angle, &o.center, flip);
    } else {
        SDL_RenderCopyEx(renderer, images[o.img], &o.src, &o.loc, o.angle, NULL, flip);
    }
    if(debug) {
        SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &o.loc);
        write(std::to_string(o.src.x) + ", " +  std::to_string(o.src.y), o.loc.x + 20, o.loc.y + 20);
        write(std::to_string(o.id) + " - " +  std::to_string(o.frame), o.loc.x + 20, o.loc.y + 40);
    }
}
void drawMap() {
    for(auto m : map) {
        draw(m);
    }
}
void drawUI() {
    healthBar.w = maxHealthBar * (player.health / player.maxHealth);
    drawRectUpfront(healthBar, healthColor);
    gunUI.src=gun.src;
    shellSelect.src.y = shellSelect.src.h * ammo;
    drawUpfront(UI);
    drawUpfront(gunUI);
    drawUpfront(shellSelect);
    if(changingMod) drawUpfront(modSelect);
    for(int a=0; a < 5; a++) {
        write(std::to_string(ammoCount[a]), 220, a*23 + 35);
    }
    if(modsUnlocked[mod]) {write(mods[mod], 193, 6*23 + 16);} else {write("Locked", 193, 6*23 + 16);}
    if(mods2Unlocked[mod2]) {write(mods2[mod2], 193, 7*23 + 20);} else {write("Locked", 193, 7*23 + 20);}
    write(std::to_string(fps), 22, 5*40 + 28);
    write(std::to_string(camera.x) + ", " + std::to_string(camera.y), 22, 6*40 + 16);
    //SDL_RenderDrawLine(renderer, gun.loc.x + gun.center.x - (bullet.loc.w/2) - lens.x, gun.loc.y + gun.center.y - (bullet.loc.h/2) - lens.y, mouse.loc.x, mouse.loc.y);
}
std::vector<obj> buffer, organizedBuffer;
int bufLow, bufHigh;
void drawToBuffer(obj o) {
    if(o.loc.y+o.loc.h > bufHigh) bufHigh=o.loc.y+o.loc.h;
    if(o.loc.y+o.loc.h < bufLow || bufLow==-99) bufLow=o.loc.y+o.loc.h;
    buffer.push_back(o);
}
void drawBuffer() {
    for(int y=bufLow-500; y<bufHigh+500; y++) {
        for(int b=0; b<buffer.size(); b++) {
            if(buffer[b].loc.y+buffer[b].loc.h==y) {
                organizedBuffer.push_back(buffer[b]);
                if(buffer[b].parent) {
                    for(int c=0; c<buffer[b].children.size(); c++) {
                        organizedBuffer.push_back(*buffer[b].children[c]);
                    }
                }
            }
        }
    }
    draw(organizedBuffer);
    buffer.clear();
    organizedBuffer.clear();
    bufLow=bufHigh=-99;
}
//

void render() {
 SDL_SetRenderTarget(renderer, screen);
 SDL_SetRenderDrawColor(renderer, bkg.r, bkg.g, bkg.b, bkg.a);
 SDL_RenderClear(renderer);
 frameCount++;
 timerFPS = SDL_GetTicks()-lastFrame;
 if(timerFPS<(1000/setFPS)) {
  SDL_Delay((1000/setFPS)-timerFPS);
 }

 //drawMap
 drawMap();
 draw(footPrints);
 //drawtrees -> buffer
 int treeCnt=0;
 //buffer.clear();
 for(auto m:map) {
     int shift = 0;
     if((treeCnt / map_width) % 2) shift+=tile_size/2;
     if(m.id == TREE && inCamView(m.loc)) {
         treeObj.loc = initRect(m.loc.x+shift,m.loc.y-(tile_size*.55),tile_size,tile_size*1.5);
         treeObj.src = initRect(43*m.tick,0,43,76);
         treeObj.flipH = m.flip;
         drawToBuffer(treeObj);
     }
     treeCnt++;
 }
 //drawplayer -> buffer
 drawToBuffer(player);
 //draw(gun);
 //drawenemy -> buffer
 for(auto e:enemies) draw(e);
 draw(bullets);
 draw(collectables);
 //draw(buffer);
 drawBuffer();
 draw(snowFall);
 if(lighting.loc.w<=WIDTH*4) draw(lighting);
 drawUI();
 cursor.src.x = cursor.frame * cursor.src.w;
 drawUpfront(cursor);
 //write("hello world", WIDTH/2, HEIGHT/2);
 //write(std::to_string(bullets.size()), WIDTH/2, HEIGHT/2);

 SDL_SetRenderTarget(renderer, NULL);
 if(shaking) {
  if(shake) {
   screendest.x+=3;
   screendest.y+=3;
  } else {
   screendest.x-=3;
   screendest.y-=3;
  }
  shake=!shake;
  shaketick++;
 }
 if(shaketick > 3) {
  shaking=0;
  shaketick=0;
  screendest=screensrc;
 }
 SDL_RenderCopy(renderer, screen, &screensrc, &screendest);

 SDL_RenderPresent(renderer);
}
//draw

//init
int initAudio() {
 SDL_Init(SDL_INIT_AUDIO);
 if(Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 ) < 0) {
  printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
  return -1;
 }
 gunSound = loadSound("res/ray.wav");
 //song = loadMusic("res/cold.wav"); //https://www.youtube.com/watch?v=eQyg8MBQQog
 return 0;
}
void initImages() {
    tileImgId = setImage("res/tiles.png");
    player.img = setImage("res/player.png");
    cursor.img = setImage("res/cursor.png");
    gun.img = setImage("res/raygun.png");
    UI.img = setImage("res/UI.png");
    shellSelect.img = setImage("res/select.png");
    modSelect.img = shellSelect.img;
    treeObj.img = setImage("res/tree.png");
    footTmp.img = setImage("res/footprints.png");
    snowTmp.img = setImage("res/snow.png");
    chess.img = setImage("res/chess.png");
    heart.img = setImage("res/health.png");
    shell.img = setImage("res/shells.png");
    bullet.img = setImage("res/bullets.png");
    lighting.img = setImage("res/lighting2.png");
    enemy.img = setImage("res/enemy.png");
    enemyShadow.img = enemy.img;
}
void initObjs() {
    player.src = initRect(0,0,7,10);
    player.loc = initRect(player.loc.x, player.loc.y, tile_size-8, tile_size*1.2);
    player.health=460; player.maxHealth=500;
    player.vel=16;
    player.alive=1;
    cursor.src = initRect(0, 0, 18, 18);
    cursor.loc = initRect(0, 0, tile_size, tile_size);
    gun.src = initRect(0, 0, 14, 5);
    gun.loc = initRect(0, 0, tile_size*1.1, (tile_size*1.1)/2);
    gun.rotateOnCenter=1;
    gun.center.x=0;gun.center.y=gun.loc.h*.5;
    player.parent=1;
    player.children.push_back(&gun);
    footTmp.loc = initRect(0, 0, 50, 60);
    footTmp.src = initRect(0, 0, 10, 15);
    footTmp.tick = 300;
    snowTmp.src = initRect(0, 0, 7, 8);
    snowTmp.loc = initRect(0, 0, 17, 18);

    enemy.src = initRect(0, 0, 12, 12);
    enemy.loc = initRect(0, 0, player.loc.w, player.loc.w);
    enemyShadow = enemy;
    enemyShadow.src.x = enemyShadow.src.w * 6;

    bullet.src = initRect(0, 0, 8, 6);
    bullet.loc = initRect(0, 0, 40, 25);

    chess.src = initRect(0,0,10,10);
    chess.loc = initRect(0,0,50,50);
    heart.src = initRect(0,0,10,10);
    heart.loc = initRect(0,0,50,50);
    shell.src = initRect(0,0,8,7);
    shell.loc = initRect(0,0,30,20);

    lighting.src = initRect(0, 0, 640, 360);
    lighting.loc = initRect(0, 0, WIDTH, HEIGHT);
    dayClock = WIDTH;
    dayCycle = 1;

    gunUI = gun;
    gunUI.angle = -20;
    gunUI.loc.x=43;gunUI.loc.y=96;
    gunUI.loc.w=gun.loc.w*1.5;gunUI.loc.h=gun.loc.h*1.5;
    UI.src.x=UI.src.y=0;
    UI.src.w=220;UI.src.h=154;
    UI.loc.x=UI.loc.y=20;
    UI.loc.w=250;UI.loc.h=200;
    healthBar.x=30;
    healthBar.y=180;
    maxHealthBar=UI.loc.w*.6;
    healthBar.w=maxHealthBar;
    healthBar.h=40;
    int SSIMG = shellSelect.img;
    shellSelect = UI;
    shellSelect.img = SSIMG;
    modSelect = shellSelect;

    screensrc = initRect(0,0,WIDTH,HEIGHT);
    screendest = screensrc;
    screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, WIDTH, HEIGHT);
    shake=shaking=0;
}
void init() {
 seed = time(NULL);
 srand(seed);
 SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
 if(SDL_Init(SDL_INIT_EVERYTHING) < 0) std::cout << "Failed at SDL_Init()" << std::endl;
 SDL_GetCurrentDisplayMode(0, &DM);
 WIDTH=DM.w;
 HEIGHT=DM.h;
 window = SDL_CreateWindow(TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
 renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
 SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
 SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
 TTF_Init();
 font_size = 16;
 font = TTF_OpenFont("res/font.ttf", font_size);
 if(font == NULL) std::cout << "Failed to load font" << std::endl;
 SDL_ShowCursor(SDL_DISABLE);
 initAudio();
 genMap();
 running = 1;
 setBkg(51, 73, 95);
 lens.w=WIDTH;
 lens.h=HEIGHT;
 camera.x=WIDTH/2; camera.y=HEIGHT/2;
 initImages();
 initObjs();
 floorPer();
 font_color = white;
}

//quit
void quitSounds() {
 for(int s=0; s<sounds.size(); s++) {
  Mix_FreeChunk(sounds[s]);
  sounds[s]=NULL;
 }
 for(int m=0; m<music.size(); m++) {
  Mix_FreeMusic(music[m]);
  music[m]=NULL;
 }
 Mix_Quit();
}
int quit() {
 quitSounds();
 TTF_CloseFont(font);
 SDL_DestroyRenderer(renderer);
 SDL_DestroyWindow(window);
 SDL_Quit();
 return 0;
}

//main
int main() {
    init();
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
      //std::cout << "Enemies: " << enemies.size() << std::endl;
      //std::cout << "Bullets: " << bullets.size() << std::endl;
      //std::cout << "Collectables: " << collectables.size() << std::endl;
      //std::cout << "Snowfall: " << snowFall.size() << std::endl;
      //std::cout << fps << std::endl << std::endl;
    }
    return quit();
}
