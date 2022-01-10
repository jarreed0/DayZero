# TinyEngine Makefile
# Compiler Configuration

CC = g++ -std=c++17
CF = -Wfatal-errors -O
LF = -I/usr/local/include -L/usr/local/lib

# General Linking

LINKS = -lpthread -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -lboost_system -lboost_filesystem

# OS Specific Linking

UNAME := $(shell uname)
ifeq ($(UNAME), Linux)			#Detect GNU/Linux
TINYOS = -lX11 -lGL
endif
ifeq ($(UNAME), Darwin)			#Detext MacOS
TINYOS = -framework OpenGL
endif

all: proj.cpp
			$(CC) proj.cpp $(CF) $(LF) $(TINYOS) $(LINKS) -o game

run: proj.cpp
			$(CC) proj.cpp $(CF) $(LF) $(TINYOS) $(LINKS) -o game
			./game
