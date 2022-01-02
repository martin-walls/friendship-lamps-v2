#ifndef CONF_H_
#define CONF_H_

#define THING_S 0
#define THING_M 1
#define THING_INDEX THING_S

#if THING_INDEX == 0
#define VPIN_COLOR_SEND V0
#define VPIN_COLOR_READ V1

#define VPIN_STATUS_SEND V2
#define VPIN_STATUS_READ V3

#define VPIN_EFFECT_SEND V4
#define VPIN_EFFECT_READ V5

#define VPIN_ZERGBA_READ V6
#define VPIN_APP_COLOR_LED V7

#define VPIN_SEND V8
#define VPIN_READ V9

#include "secrets/secrets_0.h"

#else
#define VPIN_COLOR_SEND V1
#define VPIN_COLOR_READ V0

#define VPIN_STATUS_SEND V3
#define VPIN_STATUS_READ V2

#define VPIN_EFFECT_SEND V5
#define VPIN_EFFECT_READ V4

#define VPIN_ZERGBA_READ V6
#define VPIN_APP_COLOR_LED V7

#define VPIN_SEND V9
#define VPIN_READ V8

#include "secrets/secrets_1.h"

#endif

#endif
