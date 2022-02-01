ifeq ($(os),windows)
	TARGET ?= DayZero.exe

	CC := x86_64-w64-mingw32-g++
	CFLAGS := --std=c++17 -w -fpermissive -I/usr/local/win64/x86_64-w64-mingw32/include
	LDFLAGS := -static -static-libgcc
	LIBS := `/usr/local/win64/x86_64-w64-mingw32/bin/sdl2-config --static-libs`
else
	TARGET ?= DayZero

	CC := g++
	CFLAGS := --std=c++17 -w -fpermissive `pkg-config --cflags sdl2 SDL2_image SDL2_ttf SDL2_mixer`
	LDFLAGS :=
	LIBS := `pkg-config --libs sdl2 SDL2_image SDL2_ttf SDL2_mixer`
endif

OBJECTS := \
	obj/proj.o

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@ $(LIBS)

obj/%.o: %.cpp
	$(CC) $(CFLAGS) $< -c -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: clean
