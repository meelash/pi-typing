// Software rendering onto a 32-bit ARGB canvas (matches the Pi framebuffer).
#pragma once
#include "base.h"
#include "text.h"

typedef u32 Color;  // 0xAARRGGBB; alpha < 0xFF blends

inline Color Rgb(u32 rgb) { return 0xFF000000u | rgb; }
inline Color WithAlpha(Color c, u32 a) { return (c & 0x00FFFFFFu) | (Min<u32>(a, 255) << 24); }
Color Mix(Color a, Color b, int t256);  // t256=0 -> a, 256 -> b
Color Lighten(Color c, int t256);
Color Darken(Color c, int t256);

enum Align { AlignLeft, AlignCenter, AlignRight };

struct Point
{
	int x, y;
};

class Canvas
{
public:
	Canvas(u32 *pixels, int w, int h) : m_px(pixels), m_w(w), m_h(h) { ResetClip(); }

	int Width() const { return m_w; }
	int Height() const { return m_h; }
	u32 *Pixels() { return m_px; }

	void SetClip(int x, int y, int w, int h);
	void ResetClip() { m_cx0 = 0, m_cy0 = 0, m_cx1 = m_w, m_cy1 = m_h; }

	void Clear(Color c);
	void FillRect(int x, int y, int w, int h, Color c);
	void FillRoundRect(int x, int y, int w, int h, int r, Color c);
	void StrokeRoundRect(int x, int y, int w, int h, int r, int thickness, Color c);
	void FillCircle(int cx, int cy, int r, Color c);
	void FillEllipse(int cx, int cy, int rx, int ry, Color c);
	void FillPolygon(const Point *pts, int n, Color c);  // anti-aliased, small shapes
	void FillStar(int cx, int cy, int r, Color c);
	void FillHeart(int cx, int cy, int size, Color c);
	void VGradient(int x, int y, int w, int h, Color top, Color bottom);
	void Line(int x0, int y0, int x1, int y1, int thickness, Color c);

	// Text. y is the baseline. Returns the drawn width.
	int DrawLayout(const text::Layout &l, int x, int y, Color c);
	int Text(const char *utf8, font::Size size, int x, int y, Color c, Align align = AlignLeft,
		 bool rtl = false);
	// Text vertically centred in a box of height h starting at top.
	int TextCentered(const char *utf8, font::Size size, int x, int top, int h, Color c,
			 Align align = AlignLeft, bool rtl = false);
	void DrawGlyph(const text::PlacedGlyph &g, int x, int y, Color c);

	void BlendPixel(int x, int y, Color c, u32 coverage255);

private:
	inline void Blend(u32 *p, Color c, u32 a);

	u32 *m_px;
	int m_w, m_h;
	int m_cx0, m_cy0, m_cx1, m_cy1;
};
