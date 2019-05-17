#pragma once

#include "common.h"

#include "hid_map.h"

// see: http://3dbrew.org/wiki/CONFIG9_Registers
// see: http://3dbrew.org/wiki/EMMC_Registers
#define CART_STATE (~(*(volatile u8*)0x10000010) & 0x1)
#define SD_STATE ((*(volatile u16*)0x1000601C) & (0x1<<5))

#define HID_RAW_TX(t)	((s32)(((t) / (1 << 16)) & 0xFFF))
#define HID_RAW_TY(t)	((s32)((t) & 0xFFF))

u32 HID_ReadState(void);

// ts_raw is the raw touchscreen value obtained when pressing down
// the touchscreen at the screen coordinates [screen_x, screen_y]
// note: no point can be at the center
typedef struct {
	u32 ts_raw;
	int screen_x, screen_y;
} HID_CalibrationData;

u32 HID_ReadRawTouchState(void);
bool HID_ReadTouchState(u16 *x, u16 *y);
bool HID_SetCalibrationData(const HID_CalibrationData *calibs, int point_cnt, int screen_w, int screen_h);

typedef struct {
    u16 x;
    u16 y;
    u16 w;
    u16 h;
    u32 id; // shouldn't be zero
} TouchBox;

// abstraction for HID_ReadTouchState, also returns touchbox id (if any)
const TouchBox* TouchBoxGet(u32* id, const u16 x, const u16 y, const TouchBox* tbs, const u32 tbn);

u32 InputWait(u32 timeout_sec);
bool CheckButton(u32 button);

void ButtonToString(u32 button, char* str);
u32 StringToButton(char* str);
