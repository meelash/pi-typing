#include "sfx.h"

namespace sfx {

enum Wave : u8 { Sine, Square, Triangle, Noise };

struct Note
{
	u16 hz;  // 0 = rest
	u16 ms;  // 0 = end of sequence
	u8 wave;
	u8 vol;  // 0..255
};

// Note frequencies (Hz)
enum : u16 { C4 = 262, E4 = 330, G4 = 392, A4 = 440, C5 = 523, D5 = 587, E5 = 659, G5 = 784,
	     A5 = 880, C6 = 1047, E6 = 1319, G6 = 1568 };

static const Note kKey[] = {{1200, 25, Sine, 60}, {0, 0, 0, 0}};
static const Note kError[] = {{150, 110, Square, 70}, {0, 0, 0, 0}};
static const Note kCombo[] = {{C5, 60, Triangle, 150}, {E5, 60, Triangle, 150}, {G5, 60, Triangle, 150},
			      {C6, 140, Triangle, 170}, {0, 0, 0, 0}};
static const Note kStar[] = {{E6, 90, Sine, 170}, {G6, 220, Sine, 170}, {0, 0, 0, 0}};
static const Note kFanfare[] = {{G4, 110, Triangle, 170}, {C5, 110, Triangle, 170}, {E5, 110, Triangle, 170},
				{G5, 200, Triangle, 180}, {E5, 100, Triangle, 170}, {G5, 400, Triangle, 190},
				{0, 0, 0, 0}};
static const Note kPop[] = {{900, 18, Noise, 200}, {500, 60, Sine, 140}, {0, 0, 0, 0}};
static const Note kLose[] = {{A4, 140, Square, 90}, {E4, 140, Square, 90}, {C4, 300, Square, 90}, {0, 0, 0, 0}};
static const Note kMove[] = {{A5, 30, Sine, 90}, {0, 0, 0, 0}};
static const Note kSelect[] = {{C5, 50, Sine, 120}, {G5, 90, Sine, 120}, {0, 0, 0, 0}};
static const Note kLevelUp[] = {{C5, 70, Square, 80}, {D5, 70, Square, 80}, {E5, 70, Square, 80},
				{G5, 70, Square, 80}, {C6, 200, Square, 90}, {0, 0, 0, 0}};
static const Note kBadge[] = {{G5, 90, Triangle, 170}, {C6, 90, Triangle, 170}, {E6, 90, Triangle, 170},
			      {G6, 350, Sine, 190}, {0, 0, 0, 0}};

static const Note *const kSeq[SfxCount] = {kKey, kError, kCombo, kStar, kFanfare, kPop,
					  kLose, kMove, kSelect, kLevelUp, kBadge};

struct Voice
{
	const Note *note;
	u32 phase, inc;
	int left, total;  // samples
};

static const int kVoices = 6;
static Voice s_voices[kVoices];
static s16 s_sine[256];
static int s_rate = 44100;
static u32 s_noise = 12345;
static volatile bool s_enabled = true;

// Single-producer/single-consumer trigger queue.
static volatile u8 s_queue[16];
static volatile u32 s_head, s_tail;

void Init(int rate)
{
	s_rate = rate;
	for (int i = 0; i < 256; i++)
		s_sine[i] = (s16)(ISin(i * 4) * 32);  // ISin is +-1024, scale to +-32768
}

void SetEnabled(bool on) { s_enabled = on; }
bool Enabled() { return s_enabled; }

void Trigger(SfxId id)
{
	if (!s_enabled)
		return;
	u32 next = (s_head + 1) & 15;
	if (next == s_tail)
		return;  // queue full, drop
	s_queue[s_head] = (u8)id;
	s_head = next;
}

static void StartNote(Voice &v)
{
	const Note &n = *v.note;
	v.total = v.left = Max(1, (int)n.ms * s_rate / 1000);
	v.inc = (u32)(((unsigned long long)n.hz << 32) / (u32)s_rate);
}

void Render(s16 *out, int frames)
{
	while (s_tail != s_head) {
		const Note *seq = kSeq[s_queue[s_tail]];
		s_tail = (s_tail + 1) & 15;
		// Use a free voice, or steal the one closest to finishing.
		int best = 0;
		for (int i = 0; i < kVoices; i++) {
			if (!s_voices[i].note) {
				best = i;
				break;
			}
			if (s_voices[i].left < s_voices[best].left)
				best = i;
		}
		s_voices[best].note = seq;
		s_voices[best].phase = 0;
		StartNote(s_voices[best]);
	}

	for (int f = 0; f < frames; f++) {
		int mix = 0;
		for (int i = 0; i < kVoices; i++) {
			Voice &v = s_voices[i];
			if (!v.note)
				continue;
			const Note &n = *v.note;
			int s = 0;
			if (n.hz) {
				switch (n.wave) {
				case Sine: s = s_sine[v.phase >> 24]; break;
				case Square: s = (v.phase & 0x80000000u) ? 20000 : -20000; break;
				case Triangle: {
					int p = (int)(v.phase >> 16);  // 0..65535
					s = p < 32768 ? p * 2 - 32768 : (65535 - p) * 2 - 32768;
					break;
				}
				default:
					s_noise ^= s_noise << 13, s_noise ^= s_noise >> 17, s_noise ^= s_noise << 5;
					s = (int)(s_noise & 0xFFFF) - 32768;
					break;
				}
				v.phase += v.inc;
			}
			// Short attack, then linear decay to 30% over the note.
			int elapsed = v.total - v.left;
			int env = elapsed < 64 ? elapsed * 256 / 64 : 256 - (elapsed * 180 / v.total);
			mix += s * n.vol / 256 * env / 256;
			if (--v.left <= 0) {
				v.note++;
				if (v.note->ms)
					StartNote(v);
				else
					v.note = 0;
			}
		}
		mix = mix * 3 / 5;
		out[f] = (s16)Clamp(mix, -32767, 32767);
	}
}

} // namespace sfx
