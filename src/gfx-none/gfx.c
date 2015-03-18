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


TCHAR config_filename[256] = "";

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
}

int graphics_init (void) {
}

void graphics_leave (void) {
}

bool handle_events (void) {
}

int is_fullscreen (void) {
}

int picasso_palette (void) {
}

void screenshot (int mode, int doprepare) {
}

int target_checkcapslock (int scancode, int *state) {
}

void setmaintitle (void) {
}

void toggle_fullscreen (int mode) {
}

void toggle_mousegrab (void) {
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
	return "Default mouse";
}
static TCHAR *get_mouse_uniquename (int mouse)
{
	return "DEFMOUSE1";
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
	return "Default keyboard";
}
static TCHAR *get_kb_uniquename (int kb)
{
	return "DEFKEYB1";
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
}

void setcapslockstate (int state) {

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
