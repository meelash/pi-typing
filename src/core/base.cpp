#include "base.h"

void MemZero(void *p, u32 n)
{
	u8 *b = (u8 *)p;
	while (n--)
		*b++ = 0;
}

void MemCopy(void *d, const void *s, u32 n)
{
	u8 *db = (u8 *)d;
	const u8 *sb = (const u8 *)s;
	while (n--)
		*db++ = *sb++;
}

int StrLen(const char *s)
{
	int n = 0;
	while (s[n])
		n++;
	return n;
}

bool StrEq(const char *a, const char *b)
{
	while (*a && *a == *b)
		a++, b++;
	return *a == *b;
}

void StrCopy(char *d, const char *s, int cap)
{
	int i = 0;
	for (; i < cap - 1 && s[i]; i++)
		d[i] = s[i];
	d[i] = 0;
}

bool StrAppend(char *d, const char *s, int cap)
{
	int n = StrLen(d);
	while (*s) {
		if (n >= cap - 1) {
			d[n] = 0;
			return false;
		}
		d[n++] = *s++;
	}
	d[n] = 0;
	return true;
}

void StrAppendUInt(char *d, u32 v, int cap)
{
	char tmp[12];
	int i = 11;
	tmp[i] = 0;
	do {
		tmp[--i] = (char)('0' + v % 10);
		v /= 10;
	} while (v);
	StrAppend(d, tmp + i, cap);
}

bool ParseUInt(const char *s, u32 *out)
{
	if (!*s)
		return false;
	u32 v = 0;
	for (; *s; s++) {
		if (*s < '0' || *s > '9')
			return false;
		v = v * 10 + (u32)(*s - '0');
	}
	*out = v;
	return true;
}

// Quarter-wave sine table, 65 entries for 0..256 (1/1024 turn units), scaled to 1024.
static const s16 kQuarterSine[65] = {
	0,    25,   50,   75,   100,  125,  150,  175,  200,  224,  249,  273,  297,
	321,  345,  369,  392,  415,  438,  460,  483,  505,  526,  548,  569,  590,
	610,  630,  650,  669,  688,  706,  724,  742,  759,  775,  792,  807,  822,
	837,  851,  865,  878,  891,  903,  915,  926,  936,  946,  955,  964,  972,
	980,  987,  993,  999,  1004, 1009, 1013, 1016, 1019, 1021, 1023, 1024, 1024,
};

int ISin(int t)
{
	t &= 1023;
	int q = t >> 8, r = t & 255;
	// Linear interpolation between table entries (every 4 units).
	auto quarter = [](int x) {
		int i = x >> 2, f = x & 3;
		int a = kQuarterSine[i], b = kQuarterSine[i < 64 ? i + 1 : 64];
		return a + (b - a) * f / 4;
	};
	switch (q) {
	case 0: return quarter(r);
	case 1: return quarter(256 - r);
	case 2: return -quarter(r);
	default: return -quarter(256 - r);
	}
}
