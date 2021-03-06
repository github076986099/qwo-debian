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

#include <Imlib2.h>

#ifdef HAVE_LIBCONFIG
#include <libconfig.h>
#endif

// needed for setting motif hints
#define PROP_MOTIF_WM_HINTS_ELEMENTS    5
#define MWM_HINTS_DECORATIONS          (1L << 1)
typedef struct
{
	unsigned long       flags;
	unsigned long       functions;
	unsigned long       decorations;
	long                inputMode;
	unsigned long       status;
} PropMotifWmHints;
// end motif hints

#define WIDTH   300
#define HEIGHT  WIDTH

#define MAX_REGIONS 9
#define MAX_POINTS  9

#define DELTA                   (WIDTH >> 4)
#define BORDER_WIDTH            3

#define GRID_COLOR				"orange"

#define CONFIG_FILE		"/.qworc"
#define MAX_CONFIG_PATH 50

#define DATA_PATH		DATADIR "/" PACKAGE_NAME "/"

#define MAX_IMAGE_NAME 10
#define MAX_IMAGE_PATH 50

#define MAX_IMAGES		3

#define IMAGE_SUFFIXE ".png"

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
		"a,z?>>>c",
		"deb?????",
		"gfihj???",
		"??l>k???",
		"??mnopq?",
		"????tsr?",
		"????.vuw",
		"y?????x<"
};

KeySym char_free[MAX_REGIONS][MAX_REGIONS] = {
	{XK_KP_5, 0xffff, XK_Up, 0xffff, XK_Right, XK_greater, XK_KP_0, XK_less, XK_Left},
	{0xffff, XK_KP_1, XK_Tab, XK_KP_Multiply, 0xffff, 0xffff, 0xffff, XK_parenleft, XK_slash},
	{XK_Down, XK_exclam, XK_KP_2, XK_minus, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff},
	{0xffff, XK_equal, XK_dollar, XK_KP_3, XK_backslash, XK_parenright, 0xffff, 0xffff, 0xffff},
	{0xffff, 0xffff, 0xffff, XK_plus, XK_KP_6, XK_Return, 0xffff, 0xffff, 0xffff},
	{0xffff, 0xffff, 0xffff, XK_colon, XK_semicolon, XK_KP_9, XK_bracketright, XK_at, 0xffff},
	{0xffff, 0xffff, 0xffff, 0xffff, 0xffff, XK_underscore, XK_KP_8, XK_braceright, 0xffff},
	{0xffff, XK_ampersand, 0xffff, 0xffff, 0xffff, XK_braceleft, XK_bracketleft, XK_KP_7, XK_numbersign},
	{0xffff, XK_Escape, 0xffff, 0xffff, 0xffff, 0xffff, XK_Control_L, XK_quotedbl, XK_KP_4}};

static KeyCode Shift_code, Control_code;

KeySym custom_charset[MAX_REGIONS - 1][MAX_CHAR_PER_REGION];

char image_names[][MAX_IMAGE_NAME] = { "normal", "caps", "extra" };

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

static Pixmap char_pixmaps[3];

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

void draw_grid(Display *dpy, Pixmap pixmap, GC gc)
{
	XColor grid_color, exact;
	Colormap cmap;

	unsigned long blackColor = BlackPixel(dpy, DefaultScreen(dpy));
	cmap = DefaultColormap(dpy, DefaultScreen(dpy));

	XAllocNamedColor(dpy, cmap, GRID_COLOR, &grid_color, &exact);

	XSetForeground(dpy, gc, grid_color.pixel);

	XDrawLine(dpy, pixmap, gc, point1.x, point1.y, point9.x, point9.y);
	XDrawLine(dpy, pixmap, gc, point2.x, point2.y, point10.x, point10.y);

	XDrawLine(dpy, pixmap, gc, point3.x, point3.y, point11.x, point11.y);
	XDrawLine(dpy, pixmap, gc, point4.x, point4.y, point12.x, point12.y);

	XDrawLine(dpy, pixmap, gc, point5.x, point5.y, point13.x, point13.y);
	XDrawLine(dpy, pixmap, gc, point6.x, point6.y, point14.x, point14.y);

	XDrawLine(dpy, pixmap, gc, point7.x, point7.y, point15.x, point15.y);
	XDrawLine(dpy, pixmap, gc, point8.x, point8.y, point16.x, point16.y);

	XSetForeground(dpy, gc, blackColor);

}

int load_charset(Display *dpy, GC gc, int num){
	Visual *vis;
	Colormap cm;
	int depth;
	char image_path[MAX_IMAGE_PATH];
	Imlib_Image image;

	unsigned long blackColor = BlackPixel(dpy, DefaultScreen(dpy));
	unsigned long whiteColor = WhitePixel(dpy, DefaultScreen(dpy));

	XSetForeground(dpy, gc, whiteColor);
	XFillRectangle(dpy, char_pixmaps[num], gc, 0, 0, WIDTH, HEIGHT);
	XSetForeground(dpy, gc, blackColor);

	vis = DefaultVisual(dpy, DefaultScreen(dpy));
	depth = DefaultDepth(dpy, DefaultScreen(dpy));
	cm = DefaultColormap(dpy, DefaultScreen(dpy));

	imlib_context_set_display(dpy);
	imlib_context_set_visual(vis);
	imlib_context_set_colormap(cm);
	imlib_context_set_progress_function(NULL);

	imlib_context_set_drawable(char_pixmaps[num]);

	strncpy(image_path, DATA_PATH, MAX_IMAGE_PATH);
	strncat(image_path + strlen(DATA_PATH), image_names[num],
		MAX_IMAGE_PATH - strlen(DATA_PATH));
	strncat(image_path + strlen(DATA_PATH) + strlen(image_names[num]),
			IMAGE_SUFFIXE, MAX_IMAGE_PATH - strlen(DATA_PATH) - strlen(image_names[num]));
	image = imlib_load_image(image_path);
	if (!image) {
		fprintf(stderr, "%s : Can't open file\n", image_path);
		return 1;
	}
	imlib_context_set_image(image);
	imlib_render_image_on_drawable(0, 0);

	draw_grid(dpy, char_pixmaps[num], gc);

	imlib_free_image();

	return 0;
}

#ifdef HAVE_LIBCONFIG
int read_config(char *config_path, char **geometry)
{
	int j, i = 0;
	config_t configuration;
	FILE * file;
	const char *keysym_name, *string;
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

	if (keymap) {
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
	}

	string = config_lookup_string(&configuration, "geometry");

	if (string) {
		*geometry = (char *) malloc(sizeof(char) * strlen(string));
		strcpy(*geometry, string);
	}

	config_destroy(&configuration);
	return 1;
}
#endif

int set_window_properties(Display *dpy, Window toplevel, char decorations){
	XWMHints *wm_hints;
	Atom mwm_atom = XInternAtom(dpy, "_MOTIF_WM_HINTS",False);
	PropMotifWmHints mwm_hints;

	XStoreName(dpy, toplevel, "Keyboard");

	wm_hints = XAllocWMHints();

	if (wm_hints) {
		wm_hints->input = False;
		wm_hints->flags = InputHint;
		XSetWMHints(dpy, toplevel, wm_hints);
		XFree(wm_hints);
	}
	if (decorations == 0)
	{
		// mwm_hints->input seems not to have any effect in metacity
		// unless the motif hint "mwm_hints->decorations = 0" is set
		mwm_hints.flags = MWM_HINTS_DECORATIONS;
		mwm_hints.decorations = 0;
		XChangeProperty(dpy, toplevel, mwm_atom, XA_ATOM, 32,
				PropModeReplace, (unsigned char *)&mwm_hints,
				PROP_MOTIF_WM_HINTS_ELEMENTS);
	}

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

int set_window_geometry(Display *dpy, Window win, char *geometry){
	int xpos, ypos;
	unsigned int width, height, return_mask;
	XSizeHints	size_hints;

	size_hints.flags = PMinSize | PMaxSize;
	size_hints.min_width = size_hints.max_width = WIDTH;
	size_hints.min_height = size_hints.max_height = HEIGHT;

	XSetWMNormalHints(dpy, win, &size_hints);

	if (geometry){

		return_mask = XParseGeometry(geometry, &xpos, &ypos, &width, &height);

		if (return_mask & (WidthValue | HeightValue)){
			fprintf(stderr, "Can't resize windows\n");
		}

		if (return_mask & (XValue | YValue)){
			XMoveWindow(dpy, win, xpos, ypos);
		}
	}

	return 0;
}

int init_keycodes(Display *dpy){
	Shift_code = XKeysymToKeycode(dpy, XK_Shift_L);
	Control_code = XKeysymToKeycode(dpy, XK_Control_L);

	return 0;
}

void update_display(Display *dpy, Window toplevel, GC gc, int shift, int help){

	XClearWindow(dpy, toplevel);
	if (help) {
		XCopyArea(dpy, char_pixmaps[MAX_IMAGES - 1], toplevel, gc, 0,0, WIDTH, HEIGHT, 0, 0);
	} else if (shift) {
		XCopyArea(dpy, char_pixmaps[1], toplevel, gc, 0,0, WIDTH, HEIGHT, 0, 0);
	} else {
		XCopyArea(dpy, char_pixmaps[0], toplevel, gc, 0,0, WIDTH, HEIGHT, 0, 0);
	}
	XSync(dpy, False);
}

int main(int argc, char **argv)
{
	Display *dpy;
	char *display_name;
	Window toplevel;

	unsigned long valuemask;
	XGCValues xgc;
	GC gc;

	int event_base, error_base;
	int shape_ext_major, shape_ext_minor;

	char *config_path = NULL;
	char *config_geometry = NULL;
	char *switch_geometry = NULL;
	char decorations = 1; // 0 if decorations are to be hidden
	int loaded_config = 0;
	int run = 1;
	int options;
	int visible = 0;
	int buffer[MAX_GESTURES_BUFFER];
	int buffer_count = 0;
	int shift_modifier = 0;
	int ctrl_modifier = 0;
	int help_screen = 0;
	int i, status = 0;
	Time last_cross_timestamp = 0L;
	Time last_pressed = 0L;


	while ((options = getopt(argc, argv, "dc:g:")) != -1)
	{
		switch(options){
			case 'c':
				config_path = optarg;
				break;
			case 'g':
				switch_geometry = optarg;
				break;
			case 'd':
				printf("Decorations will be disabled");
				decorations = 0;
				break;

		}
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
		loaded_config = read_config(config_path, &config_geometry);
	} else {
		char config_path[MAX_CONFIG_PATH];
		char *home_dir = getenv("HOME");
		strncpy(config_path, home_dir, MAX_CONFIG_PATH);
		strncat(config_path + strlen(home_dir), CONFIG_FILE,
				MAX_CONFIG_PATH - strlen(home_dir));
		loaded_config = read_config(config_path, &config_geometry);
	}
#endif

	XShapeQueryVersion(dpy, &shape_ext_major, &shape_ext_minor);

	xgc.foreground = BlackPixel(dpy, DefaultScreen(dpy));
	xgc.background = WhitePixel(dpy, DefaultScreen(dpy));
	xgc.line_width = 2;
	valuemask = GCForeground | GCBackground | GCLineWidth;

	gc = XCreateGC(dpy, DefaultRootWindow(dpy), valuemask, &xgc);

	toplevel = XCreateWindow(dpy, DefaultRootWindow(dpy), 0, 0,
			WIDTH, HEIGHT, 0, CopyFromParent, CopyFromParent,
			CopyFromParent, 0, NULL);

	init_regions(dpy, toplevel);

	set_window_properties(dpy, toplevel, decorations);

	init_keycodes(dpy);

	XSelectInput(dpy, toplevel, SubstructureNotifyMask | StructureNotifyMask | ExposureMask);

	XMapWindow(dpy, toplevel);

	if (switch_geometry){
		set_window_geometry(dpy, toplevel, switch_geometry);
	} else {
		set_window_geometry(dpy, toplevel, config_geometry);
	}
	if (config_geometry) {
		free(config_geometry);
	}

	for( i = 0; i < MAX_IMAGES; i++) {
		char_pixmaps[i] = XCreatePixmap(dpy, toplevel, WIDTH, HEIGHT,
				DefaultDepth(dpy, DefaultScreen(dpy)));
		status |= load_charset(dpy, gc, i);
	}
	if (status)
		run = 0;

	update_display(dpy, toplevel, gc, shift_modifier, help_screen);

	while(run) {
		XEvent e;
		char *region_name;
		int region;
		int state_mod = 0;
		KeyCode code;
		KeySym ksym;

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
				ksym = char_free[buffer[0]][region];
				if (ksym != XK_Control_L) {
					code = XKeysymToKeycode(dpy, ksym);
					for (state_mod = 0; state_mod < 4; state_mod++) {
						if (XKeycodeToKeysym(dpy, code, state_mod) == ksym ) {
							break;
						}
					}
					if (state_mod)
						XTestFakeKeyEvent(dpy, Shift_code, True, 0);
					XTestFakeKeyEvent(dpy, code, True, 0);
					XTestFakeKeyEvent(dpy, code, False, 0);
					if (state_mod)
						XTestFakeKeyEvent(dpy, Shift_code, False, 0);
				} else
					ctrl_modifier = 1;
				buffer_count = 0;
				break;
			case EnterNotify:
				XFetchName(dpy, e.xcrossing.window, &region_name);
				region = region_name[0] - 48;
				XFree(region_name);
				KeySym index;

				char c = '\0';

				if ((region == 0) && buffer_count > 1 && buffer[0] == 0){

					if ((buffer[1] == buffer[buffer_count - 1]) && loaded_config && (buffer_count > 2)) {
						buffer_count = (buffer_count - 1) >> 1;

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
						index = (KeySym) c;
					}
					for (state_mod = 0; state_mod < 4; state_mod++) {
						if (XKeycodeToKeysym(dpy, code, state_mod) == index) {
							break;
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
						} else if (shift_modifier || help_screen) {
							shift_modifier = help_screen = 0;
						} else if (buffer_count == 4) {
							shift_modifier = 1;
						} else if (buffer_count == 5) {
							shift_modifier = 2;
						} else if (buffer_count == 6) {
							help_screen = 1;
						}

						if (buffer_count != 2) {
							XClearWindow(dpy, toplevel);
							update_display(dpy, toplevel, gc, shift_modifier, help_screen);
							buffer_count = 1;
							buffer[0] = 0;
							break;
						}
					} else if (code) {
						if ((shift_modifier && isalpha(c)) || state_mod) {
							XTestFakeKeyEvent(dpy, Shift_code, True, 0);
							XTestFakeKeyEvent(dpy, code, True, 0);
							XTestFakeKeyEvent(dpy, code, False, 0);
							XTestFakeKeyEvent(dpy, Shift_code, False, 0);
						} else if (ctrl_modifier) {
							XTestFakeKeyEvent(dpy, Control_code, True, 0);
							XTestFakeKeyEvent(dpy, code, True, 0);
							XTestFakeKeyEvent(dpy, code, False, 0);
							XTestFakeKeyEvent(dpy, Control_code, False, 0);
							ctrl_modifier = 0;
						} else {
							XTestFakeKeyEvent(dpy, code, True, 0);
							XTestFakeKeyEvent(dpy, code, False, 0);
						}
						if (shift_modifier == 1) {
							shift_modifier = 0;
							XClearWindow(dpy, toplevel);
							update_display(dpy, toplevel, gc, shift_modifier, help_screen);
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
			case Expose:
			case ConfigureNotify:
				XMapWindow(dpy, toplevel);
				update_display(dpy, toplevel, gc, shift_modifier, help_screen);
			case ClientMessage:
				if ((e.xclient.message_type == mb_im_invoker_command) ||
					(e.xclient.message_type == mtp_im_invoker_command)) {
					if (e.xclient.data.l[0] == KeyboardShow) {
						XMapWindow(dpy, toplevel);
						update_display(dpy, toplevel, gc, shift_modifier, help_screen);
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
							update_display(dpy, toplevel, gc, shift_modifier, help_screen);
							visible = 1;
						}
					}
					break;
				}
				if (e.xclient.data.l[0] == wmDeleteMessage)
					run = 0;
				break;
			}
	}

	XFreeGC(dpy, gc);
	XFreePixmap(dpy, char_pixmaps[0]);
	XFreePixmap(dpy, char_pixmaps[1]);
	XDestroyWindow(dpy, toplevel);
	XCloseDisplay(dpy);

	exit(0);
}

