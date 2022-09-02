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
#define SUBWINDOW_INIT_WIDTH  100
#define SUBWINDOW_INIT_HEIGHT 100
#define SUBWINDOW_INIT_X 300
#define SUBWINDOW_INIT_Y 300

#define APP_NAME    "Moving"
#define WINDOW_NAME APP_NAME
#define ICON_NAME   "Mo"

int main(int argc, char *argv[]) {
  Display *my_display;
  XSetWindowAttributes attrs;
  Window root_window, sub_window;
  XSizeHints size_hints;
  XWMHints wm_hints;
  XTextProperty window_name_property, icon_name_property;
  XEvent event;
  XWindowChanges changes;
  int subwindow_x, subwindow_y;
  
  char *window_name = WINDOW_NAME;
  char *icon_name = ICON_NAME;

  int screen_num, done;
  unsigned long mask;

  /* Get connection */
  if ((my_display = XOpenDisplay(NULL)) == NULL) {
    fprintf(stderr, "Failed to open display: %s.\n", getenv("DISPLAY"));
    exit(EXIT_FAILURE);
  }

  /* Create top level window */
  screen_num = XDefaultScreen(my_display);
  attrs.background_pixel = WhitePixel(my_display, screen_num);
  attrs.border_pixel = BlackPixel(my_display, screen_num);
  attrs.event_mask = ButtonPressMask | KeyPressMask | StructureNotifyMask;
  mask = CWBackPixel | CWBorderPixel | CWEventMask;
  root_window = XCreateWindow(my_display,
                              RootWindow(my_display, screen_num),
                              INIT_X, INIT_Y,
                              INIT_WIDTH, INIT_HEIGHT,
                              BORDER_WIDTH,
                              DefaultDepth(my_display, screen_num),
                              InputOutput,
                              DefaultVisual(my_display, screen_num),
                              mask,
                              &attrs);
  
  /* Give window manager hints */
  size_hints.flags = USPosition | USSize;
  XSetWMNormalHints(my_display, root_window, &size_hints);
  
  wm_hints.initial_state = NormalState;
  wm_hints.flags = StateHint;
  XSetWMHints(my_display, root_window, &wm_hints);

  XStringListToTextProperty(&window_name, 1, &window_name_property);
  XSetWMName(my_display, root_window, &window_name_property);
  XStringListToTextProperty(&icon_name, 1, &icon_name_property);
  XSetWMIconName(my_display, root_window, &icon_name_property);

  /* Create sub-window */
  subwindow_x = SUBWINDOW_INIT_X;
  subwindow_y = SUBWINDOW_INIT_Y;
  attrs.background_pixel = BlackPixel(my_display, screen_num);
  sub_window = XCreateWindow(my_display,
                             root_window,
                             subwindow_x, subwindow_y,
                             SUBWINDOW_INIT_WIDTH, SUBWINDOW_INIT_HEIGHT,
                             BORDER_WIDTH,
                             DefaultDepth(my_display, screen_num),
                             InputOutput,
                             DefaultVisual(my_display, screen_num),
                             mask,
                             &attrs);

  
  XMapWindow(my_display, root_window);
  XMapWindow(my_display, sub_window);
  
  mask = CWX | CWY;
  while (!done) {
    XNextEvent(my_display, &event);
    switch (event.type) {
      case ButtonPress:
        done = 1;
        break;
      case KeyPress:
        printf("KeyPress Event occurred.\n");
        subwindow_x += 10;
        subwindow_y += 10;
        changes.x = subwindow_x;
        changes.y = subwindow_y;
        XConfigureWindow(my_display, sub_window, mask, &changes);
        break;
      case ConfigureNotify:
        XMapWindow(my_display, sub_window);
    }
  }
  
  
  /* Final step: clean up */
  XUnmapWindow(my_display, root_window);
  XDestroyWindow(my_display, root_window);
  XCloseDisplay(my_display);
  
  return EXIT_SUCCESS;
}
