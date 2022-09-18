/*
 * color-drawing.c - demonstrate drawing of pixels, lines, arcs, etc, using
 *		      different foreground colors, in a window.
 */

#include <X11/Xlib.h>

#include <stdio.h>
#include <stdlib.h> /* getenv(), etc. */
#include <unistd.h> /* sleep(), etc.  */

GC create_gc(Display *display, Window win, int reverse_video)
{

  GC gc;                       /* handle of newly created GC.  */
  unsigned long valuemask = 0; /* which values in 'values' to  */
                               /* check when creating the GC.  */
  XGCValues values;            /* initial values for the GC.   */
  unsigned int line_width = 2; /* line width for the GC.       */
  int line_style = LineSolid;  /* style for lines drawing and  */
  int cap_style = CapButt;     /* style of the line's edje and */
  int join_style = JoinBevel;  /*  joined lines.		*/
  int screen_num = DefaultScreen(display);
  gc = XCreateGC(display, win, valuemask, &values);
  if (gc < 0)
  {
    fprintf(stderr, "XCreateGC: \n");
  }

  XSetForeground(display, gc, BlackPixel(display, screen_num));
  XSetBackground(display, gc, WhitePixel(display, screen_num));
  XSetFillStyle(display, gc, FillSolid);

  return gc;
}

int main(int argc, char *argv[])
{

  Display *display;
  display = XOpenDisplay(NULL);
  int screen_num = DefaultScreen(display);
  XGCValues gr_values;
  Visual *visual;
  visual = DefaultVisual(display, screen_num);
  int depth = DefaultDepth(display, screen_num);
  XSetWindowAttributes attributes;
  Window win;
  win = XCreateWindow(display, XRootWindow(display, screen_num),
                      0, 0, 800, 600, 5, depth, InputOutput,
                      visual, CWBackPixel, &attributes);
  XMapWindow(display, win);
  GC gc = create_gc(display, win, 0);

  XEvent event;
  KeySym key;
  int i;
  char text[10];
  int done = 0;
  while (done == 0)
  {

    XNextEvent(display, &event);
    switch (event.type)
    {

    case Expose:
      // XSetForeground(display, gc, BlackPixel(display, screen_num));
      // XDrawRectangle(display, win, gc, 50, 50, 100, 100);
      // XFlush(display);
      break;
    case ButtonPress:

      XDrawRectangle(display, win, gc, 10, 10, 10, 10);
      break;

    case KeyPress:

      XDrawRectangle(display, win, gc, 100, 100, 10, 10);
      // int i = XLookupString(&event, text, 10, &key, 0);
      // if (i == 1 && text[0] == 'q')
      //   done = 1;

      break;
    }
  }

  XFreeGC(display, gc);
  XCloseDisplay(display);
}
