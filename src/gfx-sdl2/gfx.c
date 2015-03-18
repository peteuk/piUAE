/*
 * PUAE - The Un*x Amiga Emulator
 *
 *
 *
 * Copyright 2015 Mustafa 'GnoStiC' TUFAN
 *
 */

#include "sysconfig.h"
#include "sysdeps.h"
#include "../keymap/keymap.h"
#include "misc.h"
#include "cfgfile.h"
#include "uae.h"
#include "xwin.h"
#include "custom.h"
#include "drawing.h"
#include "keyboard.h"
#include "keybuf.h"
#include "gui.h"
#include "debug.h"
#include "picasso96.h"
#include "inputdevice.h"
#include "hotkeys.h"
#include <SDL2/SDL.h>

TCHAR config_filename[256] = "";
Uint32 *frameBuffer;

SDL_Window *sdlWindow = NULL;
SDL_Renderer *sdlRenderer = NULL;
SDL_Texture *sdlTexture = NULL;

int DX_Fill (int dstx, int dsty, int width, int height, uae_u32 color, RGBFTYPE rgbtype) {
}

void DX_Invalidate (int x, int y, int width, int height) {
}

int check_prefs_changed_gfx (void) {
  return 0;
}

int debuggable (void) {
  return 1;
}

uae_u8 *gfx_lock_picasso (bool fullupdate, bool doclear) {
}

void gfx_set_picasso_modeinfo (uae_u32 w, uae_u32 h, uae_u32 depth, RGBFTYPE rgbfmt) {
}

void gfx_set_picasso_colors (RGBFTYPE rgbfmt) {
}

void gfx_set_picasso_state (int on) {
}

void gfx_unlock_picasso (bool dorender) {
}


int graphics_setup (void) {

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		write_log ("SDL2: error initalizing SDL: %s\n", SDL_GetError() );
		return 0;
	} else {
		const SDL_version	*version = SDL_Linked_Version ();
		const SDL_VideoInfo *info    = SDL_GetVideoInfo ();

		write_log ("SDL2: Using SDL version: %d.%d.%d.\n", version->major, version->minor, version->patch);
		return 1;
	}
}

/* copy image to texture
	SDL_UpdateTexture(sdlTexture, NULL, myPixels, 640 * sizeof (Uint32));
	SDL_RenderClear(sdlRenderer);
	SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);
	SDL_RenderPresent(sdlRenderer);
*/

int graphics_init (void) {
/*
	SDL_Rect sdldest_rect;
    dest_rect.x = 0; dest_rect.y = 0;
    dest_rect.w = 10; dest_rect.h = 10;
	SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &dest_rect);
*/
// 720*568

	gfxvidinfo.width = currprefs.gfx_size_fs.width = 800;
	gfxvidinfo.height = currprefs.gfx_size_fs.height = 600;
	gfxvidinfo.emergmem = 0;
	gfxvidinfo.linemem = 0;
	gfxvidinfo.maxblocklines = MAXBLOCKLINES_MAX;
	gfxvidinfo.lockscr = sdl2_lockscr;
	gfxvidinfo.unlockscr = sdl2_unlockscr;
	gfxvidinfo.flush_line = sdl2_flush_line;
	gfxvidinfo.flush_block = sdl2_flush_block;
	gfxvidinfo.flush_clear_screen = sdl2_flush_clear_screen;

	sdlWindow = SDL_CreateWindow("PUAE", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600);
	if (sdlWindow == NULL) {
		write_log ("SDL2: couldn't create window: %s\n", SDL_GetError() );
		return 1;
	}
	sdlRenderer = SDL_CreateRenderer (sdlWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	sdlTexture = SDL_CreateTexture (sdlWindow, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 800, 600);

	reset_drawing ();
	init_colors ();

	write_log ("Screen width	: %d\n", gfxvidinfo.width);
	write_log ("Screen height	: %d\n", gfxvidinfo.height);
	write_log ("Bytes per pixel	: %d\n", gfxvidinfo.pixbytes);
	write_log ("Bytes per line	: %d\n", gfxvidinfo.rowbytes);
	write_log ("Buffer address	: %p\n", gfxvidinfo.bufmem);

	return 1;
}

void graphics_leave (void) {
	SDL_DestroyTexture(sdlTexture);
	SDL_DestroyRenderer(sdlRenderer);
	SDL_DestroyWindow(sdlWindow);
}

bool handle_events (void) {
	SDL_Event sdlEvent = { SDL_NOEVENT };

    while (SDL_PollEvent (&sdlEvent)) {

		switch (sdlEvent.type) {
			// system events
			case SDL_QUIT:
				break;

			// key events
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				switch (sdlEvent.key.keysym.sym) {
					case SDLK_UP: break;
				}
				int keycode = sdlEvent.key.keysym.sym;
				int state = (sdlEvent.type == SDL_KEYDOWN);

				di_keycodes[0][keycode] = state;
				my_kbd_handler (0, sdlk2dik (keycode), state);
				break;

			// mouse events
			case SDL_MOUSEBUTTONDOWN:
				int buttonno = -1;
				int state = (sdlEvent.type == SDL_MOUSEBUTTONDOWN);
				switch (sdlEvent.button.button) {
					case SDL_BUTTON_LEFT:	buttonno = 0; break;
					case SDL_BUTTON_RIGHT:	buttonno = 1; break;
					case SDL_BUTTON_MIDDLE:	buttonno = 2; break;
				}
				if (buttonno >= 0) {
					setmousebuttonstate (0, buttonno, state);
				}
				break;

			case SDL_MOUSEMOTION:
				if (!fullscreen && !mousegrab) {
					setmousestate (0, 0, sdlEvent.motion.x, 1);
					setmousestate (0, 1, sdlEvent.motion.y, 1);
				} else {
					setmousestate (0, 0, sdlEvent.motion.xrel, 0);
					setmousestate (0, 1, sdlEvent.motion.yrel, 0);
				}
				break;

		} //sdlEvent.type

	} //while
}

int is_fullscreen (void) {
	if (SDL_GetWindowFlags(sdlWindow) & SDL_WINDOW_FULLSCREEN) {
		return 1;
	}
	return 0;
}

int picasso_palette (void) {
}

void screenshot (int mode, int doprepare) {
    Uint32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif

	SDL_Surface *screenshot = SDL_CreateRGBSurface (0, 800, 600, 32, rmask, gmask, bmask, amask);
	if (screenshot == NULL) {
        write_log ("SDL2: create RGB surface failed: %s\n", SDL_GetError());
    } else {
		SDL_RenderReadPixels( sdlRenderer, NULL, SDL_PIXELFORMAT_ARGB8888, screenshot->pixels, screenshot->pitch);
		SDL_SaveBMP (screenshot, "screenshot.bmp");
		SDL_FreeSurface (screenshot);
	}
}

int target_checkcapslock (int scancode, int *state) {
	if (SDL_GetModState() & KMOD_CAPS == KMOD_CAPS) {
		//caps on
	}
}

void setmaintitle (void) {
	const char *maintitle = "";
	SDL_SetWindowTitle(sdlWindow, maintitle);
}

void toggle_fullscreen (int mode) {
	if (!is_fullscreen) {
		if (SDL_SetWindowFullscreen(sdlWindow, SDL_TRUE) < 0) {
			write_log ("SDL2: set full screen failed: %s\n", SDL_GetError);
		}
	} else {
		if (SDL_SetWindowFullscreen(sdlWindow, SDL_FALSE) < 0) {
			write_log ("SDL2: set window mode failed: %s\n", SDL_GetError);
		}
	}
}

void toggle_mousegrab (void) {
	SDL_SetRelativeMouseMode(SDL_TRUE);
}

/*
 * Mouse inputdevice functions
 */

static int init_mouse (void) {
	return 1;
}

static void close_mouse (void) {
	return;
}

static int acquire_mouse (int num, int flags)
{
	return 1;
}

static void unacquire_mouse (int num)
{
	return;
}

static int get_mouse_num (void)
{
	return 1;
}

static TCHAR *get_mouse_friendlyname (int mouse)
{
	return "SDL2 mouse1";
}
static TCHAR *get_mouse_uniquename (int mouse)
{
	return "SDL2MOUSE1";
}

static int get_mouse_widget_num (int mouse)
{
}

static int get_mouse_widget_first (int mouse, int type)
{
	return -1;
}

static int get_mouse_widget_type (int mouse, int num, TCHAR *name, uae_u32 *code)
{

}

static void read_mouse (void)
{
}

static int get_mouse_flags (int num)
{
	return 0;
}

struct inputdevice_functions inputdevicefunc_mouse = {
	init_mouse,
	close_mouse,
	acquire_mouse,
	unacquire_mouse,
	read_mouse,
	get_mouse_num,
	get_mouse_friendlyname,
	get_mouse_uniquename,
	get_mouse_widget_num,
	get_mouse_widget_type,
	get_mouse_widget_first,
	get_mouse_flags
};

/*
 * Keyboard inputdevice functions
 */
static int get_kb_num (void)
{
	return 1;
}

static TCHAR *get_kb_friendlyname (int kb)
{
	return "SDL2 keyboard1";
}
static TCHAR *get_kb_uniquename (int kb)
{
	return "SDL2KEYB1";
}

static int get_kb_widget_num (int kb)
{
	return 255; // fix me
}

static int get_kb_widget_first (int kb, int type)
{
	return 0;
}

static int get_kb_widget_type (int kb, int num, TCHAR *name, uae_u32 *code)
{

}

static int init_kb (void)
{
	return 1;
}

static void close_kb (void)
{
}

static int keyhack (int scancode, int pressed, int num)
{
}

static void read_kb (void)
{
}

static int acquire_kb (int num, int flags)
{
	return 1;
}

static void unacquire_kb (int num)
{
}

static int get_kb_flags (int num)
{
	return 0;
}

struct inputdevice_functions inputdevicefunc_keyboard =
{
	init_kb,
	close_kb,
	acquire_kb,
	unacquire_kb,
	read_kb,
	get_kb_num,
	get_kb_friendlyname,
	get_kb_uniquename,
	get_kb_widget_num,
	get_kb_widget_type,
	get_kb_widget_first,
	get_kb_flags
};


int getcapslockstate (void) {
	return ((SDL_GetModState() & KMOD_CAPS) == KMOD_CAPS);
}

void setcapslockstate (int state) {
	SDL_Keymod currentmod = SDL_GetModState();
	int newmod = KMOD_NONE;
	if (currentmod & KMOD_CAPS) // preserve capslock
		newmod |= KMOD_CAPS;
	SDL_SetModState((SDL_Keymod)newmod);
}

/*
 * Default inputdevice config for SDL mouse
 */
int input_get_default_mouse (struct uae_input_device *uid, int num, int port, int af, bool gp, bool wheel, bool joymouseswap) {
	return 0;
}

/*
 * Handle gfx specific cfgfile options
 */
void gfx_default_options (struct uae_prefs *p) {

}

void gfx_save_options (struct zfile *f, const struct uae_prefs *p) {

}

int gfx_parse_option (struct uae_prefs *p, const char *option, const char *value) {

}
