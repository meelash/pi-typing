#include "keyboard.h"
#include "text.h"

namespace kbd {

// Arabic lam-alef keys produce two characters; they are stored as the
// ligature presentation form and expanded by Output().
enum : u16 { LamAlef = 0xFEFB, LamAlefHamzaAbove = 0xFEF7, LamAlefHamzaBelow = 0xFEF9, LamAlefMadda = 0xFEF5 };

#define K(usage, row, x4, w4, finger, en, enS, ar, arS) {usage, row, x4, w4, finger, {{en, enS}, {ar, arS}}}

const KeyDef g_Keys[] = {
	// Number row
	K(0x35, 0, 0, 4, LPinky, '`', '~', 0x0630, 0x0651),  // ذ, shadda
	K(0x1E, 0, 4, 4, LPinky, '1', '!', '1', '!'),
	K(0x1F, 0, 8, 4, LRing, '2', '@', '2', '@'),
	K(0x20, 0, 12, 4, LMiddle, '3', '#', '3', '#'),
	K(0x21, 0, 16, 4, LIndex, '4', '$', '4', '$'),
	K(0x22, 0, 20, 4, LIndex, '5', '%', '5', '%'),
	K(0x23, 0, 24, 4, RIndex, '6', '^', '6', '^'),
	K(0x24, 0, 28, 4, RIndex, '7', '&', '7', '&'),
	K(0x25, 0, 32, 4, RMiddle, '8', '*', '8', '*'),
	K(0x26, 0, 36, 4, RRing, '9', '(', '9', ')'),
	K(0x27, 0, 40, 4, RPinky, '0', ')', '0', '('),
	K(0x2D, 0, 44, 4, RPinky, '-', '_', '-', '_'),
	K(0x2E, 0, 48, 4, RPinky, '=', '+', '=', '+'),
	K(KeyBackspace, 0, 52, 8, RPinky, 0, 0, 0, 0),
	// Top row
	K(KeyTab, 1, 0, 6, LPinky, 0, 0, 0, 0),
	K(0x14, 1, 6, 4, LPinky, 'q', 'Q', 0x0636, 0x064E),   // ض, fatha
	K(0x1A, 1, 10, 4, LRing, 'w', 'W', 0x0635, 0x064B),   // ص, fathatan
	K(0x08, 1, 14, 4, LMiddle, 'e', 'E', 0x062B, 0x064F), // ث, damma
	K(0x15, 1, 18, 4, LIndex, 'r', 'R', 0x0642, 0x064C),  // ق, dammatan
	K(0x17, 1, 22, 4, LIndex, 't', 'T', 0x0641, LamAlefHamzaBelow),  // ف, لإ
	K(0x1C, 1, 26, 4, RIndex, 'y', 'Y', 0x063A, 0x0625),  // غ, إ
	K(0x18, 1, 30, 4, RIndex, 'u', 'U', 0x0639, 0x2018),  // ع
	K(0x0C, 1, 34, 4, RMiddle, 'i', 'I', 0x0647, 0x00F7), // ه, ÷
	K(0x12, 1, 38, 4, RRing, 'o', 'O', 0x062E, 0x00D7),   // خ, ×
	K(0x13, 1, 42, 4, RPinky, 'p', 'P', 0x062D, 0x061B),  // ح, ؛
	K(0x2F, 1, 46, 4, RPinky, '[', '{', 0x062C, '<'),     // ج
	K(0x30, 1, 50, 4, RPinky, ']', '}', 0x062F, '>'),     // د
	K(0x31, 1, 54, 6, RPinky, '\\', '|', '\\', '|'),
	// Home row
	K(KeyCapsLock, 2, 0, 7, LPinky, 0, 0, 0, 0),
	K(0x04, 2, 7, 4, LPinky, 'a', 'A', 0x0634, 0x0650),   // ش, kasra
	K(0x16, 2, 11, 4, LRing, 's', 'S', 0x0633, 0x064D),   // س, kasratan
	K(0x07, 2, 15, 4, LMiddle, 'd', 'D', 0x064A, ']'),    // ي
	K(0x09, 2, 19, 4, LIndex, 'f', 'F', 0x0628, '['),     // ب
	K(0x0A, 2, 23, 4, LIndex, 'g', 'G', 0x0644, LamAlefHamzaAbove),  // ل, لأ
	K(0x0B, 2, 27, 4, RIndex, 'h', 'H', 0x0627, 0x0623),  // ا, أ
	K(0x0D, 2, 31, 4, RIndex, 'j', 'J', 0x062A, 0x0640),  // ت, tatweel
	K(0x0E, 2, 35, 4, RMiddle, 'k', 'K', 0x0646, 0x060C), // ن, ،
	K(0x0F, 2, 39, 4, RRing, 'l', 'L', 0x0645, '/'),      // م
	K(0x33, 2, 43, 4, RPinky, ';', ':', 0x0643, ':'),     // ك
	K(0x34, 2, 47, 4, RPinky, '\'', '"', 0x0637, '"'),    // ط
	K(KeyEnter, 2, 51, 9, RPinky, 0, 0, 0, 0),
	// Bottom row
	K(KeyLeftShift, 3, 0, 9, LPinky, 0, 0, 0, 0),
	K(0x1D, 3, 9, 4, LPinky, 'z', 'Z', 0x0626, '~'),      // ئ
	K(0x1B, 3, 13, 4, LRing, 'x', 'X', 0x0621, 0x0652),   // ء, sukun
	K(0x06, 3, 17, 4, LMiddle, 'c', 'C', 0x0624, '}'),    // ؤ
	K(0x19, 3, 21, 4, LIndex, 'v', 'V', 0x0631, '{'),     // ر
	K(0x05, 3, 25, 4, LIndex, 'b', 'B', LamAlef, LamAlefMadda),  // لا, لآ
	K(0x11, 3, 29, 4, RIndex, 'n', 'N', 0x0649, 0x0622),  // ى, آ
	K(0x10, 3, 33, 4, RIndex, 'm', 'M', 0x0629, 0x2019),  // ة
	K(0x36, 3, 37, 4, RMiddle, ',', '<', 0x0648, ','),    // و
	K(0x37, 3, 41, 4, RRing, '.', '>', 0x0632, '.'),      // ز
	K(0x38, 3, 45, 4, RPinky, '/', '?', 0x0638, 0x061F),  // ظ, ؟
	K(KeyRightShift, 3, 49, 11, RPinky, 0, 0, 0, 0),
	// Space row
	K(KeySpace, 4, 15, 25, Thumb, ' ', ' ', ' ', ' '),
};

const int g_KeyCount = ARRAY_LEN(g_Keys);

const KeyDef *Find(u8 usage)
{
	if (usage == 0x64)  // ISO backslash key, treat like '\'
		usage = 0x31;
	for (int i = 0; i < g_KeyCount; i++)
		if (g_Keys[i].usage == usage)
			return &g_Keys[i];
	return 0;
}

static int Expand(u32 c, u32 out[2])
{
	switch (c) {
	case LamAlef: out[0] = text::kLam, out[1] = text::kAlef; return 2;
	case LamAlefHamzaAbove: out[0] = text::kLam, out[1] = text::kAlefHamzaAbove; return 2;
	case LamAlefHamzaBelow: out[0] = text::kLam, out[1] = text::kAlefHamzaBelow; return 2;
	case LamAlefMadda: out[0] = text::kLam, out[1] = text::kAlefMadda; return 2;
	default: out[0] = c; return c ? 1 : 0;
	}
}

int Output(const KeyDef &k, Lang lang, bool shift, bool capsLock, u32 out[2])
{
	u32 c = k.chars[lang][shift ? 1 : 0];
	if (lang == LangEn && capsLock && text::IsLatinLetter(c))
		c = k.chars[lang][shift ? 0 : 1];
	return Expand(c, out);
}

const KeyDef *KeyFor(u32 cp, Lang lang, bool *shift)
{
	for (int s = 0; s < 2; s++)
		for (int i = 0; i < g_KeyCount; i++) {
			u32 out[2];
			int n = Expand(g_Keys[i].chars[lang][s], out);
			// A lam-alef key is never the answer for a single letter.
			if (n == 1 && out[0] == cp) {
				*shift = s == 1;
				return &g_Keys[i];
			}
		}
	*shift = false;
	return 0;
}

const char *SpecialLabel(u8 usage)
{
	switch (usage) {
	case KeyBackspace: return "back";
	case KeyTab: return "tab";
	case KeyCapsLock: return "caps";
	case KeyEnter: return "enter";
	case KeyLeftShift:
	case KeyRightShift: return "shift";
	default: return 0;
	}
}

} // namespace kbd
