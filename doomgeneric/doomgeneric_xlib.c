#include "doomkeys.h"

#include "doomgeneric.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "gmplay.h"

static Display *s_Display = NULL;
static Window s_Window = 0;
static int s_Screen = 0;
static GC s_Gc = 0;
static XImage *s_Image = NULL;

#define RENDER_RESX KINDLE_RESY
#define RENDER_RESY KINDLE_RESX

#ifdef KINDLE
#define WINDOW_RESX KINDLE_RESX
#define WINDOW_RESY KINDLE_RESY
#else
#define WINDOW_RESX RENDER_RESX
#define WINDOW_RESY RENDER_RESY
#endif

#define WINDOW_RESX RENDER_RESX
#define WINDOW_RESY RENDER_RESY

#define CENTER_OFFSET_X ((RENDER_RESX - DOOMGENERIC_RESX) / 2)
#define CENTER_OFFSET_Y ((RENDER_RESY - DOOMGENERIC_RESY) / 2)

#define XY_TOINDEX(x, y) ((y)*RENDER_RESX + (x))

uint8_t *greyScaleBuffer = 0;
#ifdef X_PC
uint8_t *displayBuffer = 0;
#endif

typedef enum _ButtonId_ {
  BTN_ENTER = 0,
  BTN_ESCAPE,
  BTN_LEFT,
  BTN_RIGHT,
  BTN_UP,
  BTN_DOWN,
  BTN_FIRE,
  BTN_USE,
  BTN_Y,
  BTN_N,
  BTN_NONE,
} ButtonId;

typedef struct _TouchButton_ {
  int x;
  int y;
  int width;
  int height;
  bool isPressed;
  bool wasPressed;
  unsigned int keyCode;
} TouchButton;

#define BUTTON_COUNT BTN_NONE
TouchButton buttons[BUTTON_COUNT];

unsigned int touchMoveInitX = 0;

#define KEYQUEUE_SIZE 16

static unsigned short s_KeyQueue[KEYQUEUE_SIZE];
static unsigned int s_KeyQueueWriteIndex = 0;
static unsigned int s_KeyQueueReadIndex = 0;

static unsigned char convertToDoomKey(unsigned int key) {
  switch (key) {
  case XK_Return:
    key = KEY_ENTER;
    break;
  case XK_Escape:
    key = KEY_ESCAPE;
    break;
  case XK_Left:
    key = KEY_LEFTARROW;
    break;
  case XK_Right:
    key = KEY_RIGHTARROW;
    break;
  case XK_Up:
    key = KEY_UPARROW;
    break;
  case XK_Down:
    key = KEY_DOWNARROW;
    break;
  case XK_Control_L:
  case XK_Control_R:
    key = KEY_FIRE;
    break;
  case XK_space:
    key = KEY_USE;
    break;
  case XK_Shift_L:
  case XK_Shift_R:
    key = KEY_RSHIFT;
    break;
  default:
    key = tolower(key);
    break;
  }

  return key;
}

static void addKeyToQueue(int pressed, unsigned int keyCode) {
  unsigned char key = convertToDoomKey(keyCode);

  unsigned short keyData = (pressed << 8) | key;

  s_KeyQueue[s_KeyQueueWriteIndex] = keyData;
  s_KeyQueueWriteIndex++;
  s_KeyQueueWriteIndex %= KEYQUEUE_SIZE;
}

void DG_Init() {
  memset(s_KeyQueue, 0, KEYQUEUE_SIZE * sizeof(unsigned short));

  // window creation

  s_Display = XOpenDisplay(NULL);

  s_Screen = DefaultScreen(s_Display);

  int blackColor = BlackPixel(s_Display, s_Screen);
  int whiteColor = WhitePixel(s_Display, s_Screen);

  XSetWindowAttributes attr;
  memset(&attr, 0, sizeof(XSetWindowAttributes));
  attr.event_mask = ExposureMask | KeyPressMask;
  attr.background_pixel = BlackPixel(s_Display, s_Screen);

  int depth = DefaultDepth(s_Display, s_Screen);

  s_Window =
      XCreateSimpleWindow(s_Display, DefaultRootWindow(s_Display), 0, 0,
                          WINDOW_RESX, WINDOW_RESY, 0, blackColor, blackColor);

  const char windowTitle[] = "L:A_N:application_ID:me.himbeer.doom_PC:N_O:U";
  XChangeProperty(s_Display, s_Window, XA_WM_NAME, XA_STRING, 8,
                  PropModeReplace, windowTitle, strlen(windowTitle));

  XSelectInput(s_Display, s_Window,
               StructureNotifyMask | KeyPressMask | KeyReleaseMask |
                   PointerMotionMask | ButtonPressMask | ButtonReleaseMask);

  XMapWindow(s_Display, s_Window);

  s_Gc = XCreateGC(s_Display, s_Window, 0, NULL);

  XSetForeground(s_Display, s_Gc, whiteColor);

  XkbSetDetectableAutoRepeat(s_Display, 1, 0);

  // Wait for the MapNotify event

  while (1) {
    XEvent e;
    XNextEvent(s_Display, &e);
    if (e.type == MapNotify) {
      break;
    }
  }

  Visual *visual = DefaultVisual(s_Display, s_Screen);

#ifdef X_PC
  s_Image =
      XCreateImage(s_Display, visual, depth, ZPixmap, 0, (char *)displayBuffer,
                   RENDER_RESX, RENDER_RESY, 32, 0);
#endif
}

void DG_DrawFrame() {
  unsigned int x = 0, y = 0, r = 0, g = 0, b = 0, intensity = 0;
  int touchX = 0, touchY = 0;
  ButtonId pressedButton = BTN_NONE;
  bool notTouchingAnyButton = true;
  size_t i = 0;
  unsigned char *currentPixel = (unsigned char *)DG_ScreenBuffer;
#ifdef X_PC
  uint8_t currentGreyPixelValue = 0;
#endif

  if (s_Display) {
    memset(greyScaleBuffer, 0, RENDER_RESX * RENDER_RESY);

    while (XPending(s_Display) > 0) {
      XEvent e;
      XNextEvent(s_Display, &e);

      switch (e.type) {
      case KeyPress: {
        KeySym sym = XKeycodeToKeysym(s_Display, e.xkey.keycode, 0);

        addKeyToQueue(1, sym);
        break;
      }
      case KeyRelease: {
        KeySym sym = XKeycodeToKeysym(s_Display, e.xkey.keycode, 0);
        addKeyToQueue(0, sym);
        break;
      }
      case ButtonPress:
      case ButtonRelease:
      case MotionNotify: {
#ifdef KINDLE
        if (e.type == MotionNotify) {
          touchY = (RENDER_RESY - 1) - e.xmotion.x;
          touchX = e.xmotion.y;
        } else {
          touchY = (RENDER_RESY - 1) - e.xbutton.x;
          touchX = e.xbutton.y;
        }
#else
        if (e.type == MotionNotify) {
          touchX = e.xmotion.x;
          touchY = e.xmotion.y;
        } else {
          touchX = e.xbutton.x;
          touchY = e.xbutton.y;
        }
#endif
        if ((touchX < 0 || touchX > (RENDER_RESX - 1)) ||
            (touchY < 0 || touchY > (RENDER_RESY - 1))) {
          // Ignore movement outside the window
          break;
        }

        if (e.type == ButtonPress || e.type == ButtonRelease) {
          if (e.xbutton.button != 1) {
            // Ignore Kindle Gestures
            break;
          }
        }

        notTouchingAnyButton = true;
        for (i = 0; i < BUTTON_COUNT; i++) {
          buttons[i].wasPressed = buttons[i].isPressed;
          buttons[i].isPressed = false;
          if ((touchX > buttons[i].x &&
               touchX < (buttons[i].x + buttons[i].width)) &&
              (touchY > buttons[i].y &&
               touchY < (buttons[i].y + buttons[i].height))) {
            if (e.type == ButtonPress ||
                (e.type == MotionNotify && (e.xmotion.state & Button1Mask))) {
              buttons[i].isPressed = true;
              notTouchingAnyButton = false;
            }
          }
        }

        if (e.type == ButtonRelease) {
          // printf("Cancel touch move\n");
          touchMoveInitX = 0;
        } else if (notTouchingAnyButton && e.type == ButtonPress &&
                   touchX > 10) {
          // printf("Init touch move\n");
          touchMoveInitX = touchX;
        } else if (notTouchingAnyButton && touchMoveInitX != 0 &&
                   e.type == MotionNotify) {
          if (touchX < (touchMoveInitX - 10)) {
            // printf("Touch move left\n");
            buttons[BTN_LEFT].isPressed = true;
          } else if (touchX > (touchMoveInitX + 10)) {
            // printf("Touch move right\n");
            buttons[BTN_RIGHT].isPressed = true;
          }
        }

        for (i = 0; i < BUTTON_COUNT; i++) {
          if (buttons[i].isPressed) {
            if (!buttons[i].wasPressed) {
              // printf("Down %u\n", buttons[i].keyCode);
              addKeyToQueue(1, buttons[i].keyCode);
            }
          } else if (buttons[i].wasPressed) {
            // printf("Up %u\n", buttons[i].keyCode);
            addKeyToQueue(0, buttons[i].keyCode);
          }
        }

        // greyScaleBuffer[XY_TOINDEX(touchX, touchY)] = 255;

        break;
      }
      }
    }

    for (y = 0; y < DOOMGENERIC_RESY; y++) {
      for (x = 0; x < DOOMGENERIC_RESX; x++) {
        r = currentPixel[2];
        g = currentPixel[1];
        b = currentPixel[0];
        intensity = (r + g + b) / 3;

        greyScaleBuffer[XY_TOINDEX(x + CENTER_OFFSET_X, y + CENTER_OFFSET_Y)] =
            intensity;

        currentPixel += 4;
      }
    }

    for (y = 0; y < RENDER_RESY; y++) {
      for (x = 0; x < RENDER_RESX; x++) {
        for (i = 0; i < BUTTON_COUNT; i++) {
          if ((x > buttons[i].x && x < (buttons[i].x + buttons[i].width)) &&
              (y > buttons[i].y && y < (buttons[i].y + buttons[i].height))) {
            if (buttons[i].isPressed) {
              greyScaleBuffer[XY_TOINDEX(x, y)] = 128;
            } else {
              greyScaleBuffer[XY_TOINDEX(x, y)] = 255;
            }
          }
        }
      }
    }

#ifdef X_PC
    currentPixel = (unsigned char *)displayBuffer;
    for (y = 0; y < RENDER_RESY; y++) {
      for (x = 0; x < RENDER_RESX; x++) {
        currentGreyPixelValue = greyScaleBuffer[(RENDER_RESX * y) + x];
        currentPixel[0] = currentGreyPixelValue;
        currentPixel[1] = currentGreyPixelValue;
        currentPixel[2] = currentGreyPixelValue;
        currentPixel += 4;
      }
    }

    XPutImage(s_Display, s_Window, s_Gc, s_Image, 0, 0, 0, 0, RENDER_RESX,
              RENDER_RESY);
#endif

    gm_show_fb(greyScaleBuffer);
  }
}

void DG_SleepMs(uint32_t ms) { usleep(ms * 1000); }

uint32_t DG_GetTicksMs() {
  struct timeval tp;
  // struct timezone tzp;

  // gettimeofday(&tp, &tzp);
  gettimeofday(&tp, NULL);

  return (tp.tv_sec * 1000) + (tp.tv_usec / 1000); /* return milliseconds */
}

int DG_GetKey(int *pressed, unsigned char *doomKey) {
  if (s_KeyQueueReadIndex == s_KeyQueueWriteIndex) {
    // key queue is empty

    return 0;
  } else {
    unsigned short keyData = s_KeyQueue[s_KeyQueueReadIndex];
    s_KeyQueueReadIndex++;
    s_KeyQueueReadIndex %= KEYQUEUE_SIZE;

    *pressed = keyData >> 8;
    *doomKey = keyData & 0xFF;

    return 1;
  }
}

void DG_SetWindowTitle(const char *title) {
  // Kindle window title must be constant
}

void addButton(ButtonId id, int x, int y, int width, int height,
               unsigned int keyCode) {
  TouchButton btn = {x, y, width, height, false, false, keyCode};
  buttons[id] = btn;
}

int main(int argc, char **argv) {
  greyScaleBuffer = malloc(RENDER_RESX * RENDER_RESY);
#ifdef X_PC
  displayBuffer = malloc(RENDER_RESX * RENDER_RESY * 4);
  memset(displayBuffer, 0, RENDER_RESX * RENDER_RESY * 4);
#endif

#ifdef KINDLE
  printf("Compiled for Kindle!\n");
#endif
#ifdef X_PC
  printf("Compiled for PC!\n");
#endif

  //600 × 800
  /*addButton(BTN_ENTER, 380, 505, 40, 40, KEY_ENTER);
  addButton(BTN_ESCAPE, 20, 20, 40, 40, KEY_ESCAPE);
  addButton(BTN_LEFT, 20, 500, 40, 40, KEY_LEFTARROW);
  addButton(BTN_RIGHT, 100, 500, 40, 40, KEY_RIGHTARROW);
  addButton(BTN_UP, 60, 460, 40, 40, KEY_UPARROW);
  addButton(BTN_DOWN, 60, 540, 40, 40, KEY_DOWNARROW);
  addButton(BTN_FIRE, 740, 500, 40, 40, KEY_FIRE);
  addButton(BTN_USE, 690, 500, 40, 40, KEY_USE);
  addButton(BTN_Y, 355, 550, 40, 20, 'y');
  addButton(BTN_N, 405, 550, 40, 20, 'n');*/

  //1072 × 1448
  addButton(BTN_ENTER, 625, 872, 100, 100, KEY_ENTER);
  addButton(BTN_ESCAPE, 20, 20, 100, 100, KEY_ESCAPE);
  addButton(BTN_LEFT, 20, 850, 100, 100, KEY_LEFTARROW);
  addButton(BTN_RIGHT, 220, 850, 100, 100, KEY_RIGHTARROW);
  addButton(BTN_UP, 120, 750, 100, 100, KEY_UPARROW);
  addButton(BTN_DOWN, 120, 950, 100, 100, KEY_DOWNARROW);
  addButton(BTN_FIRE, 1300, 850, 100, 100, KEY_FIRE);
  addButton(BTN_USE, 1150, 850, 100, 100, KEY_USE);
  addButton(BTN_Y, 500, 982, 100, 50, 'y');
  addButton(BTN_N, 750, 982, 100, 50, 'n');

  gmlib(GMLIB_INIT);

  doomgeneric_Create(argc, argv);

  for (int i = 0;; i++) {
    doomgeneric_Tick();
  }

  // I think these are useless because it never exists through that way anyways
  free(greyScaleBuffer);
#ifdef X_PC
  free(displayBuffer);
#endif

  gmlib(GMLIB_CLOSE);
  XCloseDisplay(s_Display);

  return 0;
}
