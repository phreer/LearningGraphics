#include <X11/X.h>
#include <stdlib.h>
#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define INIT_WIDTH  800
#define INIT_HEIGHT 600
#define INIT_X 200
#define INIT_Y 200
#define BORDER_WIDTH 10

int main(int argc, char *argv[]) {
  Display *my_display;
  XSetWindowAttributes attrs;
  Window my_window;
  XSizeHints size_hints;
  XWMHints wm_hints;
  XTextProperty window_name_property, icon_name_property;
  XEvent event;
  
  char *window_name = "Basic";
  char *icon_name = "Ba";

  int screen_num, done = 0;
  unsigned long mask;

  /* Get connection */
  if ((my_display = XOpenDisplay(NULL)) == NULL) {
    fprintf(stderr, "Failed to open display: %s.\n", getenv("DISPLAY"));
    exit(EXIT_FAILURE);
  }

  /* Create top level window */
  screen_num = XDefaultScreen(my_display);
  attrs.background_pixel = BlackPixel(my_display, screen_num);
  attrs.border_pixel = BlackPixel(my_display, screen_num);
  attrs.event_mask = ButtonPressMask | KeyPressMask;
  mask = CWBackPixel | CWBorderPixel | CWEventMask;
  my_window = XCreateWindow(my_display, RootWindow(my_display, screen_num),
                            INIT_X, INIT_Y, INIT_WIDTH, INIT_HEIGHT, BORDER_WIDTH,
                            DefaultDepth(my_display, screen_num), InputOutput, DefaultVisual(my_display, screen_num),
                            mask, &attrs);
  
  /* Give window manager hints */
  size_hints.flags = USPosition | USSize;
  XSetWMNormalHints(my_display, my_window, &size_hints);
  
  wm_hints.initial_state = NormalState;
  wm_hints.flags = StateHint;
  XSetWMHints(my_display, my_window, &wm_hints);

  XStringListToTextProperty(&window_name, 1, &window_name_property);
  XSetWMName(my_display, my_window, &window_name_property);
  XStringListToTextProperty(&icon_name, 1, &icon_name_property);
  XSetWMIconName(my_display, my_window, &icon_name_property);


  printf("Start showing window\n");
  XMapWindow(my_display, my_window);
  
  while (!done) {
    XNextEvent(my_display, &event);
    switch (event.type) {
      case ButtonPress:
        done = 1;
        break;
      case KeyPress:
        printf("KeyPress Event occurred.\n");
        break;
    }
  }
  
  
  /* Final step: clean up */
  XUnmapWindow(my_display, my_window);
  XDestroyWindow(my_display, my_window);
  XCloseDisplay(my_display);
  
  return EXIT_SUCCESS;
}
