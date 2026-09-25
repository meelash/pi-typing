// Text handling: UTF-8, Arabic contextual shaping, simplified bidi and layout.
#pragma once
#include "base.h"
#include "font.h"

namespace text {

// Arabic letters and lam-alef ligature pseudo-codepoints used by the keyboard.
enum : u32 {
	kLam = 0x0644,
	kAlef = 0x0627,
	kAlefHamzaAbove = 0x0623,
	kAlefHamzaBelow = 0x0625,
	kAlefMadda = 0x0622,
	kTatweel = 0x0640,
};

bool IsArabic(u32 cp);
bool IsArabicLetter(u32 cp);
bool IsLatinLetter(u32 cp);
u32 ToUpper(u32 cp);

// Decodes UTF-8 into codepoints, returns the count (at most max).
int Utf8Decode(const char *s, u32 *out, int max);
// Encodes codepoints as UTF-8 into out (terminated), returns bytes written.
int Utf8Encode(const u32 *cps, int n, char *out, int cap);

struct PlacedGlyph
{
	const font::Glyph *glyph;  // null for missing glyphs
	const font::Face *face;
	s16 x;         // pen x relative to the layout's left edge
	s16 logical;   // first logical index this glyph represents
	u8 span;       // number of logical characters (2 for lam-alef)
};

const int kMaxGlyphs = 160;

struct Layout
{
	PlacedGlyph glyphs[kMaxGlyphs];
	int count;
	int width;
	int ascent, descent;  // max over the fonts used
};

// Shapes and orders text for display. rtl selects the paragraph direction.
void LayoutText(const u32 *cps, int n, font::Size size, bool rtl, Layout *out);
void LayoutUtf8(const char *s, font::Size size, bool rtl, Layout *out);

// Width of a UTF-8 string in pixels.
int MeasureUtf8(const char *s, font::Size size, bool rtl);

} // namespace text
