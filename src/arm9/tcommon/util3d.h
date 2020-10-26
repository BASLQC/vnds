#ifndef T_UTIL3D_H
#define T_UTIL3D_H

#include <nds.h>

#define VERTEX_SCALE_FACTOR 	0x40		//divf32(inttof32(1), inttof32(64))
#define INV_VERTEX_SCALE_FACTOR 0x40000		//inttof32(64)
#define VERTEX_SCALE(x)			((x) << 6)
#define INV_VERTEX_SCALE(x)		((x) >> 6)

class Rect;

void init3D();
void glReset();

#endif
