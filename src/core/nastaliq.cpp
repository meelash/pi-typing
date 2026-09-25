#include "nastaliq.h"
#include <hb.h>
#include <stdlib.h>
#include <string.h>

// stb_truetype: rasteriser only (glyph ids come from HarfBuzz).
#define STBTT_malloc(x, u) ((void)(u), malloc(x))
#define STBTT_free(x, u) ((void)(u), free(x))
#define STBTT_assert(x) ((void)0)
#define STBTT_ifloor(x) ((int)__builtin_floor(x))
#define STBTT_iceil(x) ((int)__builtin_ceil(x))
#define STBTT_sqrt(x) __builtin_sqrt(x)
#define STBTT_pow(x, y) __builtin_pow(x, y)
#define STBTT_fmod(x, y) __builtin_fmod(x, y)
#define STBTT_cos(x) __builtin_cos(x)
#define STBTT_acos(x) __builtin_acos(x)
#define STBTT_fabs(x) __builtin_fabs(x)
#define STBTT_strlen(x) strlen(x)
#define STBTT_memcpy memcpy
#define STBTT_memset memset
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-compare"
#include "stb_truetype.h"
#pragma GCC diagnostic pop

namespace nastaliq {

namespace {

// Pixel sizes of font::Size (as in scripts/gen_fonts.py). Nastaliq letters
// are small for their em size but words are tall, so the scale is a compromise.
const int kBasePx[font::SizeCount] = {22, 30, 44, 58, 120};
const float kScale = 0.9f;

// Rendered glyphs, keyed by size and glyph id.
const int kSlots = 8192, kMaxGlyphs = 4096;
const u32 kPoolBytes = 8u << 20;
u32 g_slotKey[kSlots];  // (size << 16 | gid) + 1, 0 = empty
u16 g_slotIdx[kSlots];
font::Glyph g_glyphs[kMaxGlyphs];
int g_glyphCount;
u8 *g_pool;
u32 g_poolUsed;
font::Face g_faces[font::SizeCount];

// Shaped runs.
struct Entry
{
	u32 hash, stamp;
	u8 size, rtl;
	u16 n;
	int count, width;
	u32 cps[kMaxRun];
	Glyph glyphs[kMaxRunGlyphs];
};
const int kEntries = 256;
Entry *g_entries;
u32 g_clock;

bool g_tried, g_ok;
stbtt_fontinfo g_info;
float g_stbScale[font::SizeCount];
// Nastaliq words rise far above the baseline and little below it; lowering
// them a little centres them like Latin and Naskh text on the same baseline.
int g_drop[font::SizeCount];
hb_font_t *g_hbFont[font::SizeCount];
hb_buffer_t *g_buf;
hb_language_t g_urdu;

int Px(int size) { return (int)((float)kBasePx[size] * kScale + 0.5f); }

void ResetCaches()
{
	memset(g_slotKey, 0, sizeof g_slotKey);
	g_glyphCount = 0;
	g_poolUsed = 0;
	for (int i = 0; i < kEntries; i++)
		g_entries[i].stamp = 0, g_entries[i].n = 0;
}

const font::Glyph *Render(int size, u32 gid)
{
	u32 key = ((u32)size << 16 | gid) + 1;
	u32 slot = (key * 2654435761u) % kSlots;
	while (g_slotKey[slot]) {
		if (g_slotKey[slot] == key)
			return &g_glyphs[g_slotIdx[slot]];
		slot = (slot + 1) % kSlots;
	}
	float sc = g_stbScale[size];
	int x0, y0, x1, y1, adv, lsb;
	stbtt_GetGlyphBitmapBox(&g_info, (int)gid, sc, sc, &x0, &y0, &x1, &y1);
	stbtt_GetGlyphHMetrics(&g_info, (int)gid, &adv, &lsb);
	int w = x1 > x0 ? x1 - x0 : 0, h = y1 > y0 ? y1 - y0 : 0;
	if (g_glyphCount >= kMaxGlyphs || g_poolUsed + (u32)(w * h) > kPoolBytes)
		return 0;  // caller resets and retries
	font::Glyph &g = g_glyphs[g_glyphCount];
	g.cp = gid;
	g.w = (s16)w, g.h = (s16)h;
	g.x = (s16)x0, g.y = (s16)y0;
	g.adv = (s16)((float)adv * sc + 0.5f);
	g.off = g_poolUsed;
	if (w && h)
		stbtt_MakeGlyphBitmap(&g_info, g_pool + g_poolUsed, w, h, w, sc, sc, (int)gid);
	g_poolUsed += (u32)(w * h);
	g_slotKey[slot] = key;
	g_slotIdx[slot] = (u16)g_glyphCount;
	return &g_glyphs[g_glyphCount++];
}

// Shapes into e; false if the glyph cache filled up (caller resets and retries).
bool ShapeInto(Entry &e, const u32 *cps, int n, int size, bool rtl)
{
	hb_buffer_clear_contents(g_buf);
	hb_buffer_add_utf32(g_buf, (const uint32_t *)cps, n, 0, n);
	hb_buffer_set_direction(g_buf, rtl ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
	hb_buffer_set_script(g_buf, HB_SCRIPT_ARABIC);
	hb_buffer_set_language(g_buf, g_urdu);
	hb_buffer_set_cluster_level(g_buf, HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS);
	hb_shape(g_hbFont[size], g_buf, 0, 0);
	unsigned count = 0;
	const hb_glyph_info_t *info = hb_buffer_get_glyph_infos(g_buf, &count);
	const hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(g_buf, 0);
	e.count = -1;
	if (count > (unsigned)kMaxRunGlyphs)
		return true;  // too long: not shaped
	// Only words are lowered (see g_drop); digits already sit on the baseline.
	int drop = 0;
	for (int i = 0; i < n && !drop; i++)
		if (cps[i] >= 0x0620 && cps[i] <= 0x06D5 && !(cps[i] >= 0x0660 && cps[i] <= 0x066D))
			drop = g_drop[size];
	int pen = 0;  // 26.6 fixed point
	for (unsigned i = 0; i < count; i++) {
		const font::Glyph *g = Render(size, info[i].codepoint);
		if (!g)
			return false;
		// Characters up to the next cluster (in logical order) belong to this glyph.
		u32 c = info[i].cluster, next = (u32)n;
		for (unsigned k = 0; k < count; k++)
			if (info[k].cluster > c && info[k].cluster < next)
				next = info[k].cluster;
		Glyph &o = e.glyphs[i];
		o.glyph = g;
		o.x = (s16)((pen + pos[i].x_offset + 32) >> 6);
		o.y = (s16)(((-pos[i].y_offset + 32) >> 6) + drop);
		o.logical = (s16)c;
		o.span = (u8)(next - c);
		pen += pos[i].x_advance;
	}
	e.count = (int)count;
	e.width = (pen + 32) >> 6;
	return true;
}

} // namespace

bool Init()
{
	if (g_tried)
		return g_ok;
	g_tried = true;
	const unsigned char *data = font::g_NastaliqTtf;
	if (!stbtt_InitFont(&g_info, data, stbtt_GetFontOffsetForIndex(data, 0)))
		return false;
	g_pool = (u8 *)malloc(kPoolBytes);
	g_entries = (Entry *)malloc(sizeof(Entry) * kEntries);
	if (!g_pool || !g_entries)
		return false;
	hb_blob_t *blob = hb_blob_create((const char *)data, font::g_NastaliqTtfSize, HB_MEMORY_MODE_READONLY, 0, 0);
	hb_face_t *face = hb_face_create(blob, 0);
	for (int s = 0; s < font::SizeCount; s++) {
		int px = Px(s);
		g_hbFont[s] = hb_font_create(face);
		hb_font_set_scale(g_hbFont[s], px * 64, px * 64);
		g_stbScale[s] = stbtt_ScaleForMappingEmToPixels(&g_info, (float)px);
		g_drop[s] = px * 15 / 100;
		font::Face &f = g_faces[s];
		f.glyphs = 0;
		f.count = 0;
		f.bits = g_pool;
		f.ascent = (s16)(px * 11 / 10);
		f.descent = (s16)(px * 6 / 10);
	}
	g_buf = hb_buffer_create();
	g_urdu = hb_language_from_string("ur", -1);
	ResetCaches();
	g_ok = hb_buffer_allocation_successful(g_buf);
	return g_ok;
}

int Shape(const u32 *cps, int n, font::Size size, bool rtl, const Glyph **out, int *width)
{
	if (n <= 0 || n > kMaxRun || !Init())
		return -1;
	u32 hash = 2166136261u ^ (u32)size ^ (rtl ? 0x100u : 0);
	for (int i = 0; i < n; i++)
		hash = (hash ^ cps[i]) * 16777619u;
	Entry *victim = &g_entries[0];
	for (int i = 0; i < kEntries; i++) {
		Entry &e = g_entries[i];
		if (e.stamp && e.hash == hash && e.n == n && e.size == size && e.rtl == rtl &&
		    !memcmp(e.cps, cps, (size_t)n * sizeof(u32))) {
			e.stamp = ++g_clock;
			*out = e.glyphs;
			*width = e.width;
			return e.count;
		}
		if (e.stamp < victim->stamp)
			victim = &e;
	}
	Entry &e = *victim;
	if (!ShapeInto(e, cps, n, size, rtl)) {
		ResetCaches();
		if (!ShapeInto(e, cps, n, size, rtl))
			return -1;
	}
	e.hash = hash;
	e.n = (u16)n;
	e.size = (u8)size;
	e.rtl = rtl;
	memcpy(e.cps, cps, (size_t)n * sizeof(u32));
	e.stamp = ++g_clock;
	*out = e.glyphs;
	*width = e.width;
	return e.count;
}

const font::Face &Face(font::Size size)
{
	Init();
	return g_faces[size];
}

} // namespace nastaliq
