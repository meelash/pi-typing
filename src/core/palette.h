// Colours shared by all screens.
#pragma once
#include "gfx.h"
#include "keyboard.h"

namespace pal {

const Color BgTop = 0xFFFFF6E0;
const Color BgBottom = 0xFFE3F1FB;
const Color Ink = 0xFF2B3045;
const Color InkSoft = 0xFF6B7190;
const Color Faint = 0xFFB9BDCF;
const Color Panel = 0xFFFFFFFF;
const Color Navy = 0xFF2F3E75;
const Color Accent = 0xFFFF7A45;
const Color Good = 0xFF2EAD6B;
const Color Bad = 0xFFE5484D;
const Color Gold = 0xFFFFC53D;
const Color GoldDark = 0xFFE09B00;
const Color Locked = 0xFFD5D8E3;
const Color Shadow = 0x30202848;
const Color Highlight = 0xFFFFE27A;

// One colour per finger, like colour-coded typing gloves.
const Color Finger[FingerCount] = {
	0xFFF78FB3,  // left little: pink
	0xFFFFB347,  // left ring: orange
	0xFFFFE066,  // left middle: yellow
	0xFF7BD389,  // left pointer: green
	0xFFB8B5FF,  // thumbs: lavender
	0xFF6EC6FF,  // right pointer: blue
	0xFFFFE066,  // right middle: yellow
	0xFFFFB347,  // right ring: orange
	0xFFF78FB3,  // right little: pink
};

const int kAvatarColors = 8;
const Color Avatar[kAvatarColors] = {0xFFFF6B6B, 0xFFFFA94D, 0xFFFFD43B, 0xFF69DB7C,
				     0xFF4DABF7, 0xFF9775FA, 0xFFF783AC, 0xFF38D9A9};

} // namespace pal
