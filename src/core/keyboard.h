// Physical keyboard model: key positions, home-row finger assignments and
// the character each key produces in the English (US) and Arabic (101) layouts.
// The layouts are implemented here, from raw USB HID usage codes, so the
// language can be switched per lesson without any OS keymap support.
#pragma once
#include "base.h"

enum Lang : u8 { LangEn, LangAr, LangCount };

enum Finger : u8 { LPinky, LRing, LMiddle, LIndex, Thumb, RIndex, RMiddle, RRing, RPinky, FingerCount };

// USB HID usage codes for non-character keys.
enum HidKey : u8 {
	KeyNone = 0,
	KeyEnter = 0x28,
	KeyEscape = 0x29,
	KeyBackspace = 0x2A,
	KeyTab = 0x2B,
	KeySpace = 0x2C,
	KeyCapsLock = 0x39,
	KeyF1 = 0x3A,
	KeyF12 = 0x45,
	KeyHome = 0x4A,
	KeyPageUp = 0x4B,
	KeyDelete = 0x4C,
	KeyEnd = 0x4D,
	KeyPageDown = 0x4E,
	KeyRight = 0x4F,
	KeyLeft = 0x50,
	KeyDown = 0x51,
	KeyUp = 0x52,
	KeyPadEnter = 0x58,
	// Pseudo usages for drawing the modifier keys.
	KeyLeftShift = 0xE1,
	KeyRightShift = 0xE5,
};

// Modifier bits as reported by a USB boot keyboard.
enum : u8 {
	ModLCtrl = 1 << 0,
	ModLShift = 1 << 1,
	ModLAlt = 1 << 2,
	ModLGui = 1 << 3,
	ModRCtrl = 1 << 4,
	ModRShift = 1 << 5,
	ModRAlt = 1 << 6,
	ModRGui = 1 << 7,
	ModShift = ModLShift | ModRShift,
	ModCtrl = ModLCtrl | ModRCtrl,
	ModAlt = ModLAlt | ModRAlt,
};

struct KeyEvent
{
	u8 usage;
	u8 mods;
};

struct KeyDef
{
	u8 usage;
	u8 row;     // 0 = number row .. 4 = space row
	u8 x4, w4;  // position and width in quarter key units
	u8 finger;
	u16 chars[LangCount][2];  // [lang][shifted]; 0 = none
};

namespace kbd {

extern const KeyDef g_Keys[];
extern const int g_KeyCount;

const KeyDef *Find(u8 usage);

// Characters produced by a key press (up to 2: the Arabic lam-alef key).
// capsLock applies to Latin letters only.
int Output(const KeyDef &k, Lang lang, bool shift, bool capsLock, u32 out[2]);

// Key that produces cp in the given layout; *shift tells if Shift is needed.
const KeyDef *KeyFor(u32 cp, Lang lang, bool *shift);

inline bool IsLeftHand(u8 finger) { return finger < Thumb; }

// Special display label for a key (Latin UI), or null to use the character.
const char *SpecialLabel(u8 usage);

} // namespace kbd
