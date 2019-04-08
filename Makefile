CFLAGS = -O3 -fPIC -I/usr/include/lua5.1 -Wall -Wextra
LIBS   = -shared -llua5.1 -lSDL2
TARGET = sdl2fb.so

all: $(TARGET)

$(TARGET): lua-sdl2fb.c lua-db.h
	$(CC) -o $(TARGET) lua-sdl2fb.c $(CFLAGS) $(LIBS)
	strip $(TARGET)

clean:
	rm -f $(TARGET)
