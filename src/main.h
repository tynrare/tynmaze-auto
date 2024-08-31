#include <raylib.h>

#ifndef __DEBUG__
#define RES_PATH "res/"
#else
#define RES_PATH "../res/"
#endif

#ifndef DEF_MAIN
#define DEF_MAIN

#define VIEWPORT_W 800
#define VIEWPORT_H 600

extern int viewport_w;
extern int viewport_h;

#endif

#define MAX_TOUCH_POINTS 10

typedef struct TouchPoint {
	int id;
	Vector2 pos;
	bool active;
} TouchPoint;
