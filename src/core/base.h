// Shared basics for the platform-independent core. Builds both freestanding
// (Circle, no libc) and hosted (simulator), so it uses no standard headers.
#pragma once

// Same definitions as <circle/types.h>, so both can be included together.
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;

template <typename T> inline T Min(T a, T b) { return a < b ? a : b; }
template <typename T> inline T Max(T a, T b) { return a > b ? a : b; }
template <typename T> inline T Clamp(T v, T lo, T hi) { return v < lo ? lo : (v > hi ? hi : v); }
template <typename T> inline T Abs(T v) { return v < 0 ? -v : v; }
template <typename T> inline void Swap(T &a, T &b) { T t = a; a = b; b = t; }

#define ARRAY_LEN(a) ((int)(sizeof(a) / sizeof((a)[0])))

void MemZero(void *p, u32 n);
void MemCopy(void *d, const void *s, u32 n);
int StrLen(const char *s);
bool StrEq(const char *a, const char *b);
// Copies at most cap-1 bytes and always terminates.
void StrCopy(char *d, const char *s, int cap);
// Appends to a fixed buffer; returns false when truncated.
bool StrAppend(char *d, const char *s, int cap);
void StrAppendUInt(char *d, u32 v, int cap);
bool ParseUInt(const char *s, u32 *out);

// Small deterministic PRNG (xorshift32); seeded from hardware on the Pi.
class Rng
{
public:
	explicit Rng(u32 seed = 1) { Seed(seed); }
	void Seed(u32 seed) { m_s = seed ? seed : 0x9E3779B9u; }
	u32 Next()
	{
		m_s ^= m_s << 13;
		m_s ^= m_s >> 17;
		m_s ^= m_s << 5;
		return m_s;
	}
	// Uniform in [0, n)
	int Below(int n) { return n <= 1 ? 0 : (int)(Next() % (u32)n); }
	int Range(int lo, int hi) { return lo + Below(hi - lo + 1); }

private:
	u32 m_s;
};

// Integer sine for animation, input in 1/1024 turns, output in [-1024, 1024].
int ISin(int turn1024);
inline int ICos(int turn1024) { return ISin(turn1024 + 256); }
