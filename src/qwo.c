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
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#include <gtk/gtk.h>

#define WIDTH   300
#define HEIGHT  300

#define MAX_REGIONS 9
#define MAX_POINTS  9

#define DELTA                   (WIDTH >> 4)
#define BORDER_WIDTH            3

#define GRID_COLOR				"orange"
//#define DEFAULT_FONT    "-*-courier-bold-*-*-12-*-*-*-*-*-*-*"
#define DEFAULT_FONT    "fixed"

#define MAX_CHARSET      3
#define MAX_CHAR         32 // Chars per charset

#define MAX_GESTURES_BUFFER      3
#define QUIT_GESTURE			 8

#define LONG_EXPOSURE_DELAY		2000L

#define FILL_REGION4(a, b, c, d)        {a, b, c, d, a, a, a, a, a}
#define FILL_REGION5(a, b, c, d, e)     {a, b, c, d, e, a, a, a, a}
#define FILL_REGION8(a, b, c, d, e, f, g, h)     {a, b, c, d, e, f, g, h, a}

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#define CHAR_FROM_GESTURE(a, b)  \
	((ABS(b - a) < MAX_GESTURES_BUFFER ) ? (((a - 1) << 2) + b - a) :\
	 ((b - a) < 0) ? (((a - 1) << 2) + ((b - a) & (8 - 1))) : \
	 (1 << 5) + (b - a) - (1 << 3))

char charsets[][MAX_CHAR] = {
	"ab>ced<fighj>klmonpqrs.tuvw,<xyz",
	"AB>CED<FIGHJ>KLMONPQRS.TUVW,<XYZ",
	"1-!?2?$+3')4>5@\n6;&,7.:/8_=9<0(\""};

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

void init_regions(Display *dpy, Window toplevel){
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

int load_font(Display *dpy, XFontStruct **font_info, char *font){
	if (font == NULL)
		font = DEFAULT_FONT;
	if ((*font_info = XLoadQueryFont(dpy, font)) == NULL){
		fprintf( stderr, "Cannot open font %s\n", font);
		return 0;
	}
	return 1;
}

void display_charset(Display *dpy, Window win, GC gc, XFontStruct *font_info,
		char *charset){
	int len;
	int font_height;
	int i;
	int offset;
	XTextItem item = { NULL, 1, 0, font_info->fid };

	font_height = font_info->ascent + font_info->descent;
	//lbearing from Xcharstruct seems more appropriate
	len = XTextWidth(font_info, charset, 1);
	offset = WIDTH / 8;
	item.chars = (char *) charset;
	XDrawText(dpy, win, gc, len, font_height, &item, 1);
	item.chars = (char *) charset + 24;
	XDrawText(dpy, win, gc, len, HEIGHT - font_info->descent,
			&item, 1);
	for (i = 1; i < 8 ; i++){
		item.chars = (char *) charset + i;
		XDrawText(dpy, win, gc, offset * i, font_height, &item, 1);
		item.chars = (char *) charset + i + 8;
		XDrawText(dpy, win, gc, WIDTH - len, offset * i + font_info->ascent,
				&item, 1);
		item.chars = (char *) charset + (24 - i);
		XDrawText(dpy, win, gc, offset * i,
				HEIGHT - font_info->descent, &item, 1);
		item.chars = (char *) charset + 32 - i;
		XDrawText(dpy, win, gc, font_info->max_bounds.lbearing,
				offset * i + font_info->ascent, &item, 1);
	}
	item.chars = (char *) charset + 8;
	XDrawText(dpy, win, gc, offset * 8 - len, font_height, &item, 1);
	item.chars = (char *) charset + 16;
	XDrawText(dpy, win, gc, offset * 8 - len, HEIGHT - font_info->descent,
			&item, 1);
}

void draw_grid(Display *dpy, Window toplevel, GC gc){
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

int main(int argc, char **argv){
	Display *dpy;
	char *display_name;
	Window toplevel, global_window;
	XFontStruct *font_info;
	XSetWindowAttributes attributes;
	unsigned long valuemask;
	int event_base, error_base;
	int shape_ext_major, shape_ext_minor;
	XGCValues xgc;
	GC gc;
	int run = 0;
	int options;
	char *font_name = NULL;
	int current_charset = 1;
	int buffer[MAX_GESTURES_BUFFER];
	int buffer_count = 0;
	int invalid_gesture = 0;
	int modifier = 1;
	Time last_cross_timestamp = 0L;
	GtkWidget *gtk_text;
	GtkTextBuffer *text;
	GtkTextIter end, start;
#ifdef TEXT_VIEW
	GtkWidget *gtk_window;
#endif

	options = getopt(argc, argv, "f:");

	switch(options){
		case 'f':
			font_name = optarg;
			break;
	}

	display_name = XDisplayName(NULL);
	dpy = XOpenDisplay(display_name);

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

	gtk_init(NULL, NULL);
	gtk_text = gtk_text_view_new ();
	text = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_text));
#ifdef TEXT_VIEW
	gtk_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(gtk_window), 200, 100);
	gtk_container_add(GTK_CONTAINER (gtk_window), gtk_text);
	gtk_widget_show(gtk_text);
	gtk_widget_show(gtk_window);
#endif

	while (gtk_events_pending())
		gtk_main_iteration();

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

	XSelectInput(dpy, toplevel, ExposureMask | ButtonPress);

	if(!(load_font(dpy, &font_info, font_name))) {
		XDestroyWindow(dpy, toplevel);
		XCloseDisplay(dpy);
		exit(-1);
	}

	XSetFont(dpy, gc, font_info->fid);

	draw_grid(dpy, toplevel, gc);
	display_charset(dpy, toplevel, gc, font_info,
			charsets[current_charset]);
	XSync(dpy, False);

	while(!run) {
		XEvent e;
		char *region_name;

		XNextEvent(dpy, &e);
		switch (e.type) {
			case EnterNotify:
				XFetchName(dpy, e.xcrossing.window, &region_name);
				char region = region_name[0] - 48;
				if (region == 0)
					run = 1;
		}
	}

	while(run) {
		XEvent e;
		char *region_name;

		XNextEvent(dpy, &e);
		switch (e.type) {
			case Expose:
				draw_grid(dpy, toplevel, gc);
				display_charset(dpy, toplevel, gc, font_info,
						charsets[current_charset]);
				XSync(dpy, False);
				break;
			case ButtonPress:
				XClearWindow(dpy, toplevel);
				draw_grid(dpy, toplevel, gc);
				current_charset = (current_charset + 1) % MAX_CHARSET;
				display_charset(dpy, toplevel, gc, font_info,
						charsets[(current_charset)]);
				XSync(dpy, False);
				break;
			case LeaveNotify:
				XFetchName(dpy, e.xcrossing.window, &region_name);
				break;
			case EnterNotify:
				XFetchName(dpy, e.xcrossing.window, &region_name);
				char region = region_name[0] - 48;

				if (invalid_gesture) {
					if (region == 0) {
						invalid_gesture = 0;
						buffer_count = 0;
					}
					else if (buffer_count == QUIT_GESTURE) {
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
					if (buffer_count == 1) {
						gchar c = g_utf8_get_char(&(charsets[current_charset]
									[( buffer[0] - 1) << 2]));
						if (c == '<') {
							if (e.xcrossing.time - last_cross_timestamp > LONG_EXPOSURE_DELAY) {
								gtk_text_buffer_get_bounds(text, &start, &end);
								gtk_text_buffer_delete(text, &start, &end);
							} else {
							gtk_text_buffer_get_iter_at_offset(text, &end, -1);
							start = end;
							gtk_text_iter_backward_char(&end);
							gtk_text_buffer_delete(text, &start, &end);
							}
						}
						else {
							if (c == '>') {
								char ch = ' ';
								gtk_text_buffer_insert_at_cursor(text, &ch , 1);
							}
							else {
								gtk_text_buffer_insert_at_cursor(text, &c, 1);
							}
						}
						if (modifier){
							XClearWindow(dpy, toplevel);
							draw_grid(dpy, toplevel, gc);
							current_charset = modifier = 0;
							display_charset(dpy, toplevel, gc, font_info,
									charsets[current_charset]);
						}
					} else {
						gchar c = g_utf8_get_char(&(charsets[current_charset]
									[CHAR_FROM_GESTURE(buffer[0], buffer[buffer_count - 1])]));
						if ((!current_charset) && ((c == '<') || (c == '>'))){
							modifier = 1;
							if (c == '<') {
								current_charset = MAX_CHARSET - 1;
							}
							else {
								current_charset = 1;
							}
							XClearWindow(dpy, toplevel);
							draw_grid(dpy, toplevel, gc);
							display_charset(dpy, toplevel, gc, font_info,
									charsets[current_charset]);
							XSync(dpy, False);
							buffer_count = 0;
							break;
						} else {
							gtk_text_buffer_insert_at_cursor(text, &c, 1);
						}
						if (modifier){
							XClearWindow(dpy, toplevel);
							draw_grid(dpy, toplevel, gc);
							current_charset = modifier = 0;
							display_charset(dpy, toplevel, gc, font_info,
									charsets[current_charset]);
						}
					}
					while (gtk_events_pending())
						gtk_main_iteration();
					buffer_count = 0;
					break;
				}
				if(!buffer_count) {
					last_cross_timestamp = e.xcrossing.time;
				}
				buffer[buffer_count] = region_name[0] - 48;
				buffer_count++;
				break;
				default:
					printf("Received event %i\n", e.type);
				}
		}

		gtk_text_buffer_get_bounds(text, &start, &end);
		fprintf(stdout, "%s\n", (char *)gtk_text_buffer_get_text(text, &start, &end, FALSE));
		//gtk_exit(0);
		XFreeFont(dpy, font_info);
		XFreeGC(dpy, gc);
		XDestroyWindow(dpy, toplevel);
		XCloseDisplay(dpy);

		exit(0);
}
