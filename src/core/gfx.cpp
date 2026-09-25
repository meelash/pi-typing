#include "gfx.h"

Color Mix(Color a, Color b, int t)
{
	t = Clamp(t, 0, 256);
	u32 r = 0;
	for (int s = 0; s < 32; s += 8) {
		int ca = (a >> s) & 0xFF, cb = (b >> s) & 0xFF;
		r |= (u32)((ca * (256 - t) + cb * t) >> 8) << s;
	}
	return r;
}

Color Lighten(Color c, int t) { return (c & 0xFF000000u) | (Mix(c, 0xFFFFFFFFu, t) & 0xFFFFFF); }
Color Darken(Color c, int t) { return (c & 0xFF000000u) | (Mix(c, 0xFF000000u, t) & 0xFFFFFF); }

void Canvas::SetClip(int x, int y, int w, int h)
{
	m_cx0 = Max(0, x);
	m_cy0 = Max(0, y);
	m_cx1 = Min(m_w, x + w);
	m_cy1 = Min(m_h, y + h);
}

inline void Canvas::Blend(u32 *p, Color c, u32 a)
{
	a = (a * ((c >> 24) + 1)) >> 8;
	if (a >= 255) {
		*p = c | 0xFF000000u;
		return;
	}
	if (a == 0)
		return;
	u32 d = *p;
	u32 rb = ((c & 0xFF00FF) * a + (d & 0xFF00FF) * (255 - a)) >> 8;
	u32 g = ((c & 0x00FF00) * a + (d & 0x00FF00) * (255 - a)) >> 8;
	*p = 0xFF000000u | (rb & 0xFF00FF) | (g & 0x00FF00);
}

void Canvas::BlendPixel(int x, int y, Color c, u32 cov)
{
	if (x < m_cx0 || y < m_cy0 || x >= m_cx1 || y >= m_cy1)
		return;
	Blend(&m_px[y * m_w + x], c, cov);
}

void Canvas::Clear(Color c)
{
	u32 n = (u32)(m_w * m_h);
	for (u32 i = 0; i < n; i++)
		m_px[i] = c;
}

void Canvas::FillRect(int x, int y, int w, int h, Color c)
{
	int x0 = Max(x, m_cx0), y0 = Max(y, m_cy0);
	int x1 = Min(x + w, m_cx1), y1 = Min(y + h, m_cy1);
	bool opaque = (c >> 24) == 0xFF;
	for (int yy = y0; yy < y1; yy++) {
		u32 *row = &m_px[yy * m_w];
		if (opaque)
			for (int xx = x0; xx < x1; xx++)
				row[xx] = c;
		else
			for (int xx = x0; xx < x1; xx++)
				Blend(&row[xx], c, 255);
	}
}

// Coverage (0..255) of a pixel at distance d (in 1/16 px) from an edge; inside is negative.
static inline u32 EdgeCoverage(int d16)
{
	if (d16 <= -8)
		return 255;
	if (d16 >= 8)
		return 0;
	return (u32)((8 - d16) * 255 / 16);
}

static int ISqrt(u32 v)
{
	u32 r = 0, bit = 1u << 30;
	while (bit > v)
		bit >>= 2;
	while (bit) {
		if (v >= r + bit) {
			v -= r + bit;
			r = (r >> 1) + bit;
		} else
			r >>= 1;
		bit >>= 2;
	}
	return (int)r;
}

void Canvas::FillRoundRect(int x, int y, int w, int h, int r, Color c)
{
	r = Min(r, Min(w, h) / 2);
	if (r <= 0) {
		FillRect(x, y, w, h, c);
		return;
	}
	FillRect(x, y + r, w, h - 2 * r, c);
	for (int yy = 0; yy < r; yy++) {
		// Rows in the rounded top and bottom bands.
		for (int half = 0; half < 2; half++) {
			int py = half == 0 ? y + yy : y + h - 1 - yy;
			if (py < m_cy0 || py >= m_cy1)
				continue;
			int dy16 = (r - yy) * 16 - 8;  // distance from circle centre, pixel centre
			// Straight middle part.
			FillRect(x + r, py, w - 2 * r, 1, c);
			for (int xx = 0; xx < r; xx++) {
				int dx16 = (r - xx) * 16 - 8;
				int dist16 = ISqrt((u32)(dx16 * dx16 + dy16 * dy16));
				u32 cov = EdgeCoverage(dist16 - r * 16);
				if (cov) {
					BlendPixel(x + xx, py, c, cov);
					BlendPixel(x + w - 1 - xx, py, c, cov);
				}
			}
		}
	}
}

void Canvas::StrokeRoundRect(int x, int y, int w, int h, int r, int t, Color c)
{
	// Draw as ring: outer minus inner via per-pixel coverage.
	r = Min(r, Min(w, h) / 2);
	int band = Max(r, t) + 1;  // only pixels this close to the border can be drawn
	for (int py = Max(y, m_cy0); py < Min(y + h, m_cy1); py++) {
		bool edgeRow = py < y + band || py > y + h - 1 - band;
		for (int px = Max(x, m_cx0); px < Min(x + w, m_cx1); px++) {
			if (!edgeRow && px == x + band && x + w - 1 - band > px)
				px = x + w - 1 - band;  // skip the untouched middle of the row
			// Signed distance to the rounded rectangle (in 1/16 px).
			int cx = Clamp(px, x + r, x + w - 1 - r);
			int cy = Clamp(py, y + r, y + h - 1 - r);
			int dx = (px - cx) * 16, dy = (py - cy) * 16;
			int dist = ISqrt((u32)(dx * dx + dy * dy)) - r * 16;
			if (px >= x + r && px <= x + w - 1 - r)
				dist = Max(y - py, py - (y + h - 1)) * 16 - 8;
			else if (py >= y + r && py <= y + h - 1 - r)
				dist = Max(x - px, px - (x + w - 1)) * 16 - 8;
			// Band between -t and 0.
			u32 outer = EdgeCoverage(dist);
			u32 inner = EdgeCoverage(dist + t * 16);
			u32 cov = outer > inner ? outer - inner : 0;
			if (cov)
				Blend(&m_px[py * m_w + px], c, cov);
		}
	}
}

void Canvas::FillCircle(int cx, int cy, int r, Color c) { FillEllipse(cx, cy, r, r, c); }

void Canvas::FillEllipse(int cx, int cy, int rx, int ry, Color c)
{
	if (rx <= 0 || ry <= 0)
		return;
	int r16 = rx * 16;
	for (int py = Max(cy - ry - 1, m_cy0); py <= Min(cy + ry + 1, m_cy1 - 1); py++) {
		// Work in circle space: scale y so the ellipse becomes a circle of radius rx.
		int dy16 = (py - cy) * 16 * rx / ry;
		u32 *row = &m_px[py * m_w];
		// Pixels closer than r-0.5 are fully covered; only the rim needs coverage.
		int inner2 = (r16 - 8) * (r16 - 8) - dy16 * dy16;
		int inner = inner2 > 0 ? ISqrt((u32)inner2) / 16 : -1;
		int outer2 = (r16 + 8) * (r16 + 8) - dy16 * dy16;
		if (outer2 <= 0)
			continue;
		int outer = ISqrt((u32)outer2) / 16 + 1;
		for (int px = Max(cx - outer, m_cx0); px <= Min(cx + outer, m_cx1 - 1); px++) {
			int dx = px - cx;
			if (Abs(dx) < inner) {
				// Solid span: jump to its end.
				int end = Min(cx + inner - 1, m_cx1 - 1);
				for (; px <= end; px++)
					Blend(&row[px], c, 255);
				px--;
				continue;
			}
			int dx16 = dx * 16;
			int dist = ISqrt((u32)(dx16 * dx16 + dy16 * dy16));
			u32 cov = EdgeCoverage(dist - r16);
			if (cov)
				Blend(&row[px], c, cov);
		}
	}
}

void Canvas::FillPolygon(const Point *pts, int n, Color c)
{
	int minx = pts[0].x, maxx = minx, miny = pts[0].y, maxy = miny;
	for (int i = 1; i < n; i++) {
		minx = Min(minx, pts[i].x), maxx = Max(maxx, pts[i].x);
		miny = Min(miny, pts[i].y), maxy = Max(maxy, pts[i].y);
	}
	minx = Max(minx, m_cx0), miny = Max(miny, m_cy0);
	maxx = Min(maxx, m_cx1 - 1), maxy = Min(maxy, m_cy1 - 1);
	int width = maxx - minx + 1;
	if (width <= 0 || n > 32)
		return;
	// Scanline fill with 4x4 supersampling (even-odd rule): for each of four
	// sub-rows per pixel row, find the edge crossings and add the covered
	// quarter-pixels to a per-pixel counter (0..16).
	static u8 cover[4096];
	width = Min(width, 4096);
	for (int py = miny; py <= maxy; py++) {
		MemZero(cover, (u32)width);
		for (int sy = 0; sy < 4; sy++) {
			int y4 = py * 4 + sy;
			int xs[32], nx = 0;
			for (int i = 0, j = n - 1; i < n; j = i++) {
				int yi = pts[i].y * 4, yj = pts[j].y * 4;
				if ((yi > y4) != (yj > y4)) {
					int xi = pts[i].x * 4, xj = pts[j].x * 4;
					xs[nx++] = xi + (xj - xi) * (y4 - yi) / (yj - yi);
				}
			}
			for (int a = 1; a < nx; a++)  // insertion sort, nx is tiny
				for (int b = a; b > 0 && xs[b - 1] > xs[b]; b--)
					Swap(xs[b - 1], xs[b]);
			for (int k = 0; k + 1 < nx; k += 2) {
				int x0 = Max(xs[k], minx * 4), x1 = Min(xs[k + 1], (maxx + 1) * 4);
				for (int x4 = x0; x4 < x1; x4++)
					cover[(x4 >> 2) - minx]++;
			}
		}
		u32 *row = &m_px[py * m_w];
		for (int i = 0; i < width; i++)
			if (cover[i])
				Blend(&row[minx + i], c, cover[i] >= 16 ? 255 : (u32)cover[i] * 16);
	}
}

void Canvas::FillStar(int cx, int cy, int r, Color c)
{
	Point p[10];
	for (int i = 0; i < 10; i++) {
		int rr = (i & 1) ? r * 40 / 100 : r;
		int t = i * 1024 / 10 - 256;  // start pointing up
		p[i].x = cx + rr * ICos(t) / 1024;
		p[i].y = cy + rr * ISin(t) / 1024;
	}
	FillPolygon(p, 10, c);
}

void Canvas::FillHeart(int cx, int cy, int s, Color c)
{
	int r = s * 28 / 100;
	FillCircle(cx - r * 9 / 10, cy - s / 8, r, c);
	FillCircle(cx + r * 9 / 10, cy - s / 8, r, c);
	Point tri[3] = {{cx - r * 185 / 100, cy - s / 20}, {cx + r * 185 / 100, cy - s / 20}, {cx, cy + s / 2}};
	FillPolygon(tri, 3, c);
}

void Canvas::VGradient(int x, int y, int w, int h, Color top, Color bottom)
{
	for (int i = 0; i < h; i++)
		FillRect(x, y + i, w, 1, Mix(top, bottom, h > 1 ? i * 256 / (h - 1) : 0));
}

void Canvas::Line(int x0, int y0, int x1, int y1, int t, Color c)
{
	int dx = x1 - x0, dy = y1 - y0;
	int len = ISqrt((u32)(dx * dx + dy * dy));
	if (len == 0)
		return;
	// Offset perpendicular by t/2.
	int ox = -dy * t / (2 * len), oy = dx * t / (2 * len);
	if (ox == 0 && oy == 0)
		ox = t > 0 ? (dy ? 1 : 0) : 0, oy = dy ? 0 : 1;
	Point p[4] = {{x0 + ox, y0 + oy}, {x1 + ox, y1 + oy}, {x1 - ox, y1 - oy}, {x0 - ox, y0 - oy}};
	FillPolygon(p, 4, c);
}

void Canvas::DrawGlyph(const text::PlacedGlyph &pg, int x, int y, Color c)
{
	const font::Glyph *g = pg.glyph;
	if (!g || !g->w)
		return;
	const u8 *bits = pg.face->bits + g->off;
	int gx = x + g->x, gy = y + g->y;
	int x0 = Max(gx, m_cx0), y0 = Max(gy, m_cy0);
	int x1 = Min(gx + g->w, m_cx1), y1 = Min(gy + g->h, m_cy1);
	for (int py = y0; py < y1; py++) {
		const u8 *src = bits + (py - gy) * g->w;
		u32 *dst = &m_px[py * m_w];
		for (int px = x0; px < x1; px++) {
			u32 a = src[px - gx];
			if (a)
				Blend(&dst[px], c, a);
		}
	}
}

int Canvas::DrawLayout(const text::Layout &l, int x, int y, Color c)
{
	for (int i = 0; i < l.count; i++)
		DrawGlyph(l.glyphs[i], x + l.glyphs[i].x, y, c);
	return l.width;
}

int Canvas::Text(const char *s, font::Size size, int x, int y, Color c, Align align, bool rtl)
{
	static text::Layout l;
	text::LayoutUtf8(s, size, rtl, &l);
	if (align == AlignCenter)
		x -= l.width / 2;
	else if (align == AlignRight)
		x -= l.width;
	return DrawLayout(l, x, y, c);
}

int Canvas::TextCentered(const char *s, font::Size size, int x, int top, int h, Color c, Align align,
			 bool rtl)
{
	// Centre on the Latin cap-ish height so mixed scripts line up.
	const font::Face &f = font::g_Latin[size];
	int textH = f.ascent * 3 / 4;
	int baseline = top + (h + textH) / 2;
	return Text(s, size, x, baseline, c, align, rtl);
}
