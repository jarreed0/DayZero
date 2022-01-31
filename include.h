#if __has_include(<SDL2/SDL.h>)
 #include <SDL2/SDL.h>
 #include <SDL2/SDL_image.h>
 #include <SDL2/SDL_ttf.h>
 #include <SDL2/SDL_mixer.h>
#else
 #include </usr/local/win64/x86_64-w64-mingw32/include/SDL/SDL.h>
 #include </usr/local/win64/x86_64-w64-mingw32/include/SDL/SDL_image.h>
 #include </usr/local/win64/x86_64-w64-mingw32/include/SDL/SDL_ttf.h>
 #include </usr/local/win64/x86_64-w64-mingw32/include/SDL/SDL_mixer.h>
#endif

#include <cstdlib>
#include <iostream>

#include <fstream>
#include <iterator>

#include <vector>
#include <cmath>