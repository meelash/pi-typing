// Urdu text in Nastaliq. Nastaliq letters sit on a sloping baseline and change
// shape and position with their neighbours, so unlike Latin and Naskh they
// cannot be drawn from pre-rendered per-letter tables. This module shapes
// runs with HarfBuzz and rasterises the glyphs with stb_truetype on first use.
// Both results are cached, so a string costs nothing after its first frame.
//
// It is the only part of the core that uses third-party code and libc
// headers; the interface below is plain.
#pragma once
#include "base.h"
#include "font.h"

namespace nastaliq {

struct Glyph
{
	const font::Glyph *glyph;  // bitmap in Face(size).bits
	s16 x, y;                  // pen position relative to the run's left edge and baseline
	s16 logical;               // first character (index in the run) this glyph belongs to
	u8 span;                   // number of characters
};

const int kMaxRun = 96;  // characters in one shaped run
const int kMaxRunGlyphs = 192;

// Loads the embedded font; false if it cannot be used (text then falls back to Naskh).
bool Init();

// Shapes one directional run (visual order, left to right). Returns the glyph
// count, or -1 if the run is too long or shaping is unavailable.
int Shape(const u32 *cps, int n, font::Size size, bool rtl, const Glyph **out, int *width);

// Face holding the rendered bitmaps for a size (glyph lookups go through Shape).
const font::Face &Face(font::Size size);

} // namespace nastaliq
