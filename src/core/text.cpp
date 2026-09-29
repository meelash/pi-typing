#include "text.h"
#include "nastaliq.h"

namespace font {

const Glyph *Find(const Face &face, u32 cp)
{
	int lo = 0, hi = (int)face.count - 1;
	while (lo <= hi) {
		int mid = (lo + hi) / 2;
		u32 c = face.glyphs[mid].cp;
		if (c == cp)
			return &face.glyphs[mid];
		if (c < cp)
			lo = mid + 1;
		else
			hi = mid - 1;
	}
	return 0;
}

} // namespace font

namespace text {

bool IsArabic(u32 cp)
{
	return (cp >= 0x0600 && cp <= 0x06FF) || (cp >= 0xFE70 && cp <= 0xFEFF);
}

bool IsArabicLetter(u32 cp) { return cp >= 0x0621 && cp <= 0x064A && cp != kTatweel; }

bool IsUrduLetter(u32 cp)
{
	switch (cp) {
	case 0x0679: case 0x067E: case 0x0686: case 0x0688: case 0x0691: case 0x0698:  // ٹ پ چ ڈ ڑ ژ
	case 0x06A9: case 0x06AF: case 0x06BA: case 0x06BE: case 0x06C1: case 0x06C3:  // ک گ ں ھ ہ ۃ
	case 0x06CC: case 0x06D2:                                                      // ی ے
		return true;
	default:
		return false;
	}
}

static bool g_urdu;

void SetUrduContext(bool on) { g_urdu = on; }
bool UrduContext() { return g_urdu; }

// Characters only used in Urdu text (letters, ۔ and Urdu digits).
static bool IsUrduOnly(u32 cp) { return cp >= 0x0670 && cp <= 0x06FF; }
static bool IsDigitCp(u32 cp)
{
	return (cp >= '0' && cp <= '9') || (cp >= 0x0660 && cp <= 0x0669) || (cp >= 0x06F0 && cp <= 0x06F9);
}

bool IsLatinLetter(u32 cp) { return (cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z'); }

u32 ToUpper(u32 cp) { return (cp >= 'a' && cp <= 'z') ? cp - 32 : cp; }

int Utf8Decode(const char *s, u32 *out, int max)
{
	const u8 *p = (const u8 *)s;
	int n = 0;
	while (*p && n < max) {
		u32 c = *p++;
		int extra = 0;
		if (c >= 0xF0) {
			c &= 0x07;
			extra = 3;
		} else if (c >= 0xE0) {
			c &= 0x0F;
			extra = 2;
		} else if (c >= 0xC0) {
			c &= 0x1F;
			extra = 1;
		}
		for (; extra && (*p & 0xC0) == 0x80; extra--)
			c = (c << 6) | (*p++ & 0x3F);
		out[n++] = extra ? '?' : c;
	}
	return n;
}

int Utf8Encode(const u32 *cps, int n, char *out, int cap)
{
	int o = 0;
	for (int i = 0; i < n; i++) {
		u32 c = cps[i];
		u8 buf[4];
		int len;
		if (c < 0x80) {
			buf[0] = (u8)c;
			len = 1;
		} else if (c < 0x800) {
			buf[0] = (u8)(0xC0 | (c >> 6));
			buf[1] = (u8)(0x80 | (c & 0x3F));
			len = 2;
		} else {
			buf[0] = (u8)(0xE0 | (c >> 12));
			buf[1] = (u8)(0x80 | ((c >> 6) & 0x3F));
			buf[2] = (u8)(0x80 | (c & 0x3F));
			len = 3;
		}
		if (o + len >= cap)
			break;
		for (int k = 0; k < len; k++)
			out[o++] = (char)buf[k];
	}
	if (cap > 0)
		out[o] = 0;
	return o;
}

// ---------------------------------------------------------------------------
// Arabic shaping via presentation forms B.

// First presentation form (isolated) for U+0621..U+064A, 0 if none.
// Forms follow in the order isolated, final, initial, medial.
static const u16 kFormBase[] = {
	0xFE80, 0xFE81, 0xFE83, 0xFE85, 0xFE87, 0xFE89, 0xFE8D, 0xFE8F,  // ء آ أ ؤ إ ئ ا ب
	0xFE93, 0xFE95, 0xFE99, 0xFE9D, 0xFEA1, 0xFEA5, 0xFEA9, 0xFEAB,  // ة ت ث ج ح خ د ذ
	0xFEAD, 0xFEAF, 0xFEB1, 0xFEB5, 0xFEB9, 0xFEBD, 0xFEC1, 0xFEC5,  // ر ز س ش ص ض ط ظ
	0xFEC9, 0xFECD, 0,      0,      0,      0,      0,      0,       // ع غ (0x063B..0x0640)
	0xFED1, 0xFED5, 0xFED9, 0xFEDD, 0xFEE1, 0xFEE5, 0xFEE9, 0xFEED,  // ف ق ك ل م ن ه و
	0xFEEF, 0xFEF1,                                                  // ى ي
};

enum Joining { JoinNone, JoinRight, JoinDual, JoinCausing };

static Joining JoiningOf(u32 cp)
{
	if (cp == kTatweel)
		return JoinCausing;
	if (!IsArabicLetter(cp))
		return JoinNone;
	switch (cp) {
	case 0x0621:  // ء
		return JoinNone;
	case 0x0622: case 0x0623: case 0x0624: case 0x0625: case 0x0627:  // آ أ ؤ إ ا
	case 0x0629: case 0x062F: case 0x0630: case 0x0631: case 0x0632:  // ة د ذ ر ز
	case 0x0648: case 0x0649:                                          // و ى
		return JoinRight;
	default:
		return JoinDual;
	}
}

static bool JoinsForward(u32 cp)  // can connect to the following letter
{
	Joining j = JoiningOf(cp);
	return j == JoinDual || j == JoinCausing;
}

static bool JoinsBackward(u32 cp)  // can connect to the preceding letter
{
	Joining j = JoiningOf(cp);
	return j == JoinDual || j == JoinRight || j == JoinCausing;
}

static u32 LamAlefLigature(u32 alef)
{
	switch (alef) {
	case kAlefMadda: return 0xFEF5;
	case kAlefHamzaAbove: return 0xFEF7;
	case kAlefHamzaBelow: return 0xFEF9;
	case kAlef: return 0xFEFB;
	default: return 0;
	}
}

static u32 Mirror(u32 cp)
{
	switch (cp) {
	case '(': return ')';
	case ')': return '(';
	case '[': return ']';
	case ']': return '[';
	case '{': return '}';
	case '}': return '{';
	case '<': return '>';
	case '>': return '<';
	default: return cp;
	}
}

// ---------------------------------------------------------------------------

enum Dir : u8 { DirL, DirR, DirN };

static Dir ClassOf(u32 cp)
{
	if (IsArabic(cp) && !IsDigitCp(cp))
		return DirR;
	if (cp == 0xD7 || cp == 0xF7)  // × ÷ are neutral symbols, not letters
		return DirN;
	if (IsDigitCp(cp) || IsLatinLetter(cp) || (cp >= 0xC0 && cp < 0x0600))
		return DirL;
	return DirN;
}

struct Item
{
	u32 cp;       // codepoint to render
	s16 logical;
	u8 span;
	u8 level;
	bool run;     // Nastaliq run of span characters, shaped as a unit
};

void LayoutText(const u32 *cps, int n, font::Size size, bool rtl, Layout *out)
{
	static Item items[kMaxGlyphs];
	static Dir dirs[kMaxGlyphs];
	n = Min(n, kMaxGlyphs);

	// Resolve directions: neutrals take the surrounding strong direction when
	// both sides agree, otherwise the paragraph direction. Numbers count as
	// right-to-left here (UAX #9 rule N1), so "حروف ۱۲ × ۲" keeps its order,
	// except Western digits after left-to-right text (rule W7), so "99%   57" does too.
	for (int i = 0; i < n; i++)
		dirs[i] = ClassOf(cps[i]);
	Dir para = rtl ? DirR : DirL;
	auto strong = [&](int k) {
		if (!IsDigitCp(cps[k]))
			return ClassOf(cps[k]);
		if (cps[k] > '9')
			return DirR;
		for (int j = k - 1; j >= 0; j--)
			if (ClassOf(cps[j]) != DirN && !IsDigitCp(cps[j]))
				return ClassOf(cps[j]);
		return para;
	};
	for (int i = 0; i < n; i++) {
		if (dirs[i] != DirN)
			continue;
		// A separator inside a number ("1-5", "10/13") stays part of it (rule W4).
		u32 c = cps[i];
		if ((c == '-' || c == '/' || c == '.' || c == ',' || c == ':') && i > 0 && i + 1 < n &&
		    IsDigitCp(cps[i - 1]) && IsDigitCp(cps[i + 1])) {
			dirs[i] = DirL;
			continue;
		}
		Dir before = para, after = para;
		for (int k = i - 1; k >= 0; k--)
			if (ClassOf(cps[k]) != DirN) {
				before = strong(k);
				break;
			}
		for (int k = i + 1; k < n; k++)
			if (ClassOf(cps[k]) != DirN) {
				after = strong(k);
				break;
			}
		dirs[i] = (before == after) ? before : para;
	}

	// Embedding levels: RTL paragraph = 1 with LTR runs at 2; LTR paragraph = 0 with RTL runs at 1.
	auto levelOf = [&](int i) -> u8 { return dirs[i] == DirR ? 1 : (rtl ? 2 : 0); };
	bool urdu = g_urdu;
	for (int i = 0; i < n && !urdu; i++)
		urdu = IsUrduOnly(cps[i]);

	// Shape in logical order.
	int m = 0;
	for (int i = 0; i < n; i++) {
		u32 c = cps[i];
		Item &it = items[m++];
		it.logical = (s16)i;
		it.span = 1;
		it.cp = c;
		it.level = levelOf(i);
		it.run = false;

		if (urdu && IsArabic(c)) {
			// Words and the spaces between them, at one level, go to the Nastaliq shaper together.
			int j = i + 1;
			while (j < n && levelOf(j) == it.level &&
			       (IsArabic(cps[j]) || (cps[j] == ' ' && j + 1 < n && IsArabic(cps[j + 1]) && levelOf(j + 1) == it.level)))
				j++;
			it.run = true;
			it.span = (u8)(j - i);
			i = j - 1;
			continue;
		}
		if (IsArabicLetter(c)) {
			bool joinPrev = i > 0 && JoinsForward(cps[i - 1]) && JoinsBackward(c);
			u32 lig = (c == kLam && i + 1 < n) ? LamAlefLigature(cps[i + 1]) : 0;
			if (lig) {
				it.cp = lig + (joinPrev ? 1 : 0);
				it.span = 2;
				i++;
				continue;
			}
			u32 base0 = kFormBase[c - 0x0621];
			if (base0) {
				bool joinNext = i + 1 < n && JoinsForward(c) && JoinsBackward(cps[i + 1]);
				int form = joinPrev ? (joinNext ? 3 : 1) : (joinNext ? 2 : 0);
				it.cp = base0 + (u32)form;
			}
		} else if (dirs[i] == DirR) {
			it.cp = Mirror(c);
		}
	}

	// Reorder (UAX #9 rule L2): reverse runs at each level from highest to 1.
	for (int level = 2; level >= 1; level--) {
		int i = 0;
		while (i < m) {
			if (items[i].level < level) {
				i++;
				continue;
			}
			int j = i;
			while (j < m && items[j].level >= level)
				j++;
			for (int a = i, b = j - 1; a < b; a++, b--)
				Swap(items[a], items[b]);
			i = j;
		}
	}

	const font::Face &latin = font::g_Latin[size];
	const font::Face &arabic = font::g_Arabic[size];
	out->count = 0;
	out->nastaliq = false;
	out->ascent = latin.ascent;
	out->descent = latin.descent;
	int x = 0;
	for (int i = 0; i < m; i++) {
		if (items[i].run) {
			const nastaliq::Glyph *gs;
			int w, count = nastaliq::Shape(cps + items[i].logical, items[i].span, size, items[i].level & 1, &gs, &w);
			const font::Face *nf = &nastaliq::Face(size);
			for (int k = 0; k < count && out->count < kMaxGlyphs; k++) {
				const nastaliq::Glyph &sg = gs[k];
				PlacedGlyph &pg = out->glyphs[out->count++];
				pg.glyph = sg.glyph;
				pg.face = nf;
				pg.x = (s16)(x + sg.x);
				pg.y = sg.y;
				pg.logical = (s16)(items[i].logical + sg.logical);
				pg.span = sg.span;
				if (sg.glyph->h) {
					out->ascent = Max<int>(out->ascent, -(sg.y + sg.glyph->y));
					out->descent = Max<int>(out->descent, sg.y + sg.glyph->y + sg.glyph->h);
				}
			}
			if (count >= 0) {
				out->nastaliq = true;
				x += w;
				continue;
			}
			items[i].cp = '?';  // could not shape: show a placeholder
		}
		if (out->count >= kMaxGlyphs)
			break;
		const font::Face *face = IsArabic(items[i].cp) ? &arabic : &latin;
		const font::Glyph *g = font::Find(*face, items[i].cp);
		if (!g) {
			// Try the other face (e.g. punctuation), then '?'.
			const font::Face *other = face == &latin ? &arabic : &latin;
			g = font::Find(*other, items[i].cp);
			if (g)
				face = other;
			else
				g = font::Find(latin, '?'), face = &latin;
		}
		if (face == &arabic) {
			out->ascent = Max<int>(out->ascent, arabic.ascent);
			out->descent = Max<int>(out->descent, arabic.descent);
		}
		PlacedGlyph &pg = out->glyphs[out->count++];
		pg.glyph = g;
		pg.face = face;
		pg.x = (s16)x;
		pg.y = 0;
		pg.logical = items[i].logical;
		pg.span = items[i].span;
		x += g ? g->adv : 0;
	}
	out->width = x;
}

void LayoutUtf8(const char *s, font::Size size, bool rtl, Layout *out)
{
	static u32 cps[kMaxGlyphs];
	int n = Utf8Decode(s, cps, kMaxGlyphs);
	LayoutText(cps, n, size, rtl, out);
}

int MeasureUtf8(const char *s, font::Size size, bool rtl)
{
	static Layout l;
	LayoutUtf8(s, size, rtl, &l);
	return l.width;
}

} // namespace text
