// The typing tutor application: screens, lessons, game and progress.
// Platform independent; the Pi kernel and the desktop simulator both drive it.
#pragma once
#include "base.h"
#include "curriculum.h"
#include "gfx.h"
#include "keyboard.h"
#include "profile.h"
#include "strings.h"

class Platform
{
public:
	virtual ~Platform() {}
	virtual u32 Millis() = 0;
	virtual u32 Random() = 0;
	virtual bool LoadFile(const char *name, char *buf, int cap, int *len) = 0;
	virtual bool SaveFile(const char *name, const char *buf, int len) = 0;
	virtual bool KeyboardPresent() = 0;
	virtual bool StorageAvailable() = 0;
	// Status and log lines shown when no keyboard is found (oldest first).
	virtual int Diagnostics(const char **lines, int max) { return 0; }
};

enum Screen { ScrSplash, ScrProfiles, ScrNewProfile, ScrCourses, ScrMap, ScrIntro, ScrTyping, ScrResults,
	      ScrGameIntro, ScrGame, ScrBadges };

class App
{
public:
	static const int W = 1280, H = 720;

	explicit App(Platform *platform);
	void Init();
	void OnKey(const KeyEvent &e);
	void Update();
	// Renders a frame if anything changed; returns false when nothing was drawn.
	bool Draw(Canvas &c);

	// Introspection for the simulator / tests.
	Screen CurrentScreen() const { return m_screen; }
	Lang CourseLang() const { return m_lang; }
	bool ExpectedChar(u32 *cp) const;
	bool GameTarget(u32 *cp) const;  // next character to pop the highest balloon

private:
	// --- flow
	void Go(Screen s);
	void Save();
	Profile &Player() { return m_store.players[m_player]; }
	bool Rtl() const { return m_lang == LangAr; }
	const char *T(StrId id) const { return Str(id, m_lang); }

	// --- input per screen
	void KeyProfiles(const KeyEvent &e);
	void KeyNewProfile(const KeyEvent &e);
	void KeyCourses(const KeyEvent &e);
	void KeyMap(const KeyEvent &e);
	void KeyIntro(const KeyEvent &e);
	void KeyTyping(const KeyEvent &e);
	void KeyResults(const KeyEvent &e);
	void KeyGameIntro(const KeyEvent &e);
	void KeyGame(const KeyEvent &e);
	void KeyBadges(const KeyEvent &e);

	// --- drawing per screen
	void DrawSplash(Canvas &c);
	void DrawProfiles(Canvas &c);
	void DrawNewProfile(Canvas &c);
	void DrawCourses(Canvas &c);
	void DrawMap(Canvas &c);
	void DrawIntro(Canvas &c);
	void DrawTyping(Canvas &c);
	void DrawResults(Canvas &c);
	void DrawGameIntro(Canvas &c);
	void DrawGame(Canvas &c);
	void DrawBadges(Canvas &c);

	// --- shared widgets
	void DrawBackground(Canvas &c);
	void DrawHeader(Canvas &c, const char *title);
	void DrawHint(Canvas &c, const char *hint);
	void DrawAvatar(Canvas &c, int cx, int cy, int r, const Profile &p);
	void DrawStars(Canvas &c, int cx, int cy, int r, int filled, int gap);
	void DrawKeyboard(Canvas &c, int x, int y, int unit, u8 target, u8 shiftKey, bool pulse);
	void DrawHands(Canvas &c, int cy, int activeFinger);
	void DrawHand(Canvas &c, int x, int y, bool left, int activeFinger);
	void DrawKeycap(Canvas &c, int cx, int cy, int size, u32 cp, Color color);
	void DrawMedal(Canvas &c, int cx, int cy, int r, int badge, bool earned);
	void DrawToast(Canvas &c);
	void DrawDiagnostics(Canvas &c, int scrollFromEnd, bool viewer);
	void Toast(const char *text, Color color, u32 ms = 1800);
	void FormatNum(char *out, int cap, u32 v) const { FormatNumIn(out, cap, v, m_lang); }
	static void FormatNumIn(char *out, int cap, u32 v, Lang lang);
	void LessonLabel(int lesson, char *out, int cap) const;

	// --- lessons
	void StartIntro(int lesson);
	void StartLesson();
	void FinishLesson();
	void AwardBadges(u32 before);
	int MapItemCount() const { return curriculum::Count(m_lang) + 2; }

	// --- game
	void StartGame();
	void UpdateGame(int dtMs);
	void GameType(u32 cp);
	void SpawnBalloon();
	void Burst(int x, int y, Color color, int count);
	void EndGame();

	Platform *m_platform;
	Store m_store;
	Screen m_screen;
	u32 m_now, m_screenStart, m_lastUpdate;
	bool m_dirty;
	Rng m_rng;
	bool m_caps;

	int m_player;
	Lang m_lang;
	int m_sel;          // selection on the current menu
	bool m_confirmDelete;
	bool m_diag;         // F12 diagnostics viewer open
	int m_diagScroll;    // lines scrolled up from the end of the log

	// New player form
	u32 m_name[24];
	int m_nameLen;
	Lang m_nameLang;
	u8 m_nameColor;

	// Toast message
	char m_toast[128];
	Color m_toastColor;
	u32 m_toastUntil;

	// Lesson
	int m_lesson;
	int m_attempt;
	curriculum::Exercise m_ex;
	int m_line, m_pos;
	u8 m_mark[curriculum::kMaxLines][curriculum::kMaxLineLen];
	bool m_errHere;
	bool m_started;
	bool m_paused;
	u32 m_startMs, m_pausedMs, m_pauseBegan;
	int m_typed, m_errors, m_combo, m_maxCombo, m_points;
	u32 m_errCp[16];
	u8 m_errCount[16];
	int m_errN;
	u8 m_wrongKey;
	u32 m_wrongUntil, m_comboUntil, m_shiftTipUntil;
	int m_comboShown;

	// Results
	int m_resStars, m_resAcc, m_resWpm, m_resPoints, m_resStarsShown;
	bool m_resBest;
	u32 m_newBadges;

public:
	// Game
	struct Balloon
	{
		float x, y, vy, rx, ry, sway;
		Color color;
		u32 text[8];
		int len, typed;
		bool alive;
	};
	struct Particle
	{
		float x, y, vx, vy;
		int life;
		Color color;
	};
	struct Cloud
	{
		float x, y;
		int size;
	};
	static const int kMaxBalloons = 12, kMaxParticles = 160, kClouds = 5;

private:
	curriculum::GamePool m_pool;
	Balloon m_balloons[kMaxBalloons];
	Particle m_particles[kMaxParticles];
	Cloud m_clouds[kClouds];
	bool m_gameWords, m_gameOver, m_gameRecord;
	int m_score, m_lives, m_level, m_pops, m_lock;
	int m_spawnMs;
	u32 m_levelBannerUntil;
};
