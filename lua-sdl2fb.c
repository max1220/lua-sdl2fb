#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#include <stdint.h>
#include <fcntl.h>
#include <string.h>

#include <sys/types.h>

#include "lua.h"
#include "lauxlib.h"

#include <SDL2/SDL.h>
#include "lua-db.h"

#define VERSION "0.1"

#define LUA_T_PUSH_S_N(S, N) lua_pushstring(L, S); lua_pushnumber(L, N); lua_settable(L, -3);
#define LUA_T_PUSH_S_I(S, N) lua_pushstring(L, S); lua_pushinteger(L, N); lua_settable(L, -3);
#define LUA_T_PUSH_S_B(S, N) lua_pushstring(L, S); lua_pushboolean(L, N); lua_settable(L, -3);
#define LUA_T_PUSH_S_S(S, S2) lua_pushstring(L, S); lua_pushstring(L, S2); lua_settable(L, -3);
#define LUA_T_PUSH_S_CF(S, CF) lua_pushstring(L, S); lua_pushcfunction(L, CF); lua_settable(L, -3);

#define CHECK_SDL2FB(L, I, D) D=(sdl_framebuffer_t *)luaL_checkudata(L, I, "sdl2fb"); if (D==NULL) { lua_pushnil(L); lua_pushfstring(L, "Argument %d must be a sdl2fb", I); return 2; }



typedef struct {
    int fd;
    char *fbdev;
    SDL_Window *window;
	SDL_Surface *screen;
	uint16_t w;
	uint16_t h;
} sdl_framebuffer_t;


static inline void set_pixel(SDL_Surface *surface, int x, int y, uint32_t pixel) {
	uint32_t *target_pixel = surface->pixels + y*surface->pitch + x*sizeof(*target_pixel);
	*target_pixel = pixel;
}


static int sdl2fb_close(lua_State *L) {
	
	sdl_framebuffer_t *sdl2fb;
	CHECK_SDL2FB(L, 1, sdl2fb)
	
	SDL_Window *window = sdl2fb->window;
	SDL_DestroyWindow(window);
	
    return 0;
}


static int sdl2fb_pool_event(lua_State *L) {
	
	sdl_framebuffer_t *sdl2fb;
	CHECK_SDL2FB(L, 1, sdl2fb)
	
	SDL_Event ev;
	if (SDL_PollEvent(&ev) == 0) {
		return 0;
	}
	
	lua_newtable(L);
	
	//serialize event
	switch (ev.type) {
		case SDL_KEYDOWN:
			LUA_T_PUSH_S_S("type", "keydown")
			LUA_T_PUSH_S_S("scancode", SDL_GetScancodeName(ev.key.keysym.scancode))
			LUA_T_PUSH_S_S("key", SDL_GetKeyName(ev.key.keysym.sym))
			break;
		case SDL_KEYUP:
			LUA_T_PUSH_S_S("type", "keyup")
			LUA_T_PUSH_S_S("scancode", SDL_GetScancodeName(ev.key.keysym.scancode))
			LUA_T_PUSH_S_S("key", SDL_GetKeyName(ev.key.keysym.sym))
			break;
		case SDL_MOUSEMOTION:
			LUA_T_PUSH_S_S("type", "mousemotion")
			LUA_T_PUSH_S_I("x", ev.motion.x)
			LUA_T_PUSH_S_I("y", ev.motion.y)
			LUA_T_PUSH_S_I("xrel", ev.motion.xrel)
			LUA_T_PUSH_S_I("yrel", ev.motion.yrel)
			
			lua_pushstring(L, "buttons");
			lua_newtable(L);
			if (ev.motion.state & SDL_BUTTON(SDL_BUTTON_LEFT)) {
				LUA_T_PUSH_S_B("left", 1)
			}
			if (ev.motion.state & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
				LUA_T_PUSH_S_B("right", 1)
			}
			if (ev.motion.state & SDL_BUTTON(SDL_BUTTON_MIDDLE)) {
				LUA_T_PUSH_S_B("middle", 1)
			}
			lua_settable(L, -3);
			
			break;
		case SDL_MOUSEBUTTONDOWN:
			LUA_T_PUSH_S_S("type", "mousebuttondown")
			break;
		case SDL_MOUSEBUTTONUP:
			LUA_T_PUSH_S_S("type", "mousebuttonup")
			break;
		case SDL_QUIT:
			LUA_T_PUSH_S_S("type", "quit")
			break;
		default:
			LUA_T_PUSH_S_I("type", ev.type)
			
			break;
	}
	
    return 1;
}


static int sdl2fb_tostring(lua_State *L) {
	
    sdl_framebuffer_t *sdl2fb;
	CHECK_SDL2FB(L, 1, sdl2fb)

	lua_pushfstring(L, "SDL2 Framebuffer: %dx%d", sdl2fb->w, sdl2fb->h);

    return 1;
    
}


static int sdl2fb_draw_from_drawbuffer(lua_State *L) {
    
    sdl_framebuffer_t *sdl2fb;
	CHECK_SDL2FB(L, 1, sdl2fb)
    
    drawbuffer_t *db;
	CHECK_DB(L, 2, db)
	 
	SDL_Window *window = sdl2fb->window;
	SDL_Surface *screen = sdl2fb->screen;
	
    int x = lua_tointeger(L, 3);
    int y = lua_tointeger(L, 4);
    int cx;
    int cy;
    pixel_t p;
    uint32_t pixel;

	SDL_LockSurface(screen);

    for (cy=0; cy < db->h; cy=cy+1) {
        for (cx=0; cx < db->w; cx=cx+1) {
            p = db->data[cy*db->w+cx];
			pixel = SDL_MapRGBA(screen->format, p.r, p.g, p.b, p.a);
            
            if (x+cx < 0 || y+cy < 0 || x+cx > sdl2fb->w || y+cy > sdl2fb->h || p.a < 1) {
                continue;
            } else {
                // set pixel
				set_pixel(screen, cx+x,cy+y, pixel);
            }
        }
    }
    
	SDL_UnlockSurface(screen);
    
    SDL_UpdateWindowSurface(window);

    return 0;
    
}


static int sdl2fb_new(lua_State *L) {
    int w = lua_tointeger(L, 1);
	int h = lua_tointeger(L, 2);
	const char *title = lua_tostring(L, 3);

	sdl_framebuffer_t *sdl2fb = (sdl_framebuffer_t *)lua_newuserdata(L, sizeof(sdl_framebuffer_t));

	SDL_Window *window;
	SDL_Surface *screen;

	SDL_Init(SDL_INIT_VIDEO);
	window = SDL_CreateWindow(title,0, 0, w, h, 0);
	screen = SDL_GetWindowSurface(window);

	sdl2fb->window = window;
	sdl2fb->screen = screen;
	sdl2fb->w = w;
	sdl2fb->h = h;

    if (luaL_newmetatable(L, "sdl2fb")) {
		
		lua_pushstring(L, "__index");
		lua_newtable(L);

		LUA_T_PUSH_S_CF("close", sdl2fb_close)
		LUA_T_PUSH_S_CF("draw_from_drawbuffer", sdl2fb_draw_from_drawbuffer)
		LUA_T_PUSH_S_CF("pool_event", sdl2fb_pool_event)
		
		lua_settable(L, -3);
		
		LUA_T_PUSH_S_CF("__gc", sdl2fb_close)
		LUA_T_PUSH_S_CF("__tostring", sdl2fb_tostring)
	}
    lua_setmetatable(L, -2);

    return 1;
    
}



LUALIB_API int luaopen_sdl2fb(lua_State *L) {
    lua_newtable(L);

    LUA_T_PUSH_S_S("version", VERSION)
    LUA_T_PUSH_S_CF("new", sdl2fb_new)

    return 1;
}
