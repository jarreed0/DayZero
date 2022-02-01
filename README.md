# DayZero - a bullet hell game
making a bullet hell game in a month (Jan 2022) - SDL2 - procedural generated maps

(Tmp) music file too large, download it [here](http://jar.ylimaf.com/cold.wav) and put it in the res/ directory and then uncomment lines 866 and 1217 in the proj.cpp.

Tested on Linux. Use make to build. Still working on porting the game.

Dependencies: SDL2, SDL2_ttf, SDL2_mixer, SDL2_image

[Devlog 1](https://www.youtube.com/watch?v=76DXj4hbBoE)

[Devlog 2](https://youtu.be/hA2H1nx99Zc)

[Devlog 3](https://youtu.be/f-4PXaSwO-8)

Devlog 4 coming soon!


Known bugs

-Trees moving? I think this is gone

-Something was calling draw() with wrong params, didnt fix but I stopped the wrong params from going through

-Map gen is correct, but fixed maps dont replace broken ones

-Getting stuck between wall and ice
