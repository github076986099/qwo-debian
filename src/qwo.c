/*
 *  qwo: quikwriting for OpenMoko
 *
 *  Copyright (C) 2008 Charles Clement caratorn _at_ gmail.com
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301  USA
 *
 */

#include <config.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <X11/extensions/shape.h>
#include <X11/extensions/XTest.h>

#include <X11/keysym.h>
#include <locale.h>

#ifdef HAVE_LIBCONFIG
#include <libconfig.h>
#endif

#define WIDTH   400
#define HEIGHT  400

#define MAX_REGIONS 9
#define MAX_POINTS  9

#define DELTA                   (WIDTH >> 4)
#define BORDER_WIDTH            3

#define GRID_COLOR				"orange"
//#define DEFAULT_FONT    "-*-courier-bold-*-*-12-*-*-*-*-*-*-*"
#define DEFAULT_FONT    "fixed"

#define CONFIG_FILE		"/.qworc"
#define MAX_CONFIG_PATH 30

#define MAX_CHAR_PER_REGION		 5

#define MAX_GESTURES_BUFFER      6

#define LONG_EXPOSURE_DELAY		2000L
#define PRESS_DELAY				100L

#define FILL_REGION4(a, b, c, d)        {a, b, c, d, a, a, a, a, a}
#define FILL_REGION5(a, b, c, d, e)     {a, b, c, d, e, a, a, a, a}
#define FILL_REGION8(a, b, c, d, e, f, g, h)     {a, b, c, d, e, f, g, h, a}

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#define DIRECTION(a, b)  \
		(abs(b - a) == 1) ? ( (b - a) == 1) : (b - a) < 0

char charset[][MAX_REGIONS] = {
		"abc??"">>z",
		"def?????",
		"ghijk???",
		"??l>,???",
		"??mnopq?",
		"????tsr?",
		"????.vuw",
		"y?????x<"
};

KeySym char_free[MAX_REGIONS][MAX_REGIONS] = {
	{XK_KP_5, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, XK_KP_0, 0xffff, 0xffff},
	{0xffff, XK_KP_1, XK_Tab, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, XK_slash},
	{0xffff, 0xffff, XK_KP_2, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff},
	{0xffff, 0xffff, 0xffff, XK_KP_3, XK_backslash, 0xffff, 0xffff, 0xffff, 0xffff},
	{0xffff, 0xffff, 0xffff, 0xffff, XK_KP_6, XK_Return, 0xffff, 0xffff, 0xffff},
	{0xffff, 0xffff, 0xffff, 0xffff, 0xffff, XK_KP_9, 0xffff, 0xffff, 0xffff},
	{0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, XK_KP_8, 0xffff, 0xffff},
	{0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, XK_KP_7, 0xffff},
	{0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, XK_Control_L, 0xffff, XK_KP_4}};


KeySym custom_charset[MAX_REGIONS - 1][MAX_CHAR_PER_REGION];

const XPoint point1 = { WIDTH / 3, 0 };
const XPoint point2 = { 2*(WIDTH / 3), 0 };
const XPoint point3 = { WIDTH, (HEIGHT / 3) };
const XPoint point4 = { WIDTH, 2*(HEIGHT / 3) };
const XPoint point5 = { 2*(WIDTH / 3), HEIGHT };
const XPoint point6 = { (WIDTH / 3), HEIGHT };
const XPoint point7 = { 0, 2*(HEIGHT/ 3) };
const XPoint point8 = { 0, (HEIGHT / 3) };
const XPoint point9 = { (WIDTH / 3) + DELTA, (HEIGHT / 3) - DELTA };
const XPoint point10 = { 2*(WIDTH / 3) - DELTA, (HEIGHT / 3) - DELTA };
const XPoint point11 =  { 2*(WIDTH / 3) + DELTA, (HEIGHT / 3) + DELTA };
const XPoint point12 = { 2*(WIDTH / 3) + DELTA, 2*(HEIGHT / 3) - DELTA };
const XPoint point13 = { 2*(WIDTH / 3) - DELTA, 2*(HEIGHT / 3) + DELTA };
const XPoint point14 = { (WIDTH / 3) + DELTA, 2*(HEIGHT / 3) + DELTA };
const XPoint point15 = { (WIDTH / 3) - DELTA, 2*(HEIGHT / 3) - DELTA };
const XPoint point16 = { (WIDTH / 3) - DELTA, (HEIGHT / 3) + DELTA };

/*
 * Cardinal points
 */

const XPoint point_ne = { 0, 0};
const XPoint point_nw = { WIDTH, 0};
const XPoint point_sw = { WIDTH, HEIGHT};
const XPoint point_se = { 0, HEIGHT};

typedef enum {
	KeyboardNone = 0,
	KeyboardShow,
	KeyboardHide,
	KeyboardToggle
} KeyboardOperation;

static Atom wmDeleteMessage, mtp_im_invoker_command,mb_im_invoker_command,
		net_wm_state_skip_taskbar, net_wm_state_skip_pager, net_wm_state;

void init_regions(Display *dpy, Window toplevel)
{
	Window region_window;
	Region region;
	char window_name[2];
	int number;
	XWMHints *wm_hints;

	wm_hints = XAllocWMHints();

	if (wm_hints) {
		wm_hints->input = False;
		wm_hints->flags = InputHint;
	}

	XPoint regions[MAX_REGIONS][MAX_POINTS] = {
		FILL_REGION8(point9, point10, point11, point12, point13,
				point14, point15, point16),
		FILL_REGION5(point_ne, point1, point9, point16, point8),
		FILL_REGION4(point1, point2, point10, point9),
		FILL_REGION5(point2, point_nw, point3, point11, point10),
		FILL_REGION4(point3, point4, point12, point11),
		FILL_REGION5(point4, point_sw, point5, point13, point12),
		FILL_REGION4(point5, point6, point14, point13),
		FILL_REGION5(point6, point_se, point7, point15, point14),
		FILL_REGION4(point7, point8, point16, point15)
	};

	for (number = 0; number < MAX_REGIONS; number++){
		region = XPolygonRegion(regions[number],
				ARRAY_SIZE(regions[number]), EvenOddRule);
		region_window = XCreateWindow(dpy, toplevel, 0, 0,
				WIDTH, HEIGHT, 0, CopyFromParent,
				InputOnly, CopyFromParent, 0, NULL);
		sprintf(window_name, "%i", number);
		XStoreName(dpy, region_window, window_name);
		XShapeCombineRegion(dpy, region_window, ShapeBounding, 0, 0,
				region, ShapeSet);
		XSetWMHints(dpy, region_window, wm_hints);
		XSelectInput(dpy, region_window, EnterWindowMask | LeaveWindowMask | ButtonPressMask | ButtonReleaseMask);
		XMapWindow(dpy, region_window);
	}
	XFree(wm_hints);
}

int load_font(Display *dpy, XFontStruct **font_info, char *font)
{
	if (font == NULL)
		font = DEFAULT_FONT;
	if ((*font_info = XLoadQueryFont(dpy, font)) == NULL){
		fprintf( stderr, "Cannot open font %s\n", font);
		return 0;
	}
	return 1;
}

void display_charset(Display *dpy, Window win, GC gc, XFontStruct *font_info, int upper_case)
{
	int len;
	int font_height;
	int i,j, count;
	int offset;
	XTextItem item = { NULL, 1, 0, font_info->fid };

	font_height = font_info->ascent + font_info->descent;
	len = XTextWidth(font_info, &charset[0][0], 1);
	offset = WIDTH / 8;

	count = 0;
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3 && count != 8; j++, count++) {
			char c;
			if (upper_case) {
				c = toupper((int) charset[i][j]);
				item.chars = &c;
			}
			else
				item.chars = &charset[i][j];
			XDrawText(dpy, win, gc, offset * count, font_height, &item, 1);
			if (upper_case) {
				c = toupper((int) charset[i + 4][j + 4]);
				item.chars = &c;
			}
			else
				item.chars = &charset[i + 4][j + 4];
			XDrawText(dpy, win, gc, WIDTH - (offset * count) - len, HEIGHT - font_info->descent, &item, 1);
			if (upper_case) {
				c = toupper((int) charset[i + 2][j + 2]);
				item.chars = &c;
			}
			else
				item.chars = &charset[i + 2][j + 2];
			XDrawText(dpy, win, gc, WIDTH - 2 * len, offset * count + font_height, &item, 1);
			if (upper_case) {
				c = toupper((int) charset[(i + 6) & 7][(j + 6) & 7]);
				item.chars = &c;
			}
			else
				item.chars = &charset[(i + 6) & 7][(j + 6) & 7];
			XDrawText(dpy, win, gc, len, HEIGHT - (offset * count), &item, 1);
		}
	}

}

void draw_grid(Display *dpy, Window toplevel, GC gc)
{
	XColor grid_color, exact;
	Colormap cmap;

	unsigned long blackColor = BlackPixel(dpy, DefaultScreen(dpy));
	cmap = DefaultColormap(dpy, DefaultScreen(dpy));

	XAllocNamedColor(dpy, cmap, GRID_COLOR, &grid_color, &exact);

	XSetForeground(dpy, gc, grid_color.pixel);

	XDrawLine(dpy, toplevel, gc, point1.x, point1.y, point9.x, point9.y);
	XDrawLine(dpy, toplevel, gc, point2.x, point2.y, point10.x, point10.y);

	XDrawLine(dpy, toplevel, gc, point3.x, point3.y, point11.x, point11.y);
	XDrawLine(dpy, toplevel, gc, point4.x, point4.y, point12.x, point12.y);

	XDrawLine(dpy, toplevel, gc, point5.x, point5.y, point13.x, point13.y);
	XDrawLine(dpy, toplevel, gc, point6.x, point6.y, point14.x, point14.y);

	XDrawLine(dpy, toplevel, gc, point7.x, point7.y, point15.x, point15.y);
	XDrawLine(dpy, toplevel, gc, point8.x, point8.y, point16.x, point16.y);
	XSetForeground(dpy, gc, blackColor);

}

#ifdef HAVE_LIBCONFIG
int read_config(char *config_path)
{
	int j, i = 0;
	config_t configuration;
	FILE * file;
	const char *keysym_name;
	KeySym key;
	const config_setting_t *keymap;
	const config_setting_t *line;

	if ((file = fopen(config_path, "r")) == NULL) {
		fprintf(stderr, "%s : Can't open configuration file\n", config_path);
		return 0;
	}
	config_init(&configuration);
	if (config_read(&configuration, file) == CONFIG_FALSE) {
		fprintf(stderr, "File %s, Line %i : %s\n", config_path,
				config_error_line(&configuration),
				config_error_text(&configuration));
		exit(3);
	}
	fclose(file);
	keymap = config_lookup(&configuration, "charset");

	if (keymap)
		for (i = 0 ; i < config_setting_length(keymap) ; i++) {
			line = config_setting_get_elem(keymap, i);
			for (j = 0 ; j < config_setting_length(line); j++) {
				keysym_name = config_setting_get_string_elem(line, j);
				if ((key = XStringToKeysym(keysym_name)) == NoSymbol) {
					fprintf(stderr, "KeySym not found : %s\n",
							config_setting_get_string_elem(line, j));
					exit(3);
				}
				custom_charset[i][j] = key & 0xffffff;
			}
			for (; j < MAX_CHAR_PER_REGION; j++) {
				custom_charset[i][j] = '\0';
			}
		}
		for (; i < MAX_REGIONS - 1 ; i++) {
			for ( j = 0 ; j < MAX_CHAR_PER_REGION; j++) {
				custom_charset[i][j] = '\0';
			}
		}
	config_destroy(&configuration);
	return 1;
}
#endif

int set_window_properties(Display *dpy, Window toplevel){
	XSizeHints	size_hints;
	XWMHints *wm_hints;

	wm_hints = XAllocWMHints();

	if (wm_hints) {
		wm_hints->input = False;
		wm_hints->flags = InputHint;
		XSetWMHints(dpy, toplevel, wm_hints);
		XFree(wm_hints);
	}

	size_hints.flags = PPosition | PSize;
	size_hints.x = 0;
	size_hints.y = 0;
	size_hints.width = WIDTH;
	size_hints.height = HEIGHT;

	XSetStandardProperties(dpy, toplevel, "Keyboard", NULL, 0, NULL, 0,
			&size_hints);

	wmDeleteMessage = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	mtp_im_invoker_command = XInternAtom(dpy, "_MTP_IM_INVOKER_COMMAND", False);
	mb_im_invoker_command = XInternAtom(dpy, "_MB_IM_INVOKER_COMMAND", False);
	XSetWMProtocols(dpy, toplevel, &wmDeleteMessage, 1);

	net_wm_state_skip_pager = XInternAtom(dpy, "_NET_WM_STATE_SKIP_PAGER", False);
	net_wm_state_skip_taskbar = XInternAtom(dpy, "_NET_WM_STATE_SKIP_TASKBAR", False);
	net_wm_state = XInternAtom (dpy, "_NET_WM_STATE", False);

	XChangeProperty (dpy, toplevel, net_wm_state, XA_ATOM, 32, PropModeAppend,
			(unsigned char *)&net_wm_state_skip_taskbar, 1);
	XChangeProperty (dpy, toplevel, net_wm_state, XA_ATOM, 32, PropModeAppend,
			(unsigned char *)&net_wm_state_skip_pager, 1);

	return 0;
}


int main(int argc, char **argv)
{
	Display *dpy;
	char *display_name;
	Window toplevel;
	XSetWindowAttributes attributes;

	char *font_name = NULL;
	XFontStruct *font_info;
	unsigned long valuemask;
	XGCValues xgc;
	GC gc;

	int event_base, error_base;
	int shape_ext_major, shape_ext_minor;

	char *config_path = NULL;
	int loaded_config = 0;
	int run = 1;
	int options;
	int visible = 0;
	int buffer[MAX_GESTURES_BUFFER];
	int buffer_count = 0;
	int shift_modifier = 0;
	int ctrl_modifier = 0;
	Time last_cross_timestamp = 0L;
	Time last_pressed = 0L;


	options = getopt(argc, argv, "f:c:");

	switch(options){
		case 'f':
			font_name = optarg;
			break;
		case 'c':
			config_path = optarg;
			break;
	}

	display_name = XDisplayName(NULL);
	dpy = XOpenDisplay(display_name);

	if (!setlocale(LC_CTYPE, ""))
	{
		fprintf(stderr, "Locale not specified. Check LANG, LC_CTYPE, LC_ALL. ");
		return 1;
	}

	if (dpy == NULL){
		fprintf(stderr, "%s : Can't open display %s\n", argv[0],
				display_name);
		exit(1);
	}

	if (XShapeQueryExtension(dpy, &event_base, &error_base) != True){
		fprintf(stderr, "%s : X11 Shape extension not supported on "
				"%s\n", argv[0], display_name);
		exit(2);
	}

#ifdef HAVE_LIBCONFIG
	if (config_path) {
		loaded_config = read_config(config_path);
	} else {
		char config_path[MAX_CONFIG_PATH];
		char *home_dir = getenv("HOME");
		strncpy(config_path, home_dir, MAX_CONFIG_PATH);
		strncat(config_path + strlen(home_dir), CONFIG_FILE,
				MAX_CONFIG_PATH - strlen(home_dir));
		loaded_config = read_config(config_path);
	}
#endif

	XShapeQueryVersion(dpy, &shape_ext_major, &shape_ext_minor);

	unsigned long blackColor = BlackPixel(dpy, DefaultScreen(dpy));
	unsigned long whiteColor = WhitePixel(dpy, DefaultScreen(dpy));

	attributes.background_pixel = whiteColor;
	attributes.override_redirect = True;
	valuemask = CWBackPixel;

	toplevel = XCreateWindow(dpy, DefaultRootWindow(dpy), 0, 0,
			WIDTH, HEIGHT, 0, CopyFromParent, CopyFromParent,
			CopyFromParent, valuemask, &attributes);

	xgc.foreground = blackColor;
	xgc.line_width = 2;
	valuemask = GCForeground | GCLineWidth;

	gc = XCreateGC(dpy, toplevel, valuemask, &xgc);

	init_regions(dpy, toplevel);

	set_window_properties(dpy, toplevel);

	XSelectInput(dpy, toplevel, SubstructureNotifyMask | StructureNotifyMask);

	XMapWindow(dpy, toplevel);

	if(!(load_font(dpy, &font_info, font_name))) {
		XDestroyWindow(dpy, toplevel);
		XCloseDisplay(dpy);
		exit(-1);
	}

	XSetFont(dpy, gc, font_info->fid);

	draw_grid(dpy, toplevel, gc);
	display_charset(dpy, toplevel, gc, font_info, shift_modifier);
	XSync(dpy, False);

	while(run) {
		XEvent e;
		char *region_name;
		int region;
		KeyCode code;

		XNextEvent(dpy, &e);
		switch (e.type) {
			case ButtonPress:
				XFetchName(dpy, e.xbutton.window, &region_name);
				region = region_name[0] - 48;
				XFree(region_name);
				XUngrabPointer(dpy, CurrentTime);
				buffer[0] = region;
				last_pressed = e.xbutton.time;
				break;
			case ButtonRelease:
				XFetchName(dpy, e.xbutton.window, &region_name);
				region = region_name[0] - 48;
				XFree(region_name);
				if (buffer[0] == 0 && !region && e.xbutton.time - last_pressed > PRESS_DELAY)
					break;
				if (char_free[buffer[0]][region] != XK_Control_L) {
					code = XKeysymToKeycode(dpy, char_free[buffer[0]][region]);
					XTestFakeKeyEvent(dpy, code, True, 0);
					XTestFakeKeyEvent(dpy, code, False, 0);
				} else
					ctrl_modifier = 1;
				buffer_count = 0;
				break;
			case EnterNotify:
				XFetchName(dpy, e.xcrossing.window, &region_name);
				region = region_name[0] - 48;
				XFree(region_name);

				int state_mod = 0;
				char c = '\0';

				if ((region == 0) && buffer_count > 1 && buffer[0] == 0){

					if ((buffer[1] == buffer[buffer_count - 1]) && loaded_config && (buffer_count > 2)) {
						buffer_count = (buffer_count - 1) >> 1;
						KeySym index;

						if (DIRECTION(buffer[1], buffer[2]))
							index = custom_charset[buffer[1] - 1][(buffer_count << 1) - 1];
						else
							index = custom_charset[buffer[1] - 1][(buffer_count << 1) - 2];

						if (shift_modifier) {
							KeySym lower, upper;
							XConvertCase(index, &lower, &upper);
							code = XKeysymToKeycode(dpy, upper);
						} else {
							code = XKeysymToKeycode(dpy, index);
						}
					} else {
						c = charset[buffer[1] - 1][buffer[buffer_count - 1] - 1];
							// X11 KeySym maps ASCII table
						code = XKeysymToKeycode(dpy, c);
						for (state_mod = 0; state_mod < 4; state_mod++) {
							if (XKeycodeToKeysym(dpy, code, state_mod) == (KeySym) c) {
								break;
							}
						}
					}

					if (c == '<') {
						if (e.xcrossing.time - last_cross_timestamp > LONG_EXPOSURE_DELAY) {
								// Erase all buffer
						} else {
							if (buffer_count == 2) {
							code = XKeysymToKeycode(dpy, XK_BackSpace);
							}
						}
						XTestFakeKeyEvent(dpy, code, True, 0);
						XTestFakeKeyEvent(dpy, code, False, 0);
					} else if (c == '>') {
						if (buffer_count == 2) {
							if (e.xcrossing.time - last_cross_timestamp > LONG_EXPOSURE_DELAY) {
								code = XKeysymToKeycode(dpy, XK_Return);
							} else {
								code = XKeysymToKeycode(dpy, XK_space);
							}
							XTestFakeKeyEvent(dpy, code, True, 0);
							XTestFakeKeyEvent(dpy, code, False, 0);
						} else if (shift_modifier) {
							shift_modifier = 0;
						} else if (buffer_count == 4) {
							shift_modifier = 1;
						} else if (buffer_count == 5) {
							shift_modifier = 2;
						}

						if (buffer_count != 2) {
							XClearWindow(dpy, toplevel);
							draw_grid(dpy, toplevel, gc);
							display_charset(dpy, toplevel, gc, font_info, shift_modifier);
							XSync(dpy, False);
							buffer_count = 1;
							buffer[0] = 0;
							break;
						}
					} else {
						if ((shift_modifier && isalpha(c)) || state_mod) {
							XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, XK_Shift_L), True, 0);
							XTestFakeKeyEvent(dpy, code, True, 0);
							XTestFakeKeyEvent(dpy, code, False, 0);
							XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, XK_Shift_L), False, 0);
						} else if (ctrl_modifier) {
							XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, XK_Control_L), True, 0);
							XTestFakeKeyEvent(dpy, code, True, 0);
							XTestFakeKeyEvent(dpy, code, False, 0);
							XTestFakeKeyEvent(dpy, XKeysymToKeycode(dpy, XK_Control_L), False, 0);
							ctrl_modifier = 0;
						} else {
							XTestFakeKeyEvent(dpy, code, True, 0);
							XTestFakeKeyEvent(dpy, code, False, 0);
						}
						if (shift_modifier == 1) {
							shift_modifier = 0;
							XClearWindow(dpy, toplevel);
							draw_grid(dpy, toplevel, gc);
							display_charset(dpy, toplevel, gc, font_info, shift_modifier);
							XSync(dpy, False);
						}
					}

					buffer_count = 1;
					buffer[0] = 0;
					break;
				}

				if(buffer_count == 1) {
					last_cross_timestamp = e.xcrossing.time;
				}
				if (!buffer_count || (buffer[buffer_count - 1] != region)) {
					buffer[buffer_count] = region;
					buffer_count++;
				}
				break;
			case ConfigureNotify:
				XMapWindow(dpy, toplevel);
				draw_grid(dpy, toplevel, gc);
				display_charset(dpy, toplevel, gc, font_info, shift_modifier);
				XSync(dpy, False);
			case ClientMessage:
				if ((e.xclient.message_type == mb_im_invoker_command) ||
					(e.xclient.message_type == mtp_im_invoker_command)) {
					if (e.xclient.data.l[0] == KeyboardShow) {
						XMapWindow(dpy, toplevel);
						draw_grid(dpy, toplevel, gc);
						display_charset(dpy, toplevel, gc, font_info, shift_modifier);
						XSync(dpy, False);
					}
					if (e.xclient.data.l[0] == KeyboardHide) {
						XUnmapWindow(dpy, toplevel);
						XSync(dpy, False);
					}
					if (e.xclient.data.l[0] == KeyboardToggle) {
						if (visible) {
							XUnmapWindow(dpy, toplevel);
							XSync(dpy, False);
							visible = 0;
						} else {
							XMapWindow(dpy, toplevel);
							draw_grid(dpy, toplevel, gc);
							display_charset(dpy, toplevel, gc, font_info, shift_modifier);
							XSync(dpy, False);
							visible = 1;
						}
					}
					break;
				}
				if (e.xclient.data.l[0] == wmDeleteMessage)
					run = 0;
				break;
			default:
				printf("Received event %i\n", e.type);
			}
	}

	XFreeFont(dpy, font_info);
	XFreeGC(dpy, gc);
	XDestroyWindow(dpy, toplevel);
	XCloseDisplay(dpy);

	exit(0);
}

