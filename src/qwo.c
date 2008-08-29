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
#include <X11/extensions/shape.h>

#include <X11/keysym.h>
#include <locale.h>

#ifdef HAVE_LIBCONFIG
#include <libconfig.h>
#endif

#define WIDTH   300
#define HEIGHT  300

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
#define QUIT_GESTURE			 8

#define LONG_EXPOSURE_DELAY		2000L

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

KeySym custom_charset[MAX_REGIONS - 1][MAX_CHAR_PER_REGION];

const XPoint point1 = { WIDTH / 3, 0 };
const XPoint point2 = { 2*(WIDTH / 3), 0 };
const XPoint point3 = { WIDTH, (HEIGHT / 3) };
const XPoint point4 = { WIDTH, 2*(HEIGHT / 3) };
const XPoint point5 = { 2*(WIDTH / 3), HEIGHT };
const XPoint point6 = { (WIDTH / 3), HEIGHT };
const XPoint point7 = { 0, 2*(WIDTH / 3) };
const XPoint point8 = { 0, (HEIGHT / 3) };
const XPoint point9 = { (WIDTH / 3) + DELTA, (HEIGHT / 3 - DELTA) };
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

void init_regions(Display *dpy, Window toplevel)
{
	Window region_window;
	XSetWindowAttributes attributes;
	Region region;
	char window_name[2];
	int number;
	unsigned long valuemask;

	attributes.background_pixel = WhitePixel(dpy, DefaultScreen(dpy));
	attributes.border_pixel = BlackPixel(dpy, DefaultScreen(dpy));
	valuemask = CWBorderPixel | CWBackPixel;

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
		XSelectInput(dpy, region_window, EnterWindowMask |
				LeaveWindowMask);
		XMapWindow(dpy, region_window);
	}
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

int send_key_event(Display *dpy, Window client, KeyCode code, unsigned int state){
	XKeyEvent event;

	event.window = client;
	event.root = DefaultRootWindow(dpy);
	event.subwindow = None;
	event.state = state;
	event.keycode = code;
	event.type = KeyPress;
	event.time = CurrentTime;
	XSendEvent(dpy, InputFocus, False, NoEventMask, (XEvent *)&event);
	event.type = KeyRelease;
	event.time = CurrentTime;
	XSendEvent(dpy, InputFocus, False, NoEventMask, (XEvent *)&event);
	XSync(dpy, False);

	return 0;
}

int main(int argc, char **argv)
{
	Display *dpy;
	char *display_name;
	Window toplevel, global_window, client;
	int focus_state;
	XSetWindowAttributes attributes;
	XWMHints *wm_hints;
	Atom wmDeleteMessage;

	char *font_name = NULL;
	XFontStruct *font_info;
	unsigned long valuemask;
	XGCValues xgc;
	GC gc;

	int event_base, error_base;
	int shape_ext_major, shape_ext_minor;

	char *config_path = NULL;
	int loaded_config = 0;
	int run = 0;
	int options;
	int buffer[MAX_GESTURES_BUFFER];
	int buffer_count = 0;
	int invalid_gesture = 0;
	int shift_modifier = 1;
	Time last_cross_timestamp = 0L;

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
	valuemask = CWBackPixel;

	global_window = XCreateWindow(dpy, DefaultRootWindow(dpy), 0, 0,
			WIDTH, HEIGHT, 0, CopyFromParent,
			InputOutput, CopyFromParent, valuemask, &attributes);

	toplevel = XCreateWindow(dpy, global_window, 0, 0,
			WIDTH, HEIGHT, 0, CopyFromParent, InputOutput,
			CopyFromParent, valuemask, &attributes);

	xgc.foreground = blackColor;
	xgc.line_width = 2;
	valuemask = GCForeground | GCLineWidth;

	gc = XCreateGC(dpy, toplevel, valuemask, &xgc);

	init_regions(dpy, toplevel);
	XMapWindow(dpy, toplevel);
	XMapWindow(dpy, global_window);

	wm_hints = XAllocWMHints();

	if (wm_hints) {
		wm_hints->input = False;
		wm_hints->flags = InputHint;
		XSetWMHints(dpy, global_window, wm_hints);
		XSetWMHints(dpy, toplevel, wm_hints);
		XFree(wm_hints);
	}

	wmDeleteMessage = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(dpy, global_window, &wmDeleteMessage, 1);

	XSelectInput(dpy, toplevel, ExposureMask | ButtonPress);

	if(!(load_font(dpy, &font_info, font_name))) {
		XDestroyWindow(dpy, toplevel);
		XCloseDisplay(dpy);
		exit(-1);
	}

	XSetFont(dpy, gc, font_info->fid);

	draw_grid(dpy, toplevel, gc);
	display_charset(dpy, toplevel, gc, font_info, shift_modifier);
	XSync(dpy, False);

	while(!run) {
		XEvent e;
		char *region_name;

		XNextEvent(dpy, &e);
		switch (e.type) {
			case EnterNotify:
				if (XFetchName(dpy, e.xcrossing.window, &region_name)) {
					char region = region_name[0] - 48;
					if (region == 0)
						run = 1;
					XFree(region_name);
				}
		}
	}

	while(run) {
		XEvent e;
		char *region_name;

		XGetInputFocus(dpy, &client, &focus_state);

		XNextEvent(dpy, &e);
		switch (e.type) {
			case Expose:
				draw_grid(dpy, toplevel, gc);
				display_charset(dpy, toplevel, gc, font_info, shift_modifier);
				XSync(dpy, False);
				break;
			case ButtonPress:
				break;
			case LeaveNotify:
				//XFetchName(dpy, e.xcrossing.window, &region_name);
				break;
			case EnterNotify:
				XFetchName(dpy, e.xcrossing.window, &region_name);
				char region = region_name[0] - 48;
				XFree(region_name);
				KeyCode code;
				int state_mod = 0;
				char c = '\0';

				if (invalid_gesture) {
					if (region == 0) {
						invalid_gesture = 0;
						buffer_count = 0;
					} else if (buffer_count == QUIT_GESTURE) {
					run = 0;
					} else if (buffer_count > QUIT_GESTURE) {
						buffer_count = 0;
					}
					buffer_count += 1;
					break;
				}

				if (buffer_count > MAX_GESTURES_BUFFER) {
					buffer_count += 1;
					invalid_gesture = 1;
					break;
				}

				if (region == 0) {
					if (buffer_count == 0) {
						break;
					}

					if ((buffer[0] == buffer[buffer_count - 1]) && loaded_config && (buffer_count > 1)) {
						buffer_count = (buffer_count - 1) >> 1;
						KeySym index;

						if (DIRECTION(buffer[0], buffer[1]))
							index = custom_charset[buffer[0] - 1][(buffer_count << 1) - 1];
						else
							index = custom_charset[buffer[0] - 1][(buffer_count  << 1 ) - 2];

						if (shift_modifier) {
							KeySym lower, upper;
							XConvertCase(index, &lower, &upper);
							code = XKeysymToKeycode(dpy, upper);
						} else {
							code = XKeysymToKeycode(dpy, index);
						}
					} else {
						c = charset[buffer[0] - 1][buffer[buffer_count - 1] - 1];
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
							if (buffer_count == 1) {
							char string[] = "BackSpace";
							code = XKeysymToKeycode(dpy, XStringToKeysym(string));
							}
						}
						send_key_event(dpy, client, code, 0);
					} else if (c == '>') {
						if (buffer_count == 1) {
							if (e.xcrossing.time - last_cross_timestamp > LONG_EXPOSURE_DELAY) {
								char key_name[] = "Return";
								code = XKeysymToKeycode(dpy, XStringToKeysym(key_name));
							} else {
								char key_name[] = "space";
								code = XKeysymToKeycode(dpy, XStringToKeysym(key_name));
							}
							send_key_event(dpy, client, code, 0);
						} else if (shift_modifier) {
							shift_modifier = 0;
						} else if (buffer_count == 3) {
							shift_modifier = 1;
						} else if (buffer_count == 4) {
							shift_modifier = 2;
						}

						if (buffer_count != 1) {
							XClearWindow(dpy, toplevel);
							draw_grid(dpy, toplevel, gc);
							display_charset(dpy, toplevel, gc, font_info, shift_modifier);
							XSync(dpy, False);
							buffer_count = 0;
							break;
						}
					} else {
						if ((shift_modifier && isalpha(c)) || state_mod)
							send_key_event(dpy, client, code, (shift_modifier & 1) | state_mod);
						else
							send_key_event(dpy, client, code, 0);
						if (shift_modifier == 1) {
							shift_modifier = 0;
							XClearWindow(dpy, toplevel);
							draw_grid(dpy, toplevel, gc);
							display_charset(dpy, toplevel, gc, font_info, shift_modifier);
							XSync(dpy, False);
						}
					}

					buffer_count = 0;
					break;
				}

				if(!buffer_count) {
					last_cross_timestamp = e.xcrossing.time;
				}
				buffer[buffer_count] = region;
				buffer_count++;
				break;
			case ClientMessage:
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

