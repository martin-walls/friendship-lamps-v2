#ifndef CONF_H_
#define CONF_H_

#define THING_S 0
#define THING_M 1
#define THING_INDEX THING_M

#if THING_INDEX == 0
#define VPIN_STATUS_SEND V2
#define VPIN_STATUS_READ V3

#define VPIN_SEND V8
#define VPIN_READ V9

#define VPIN_ZERGBA_READ V6
#define VPIN_APP_COLOR_LED V7

#else
#define VPIN_STATUS_SEND V3
#define VPIN_STATUS_READ V2

#define VPIN_SEND V9
#define VPIN_READ V8

#define VPIN_ZERGBA_READ V6
#define VPIN_APP_COLOR_LED V7

#endif

#include "secrets/secrets.h"

#endif
