// Pre-rendered glyph tables produced by scripts/gen_fonts.py.
#pragma once
#include "base.h"

namespace font {

enum Size { Small, Body, Title, Type, Huge, SizeCount };

struct Glyph
{
	u32 cp;
	s16 w, h;  // bitmap size
	s16 x, y;  // bitmap top-left relative to the pen position on the baseline
	s16 adv;   // horizontal advance
	u32 off;   // offset into Face::bits
};

struct Face
{
	const Glyph *glyphs;  // sorted by cp
	u32 count;
	const u8 *bits;       // 8-bit alpha
	s16 ascent, descent;
};

extern const Face g_Latin[SizeCount];
extern const Face g_Arabic[SizeCount];
// Noto Nastaliq Urdu font file, used by nastaliq.cpp.
extern const u8 g_NastaliqTtf[];
extern const u32 g_NastaliqTtfSize;

const Glyph *Find(const Face &face, u32 cp);

} // namespace font
