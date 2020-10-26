#ifndef COMMON_H
#define COMMON_H

#include "tcommon/common.h"
#include "../common/fifo.h"
#include "vnds_types.h"
#include "aac.h"
#include "saveload.h"
#include "prefs.h"
#include "vnlog.h"

#define INTERNAL_IMAGE_TYPE GL_RGBA

#define COM_CORE     "core"
#define COM_SCRIPT   "script"
#define COM_GRAPHICS "graphics"
#define COM_SOUND    "sound"
#define COM_TEXT     "text"

#define DEFAULT_TEXT_COLOR (RGB15(31,31,31)|BIT(15))
#define ACTIVE_TEXT_COLOR  (RGB15(17,22,31)|BIT(15))

#define PNG_STREAM_LINES 16

#endif
