// Desktop simulator: runs the platform-independent core with scripted input
// and writes screenshots, so the UI can be checked without a Raspberry Pi.
//
// Usage: sim <script> <outdir> [savedir]
// Script commands (one per line):
//   key <name>          enter esc tab space backspace delete left right up down f2 caps y r n
//   shift               next "type" uses the given shift side: shift left|right
//   type <text>         types UTF-8 text using the current course layout
//   typeas <en|ar|ur> <t>  types using a specific layout
//   solve [n]           types the next n expected characters correctly (default: whole lesson)
//   typo                presses a wrong key once
//   wait <ms>           advances virtual time
//   shot <name>         writes <outdir>/<name>.png
//   expect <screen>     fails if the current screen is different
//   keyboard <on|off>   whether a keyboard is plugged in (default on)
#include "../core/app.h"
#include "../core/sfx.h"
#include "../core/text.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
#include <zlib.h>

class SimPlatform : public Platform
{
public:
	u32 now = 0;
	std::string saveDir;
	bool keyboard = true;
	u32 Millis() override { return now; }
	u32 Random() override { return 12345; }
	bool KeyboardPresent() override { return keyboard; }
	bool StorageAvailable() override { return true; }
	int Diagnostics(const char **lines, int max) override
	{
		static char buf[60][48];
		int n = Min(60, max);
		for (int i = 0; i < n; i++) {
			snprintf(buf[i], sizeof buf[i], "00:00:%02d.00 sim: log line %d", i / 10, i + 1);
			lines[i] = buf[i];
		}
		return n;
	}
	bool LoadFile(const char *name, char *buf, int cap, int *len) override
	{
		std::string p = saveDir + "/" + name;
		FILE *f = fopen(p.c_str(), "rb");
		if (!f)
			return false;
		*len = (int)fread(buf, 1, (size_t)cap, f);
		fclose(f);
		return true;
	}
	bool SaveFile(const char *name, const char *buf, int len) override
	{
		std::string p = saveDir + "/" + name;
		FILE *f = fopen(p.c_str(), "wb");
		if (!f)
			return false;
		fwrite(buf, 1, (size_t)len, f);
		fclose(f);
		return true;
	}
};

static void WritePng(const char *path, const u32 *px, int w, int h)
{
	std::vector<unsigned char> raw;
	raw.reserve((size_t)(w * 3 + 1) * h);
	for (int y = 0; y < h; y++) {
		raw.push_back(0);
		for (int x = 0; x < w; x++) {
			u32 c = px[y * w + x];
			raw.push_back((c >> 16) & 0xFF);
			raw.push_back((c >> 8) & 0xFF);
			raw.push_back(c & 0xFF);
		}
	}
	uLongf zlen = compressBound(raw.size());
	std::vector<unsigned char> z(zlen);
	compress2(z.data(), &zlen, raw.data(), raw.size(), 6);
	FILE *f = fopen(path, "wb");
	auto be32 = [&](u32 v) {
		unsigned char b[4] = {(unsigned char)(v >> 24), (unsigned char)(v >> 16), (unsigned char)(v >> 8),
				      (unsigned char)v};
		fwrite(b, 1, 4, f);
	};
	auto chunk = [&](const char *type, const unsigned char *data, u32 len) {
		be32(len);
		fwrite(type, 1, 4, f);
		if (len)
			fwrite(data, 1, len, f);
		uLong crc = crc32(0, (const Bytef *)type, 4);
		crc = crc32(crc, data, len);
		be32((u32)crc);
	};
	fwrite("\x89PNG\r\n\x1a\n", 1, 8, f);
	unsigned char ihdr[13] = {0, 0, 0, 0, 0, 0, 0, 0, 8, 2, 0, 0, 0};
	for (int i = 0; i < 4; i++) {
		ihdr[i] = (unsigned char)(w >> (24 - 8 * i));
		ihdr[4 + i] = (unsigned char)(h >> (24 - 8 * i));
	}
	chunk("IHDR", ihdr, 13);
	chunk("IDAT", z.data(), (u32)zlen);
	chunk("IEND", nullptr, 0);
	fclose(f);
}

static const struct
{
	const char *name;
	u8 usage;
} kKeyNames[] = {
	{"enter", KeyEnter}, {"esc", KeyEscape}, {"tab", KeyTab},       {"space", KeySpace},
	{"backspace", KeyBackspace}, {"delete", KeyDelete}, {"left", KeyLeft}, {"right", KeyRight},
	{"up", KeyUp},       {"down", KeyDown},  {"f2", KeyF1 + 1},     {"caps", KeyCapsLock},
	{"y", 0x1C},         {"r", 0x15},        {"n", 0x11},       {"f12", KeyF12},
	{"pgup", KeyPageUp}, {"pgdn", KeyPageDown}, {"home", KeyHome}, {"end", KeyEnd},
};

static const char *kScreenNames[] = {"splash", "profiles", "newprofile", "courses", "map",     "intro",
				     "typing", "results",  "gameintro",  "game",    "badges"};

int main(int argc, char **argv)
{
	if (argc > 1 && !strcmp(argv[1], "--dump")) {
		for (int l = 0; l < LangCount; l++)
			for (int i = 0; i < curriculum::Count((Lang)l); i++) {
				curriculum::Exercise ex;
				curriculum::Generate((Lang)l, i, 7, &ex);
				static const char *const kLang[LangCount] = {"en", "ar", "ur"};
				printf("== %s lesson %d (%s)\n", kLang[l], i + 1, curriculum::Get((Lang)l, i).keys);
				for (int k = 0; k < ex.lines; k++) {
					char buf[256];
					text::Utf8Encode(ex.text[k], ex.len[k], buf, sizeof buf);
					printf("   %s\n", buf);
				}
			}
		return 0;
	}
	if (argc < 3) {
		fprintf(stderr, "usage: sim <script> <outdir> [savedir]\n");
		return 2;
	}
	SimPlatform plat;
	plat.saveDir = argc > 3 ? argv[3] : argv[2];
	static u32 pixels[App::W * App::H];
	Canvas canvas(pixels, App::W, App::H);
	sfx::Init(44100);
	static App app(&plat);
	app.Init();

	FILE *script = fopen(argv[1], "r");
	if (!script) {
		perror(argv[1]);
		return 2;
	}
	u8 shiftMod = ModLShift;
	auto step = [&](u32 ms) {
		plat.now += ms;
		app.Update();
		app.Draw(canvas);
		static s16 audio[4096];
		sfx::Render(audio, 44100 * (int)ms / 1000 > 4096 ? 4096 : 44100 * (int)ms / 1000);
	};
	auto press = [&](u8 usage, u8 mods) {
		app.OnKey(KeyEvent{usage, mods});
		step(60);
	};
	auto typeChar = [&](u32 cp, Lang lang) {
		bool shift;
		const KeyDef *k = kbd::KeyFor(cp, lang, &shift);
		if (!k) {
			fprintf(stderr, "no key for U+%04X\n", cp);
			return;
		}
		press(k->usage, shift ? shiftMod : 0);
	};
	char line[1024];
	int lineNo = 0;
	int failures = 0;
	while (fgets(line, sizeof line, script)) {
		lineNo++;
		line[strcspn(line, "\r\n")] = 0;
		if (!line[0] || line[0] == '#')
			continue;
		char *arg = strchr(line, ' ');
		if (arg)
			*arg++ = 0;
		else
			arg = line + strlen(line);
		if (!strcmp(line, "key")) {
			bool found = false;
			for (auto &kn : kKeyNames)
				if (!strcmp(kn.name, arg)) {
					press(kn.usage, 0);
					found = true;
				}
			if (!found)
				fprintf(stderr, "%d: unknown key %s\n", lineNo, arg);
		} else if (!strcmp(line, "shift")) {
			shiftMod = !strcmp(arg, "right") ? ModRShift : ModLShift;
		} else if (!strcmp(line, "type") || !strcmp(line, "typeas")) {
			Lang lang = app.CourseLang();
			if (!strcmp(line, "typeas")) {
				lang = !strncmp(arg, "ar", 2) ? LangAr : !strncmp(arg, "ur", 2) ? LangUr : LangEn;
				arg += 3;
			}
			u32 cps[256];
			int n = text::Utf8Decode(arg, cps, 256);
			for (int i = 0; i < n; i++)
				typeChar(cps[i], lang);
		} else if (!strcmp(line, "solve")) {
			int n = *arg ? atoi(arg) : 1 << 30;
			u32 cp;
			for (int i = 0; i < n && app.ExpectedChar(&cp); i++) {
				typeChar(cp, app.CourseLang());
				plat.now += 150;  // ~ 60+ wpm pace with the 60 ms of press()
			}
		} else if (!strcmp(line, "pop")) {  // play the balloon game for n ms, popping balloons
			int ms = atoi(arg);
			for (int t = 0; t < ms; t += 300) {
				u32 cp;
				if (app.GameTarget(&cp))
					typeChar(cp, app.CourseLang());
				for (int k = 0; k < 8; k++)
					step(30);
			}
		} else if (!strcmp(line, "bench")) {  // average time to draw a frame of this screen
			timespec a, b;
			clock_gettime(CLOCK_MONOTONIC, &a);
			for (int i = 0; i < 100; i++) {
				app.Update();
				app.Draw(canvas);
			}
			clock_gettime(CLOCK_MONOTONIC, &b);
			double ms = ((b.tv_sec - a.tv_sec) * 1e3 + (b.tv_nsec - a.tv_nsec) / 1e6) / 100;
			printf("bench %s: %.2f ms/frame\n", kScreenNames[app.CurrentScreen()], ms);
		} else if (!strcmp(line, "typo")) {
			u32 cp;
			if (app.ExpectedChar(&cp))
				press(cp == 'q' ? 0x1D : 0x14, 0);  // 'q' or 'z' key
		} else if (!strcmp(line, "wait")) {
			int ms = atoi(arg);
			while (ms > 0) {
				step((u32)Min(ms, 33));
				ms -= 33;
			}
		} else if (!strcmp(line, "shot")) {
			step(16);
			std::string p = std::string(argv[2]) + "/" + arg + ".png";
			WritePng(p.c_str(), pixels, App::W, App::H);
			printf("shot %s\n", p.c_str());
		} else if (!strcmp(line, "keyboard")) {
			plat.keyboard = strcmp(arg, "off") != 0;
		} else if (!strcmp(line, "expect")) {
			const char *cur = kScreenNames[app.CurrentScreen()];
			if (strcmp(cur, arg)) {
				fprintf(stderr, "%d: expected screen %s, got %s\n", lineNo, arg, cur);
				failures++;
			}
		} else
			fprintf(stderr, "%d: unknown command %s\n", lineNo, line);
	}
	fclose(script);
	return failures ? 1 : 0;
}
