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

bool IsArabic(u32 cp);        // Arabic script (Arabic and Urdu)
bool IsArabicLetter(u32 cp);  // letters of the Arabic alphabet (U+0621..U+064A)
bool IsUrduLetter(u32 cp);    // Urdu letters outside the Arabic alphabet (ٹ پ چ ڈ ...)

// When on, all Arabic-script text is drawn in Urdu Nastaliq. When off, only
// text containing Urdu-only characters is; everything else uses Naskh.
void SetUrduContext(bool on);
bool UrduContext();

// Sets the Urdu context for a scope, e.g. to draw an Urdu name on another course's screen.
struct UrduScope
{
	bool old;
	explicit UrduScope(bool on) : old(UrduContext()) { SetUrduContext(on); }
	~UrduScope() { SetUrduContext(old); }
};
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
	s16 y;         // vertical offset from the baseline (Nastaliq only)
	s16 logical;   // first logical index this glyph represents
	u8 span;       // number of logical characters (2 for lam-alef)
};

const int kMaxGlyphs = 256;

struct Layout
{
	PlacedGlyph glyphs[kMaxGlyphs];
	int count;
	int width;
	int ascent, descent;  // max over the fonts used (Nastaliq: the actual ink)
	bool nastaliq;        // contains Nastaliq text
};

// Shapes and orders text for display. rtl selects the paragraph direction.
void LayoutText(const u32 *cps, int n, font::Size size, bool rtl, Layout *out);
void LayoutUtf8(const char *s, font::Size size, bool rtl, Layout *out);

// Width of a UTF-8 string in pixels.
int MeasureUtf8(const char *s, font::Size size, bool rtl);

} // namespace text
