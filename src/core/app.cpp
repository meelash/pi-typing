#include "app.h"
#include "palette.h"
#include "sfx.h"
#include "text.h"

static const char kSaveFile[] = "progress.txt";
static const int kMapPerRow = 7;

App::App(Platform *platform) : m_platform(platform) {}

void App::Init()
{
	m_store.Reset();
	static char buf[16384];
	int len = 0;
	if (m_platform->LoadFile(kSaveFile, buf, sizeof buf - 1, &len)) {
		buf[len] = 0;
		if (!m_store.Parse(buf, len))
			m_store.Reset();
	}
	sfx::SetEnabled(m_store.sound);
	m_rng.Seed(m_platform->Random());
	m_now = m_lastUpdate = m_platform->Millis();
	m_dirty = true;
	m_caps = false;
	m_player = 0;
	m_lang = LangEn;
	m_sel = 0;
	m_confirmDelete = false;
	m_diag = false;
	m_diagScroll = 0;
	m_toastUntil = 0;
	m_lesson = 0;
	m_attempt = 0;
	m_paused = false;
	m_gameOver = true;
	m_gameWords = false;
	m_startLevel = 1;
	for (int i = 0; i < kClouds; i++) {
		m_clouds[i].x = (float)m_rng.Below(W);
		m_clouds[i].y = (float)(140 + m_rng.Below(330));
		m_clouds[i].size = 40 + m_rng.Below(40);
	}
	Go(ScrSplash);
}

void App::Go(Screen s)
{
	m_screen = s;
	m_screenStart = m_now;
	m_dirty = true;
}

void App::Save()
{
	static char buf[16384];
	int len = m_store.Serialize(buf, sizeof buf);
	if (len > 0)
		m_platform->SaveFile(kSaveFile, buf, len);
}

void App::Toast(const char *text, Color color, u32 ms)
{
	StrCopy(m_toast, text, sizeof m_toast);
	m_toastColor = color;
	m_toastUntil = m_now + ms;
}

void App::FormatNumIn(char *out, int cap, u32 v, Lang lang)
{
	char digits[12] = "";
	StrAppendUInt(digits, v, sizeof digits);
	if (lang == LangEn) {
		StrCopy(out, digits, cap);
		return;
	}
	// Arabic-Indic digits for Arabic, Extended Arabic-Indic (Urdu) digits for Urdu.
	u32 zero = lang == LangUr ? 0x06F0 : 0x0660;
	u32 cps[12];
	int n = 0;
	for (const char *p = digits; *p; p++)
		cps[n++] = zero + (u32)(*p - '0');
	text::Utf8Encode(cps, n, out, cap);
}

const char *App::LangName(Lang lang)
{
	static const char *const kNames[LangCount] = {"English", "العربية", "اردو"};
	return kNames[lang];
}

void App::LessonLabel(int lesson, char *out, int cap) const
{
	const curriculum::LessonDef &d = curriculum::Get(m_lang, lesson);
	out[0] = 0;
	StrAppend(out, T(S_Lesson), cap);
	StrAppend(out, " ", cap);
	char num[16];
	FormatNum(num, sizeof num, (u32)lesson + 1);
	StrAppend(out, num, cap);
	StrAppend(out, " - ", cap);
	switch (d.kind) {
	case curriculum::NewKeys: {
		StrAppend(out, T(S_NewKeys), cap);
		StrAppend(out, ": ", cap);
		u32 cps[16];
		int n = text::Utf8Decode(d.keys, cps, 16);
		for (int i = 0; i < n; i++) {
			char one[8];
			text::Utf8Encode(&cps[i], 1, one, sizeof one);
			if (i)
				StrAppend(out, " ", cap);
			StrAppend(out, one, cap);
		}
		break;
	}
	case curriculum::Review: StrAppend(out, T(S_Review), cap); break;
	case curriculum::Capitals: StrAppend(out, T(S_Capitals), cap); break;
	case curriculum::Final: StrAppend(out, T(S_Final), cap); break;
	}
}

bool App::ExpectedChar(u32 *cp) const
{
	if (m_screen != ScrTyping || m_paused || m_line >= m_ex.lines)
		return false;
	*cp = m_ex.text[m_line][m_pos];
	return true;
}

// ---------------------------------------------------------------------------
// Input

void App::OnKey(const KeyEvent &e)
{
	m_dirty = true;
	// F12 opens a scrollable view of the system log (for a parent checking hardware).
	if (e.usage == KeyF12) {
		m_diag = !m_diag;
		m_diagScroll = 0;
		return;
	}
	if (m_diag) {
		const int page = 20;
		switch (e.usage) {
		case KeyUp: m_diagScroll += 1; break;
		case KeyDown: m_diagScroll = Max(0, m_diagScroll - 1); break;
		case KeyPageUp: m_diagScroll += page; break;
		case KeyPageDown: m_diagScroll = Max(0, m_diagScroll - page); break;
		case KeyHome: m_diagScroll = 1 << 20; break;
		case KeyEnd: m_diagScroll = 0; break;
		case KeyEscape: m_diag = false; break;
		}
		return;
	}
	if (e.usage == KeyCapsLock) {
		m_caps = !m_caps;
		return;
	}
	if (e.usage == KeyF1 + 1) {  // F2 toggles sound
		m_store.sound = !m_store.sound;
		sfx::SetEnabled(m_store.sound);
		Toast(T(m_store.sound ? S_SoundOn : S_SoundOff), pal::Navy);
		Save();
		return;
	}
	switch (m_screen) {
	case ScrSplash:
		if (m_now - m_screenStart > 600)
			Go(ScrProfiles);
		break;
	case ScrProfiles: KeyProfiles(e); break;
	case ScrNewProfile: KeyNewProfile(e); break;
	case ScrCourses: KeyCourses(e); break;
	case ScrMap: KeyMap(e); break;
	case ScrIntro: KeyIntro(e); break;
	case ScrTyping: KeyTyping(e); break;
	case ScrResults: KeyResults(e); break;
	case ScrGameIntro: KeyGameIntro(e); break;
	case ScrGame: KeyGame(e); break;
	case ScrBadges: KeyBadges(e); break;
	}
}

void App::Update()
{
	m_now = m_platform->Millis();
	int dt = (int)(m_now - m_lastUpdate);
	m_lastUpdate = m_now;
	dt = Clamp(dt, 0, 50);
	if (m_screen == ScrSplash && m_now - m_screenStart > 2500 && m_platform->KeyboardPresent())
		Go(ScrProfiles);
	if (m_screen == ScrGame || m_screen == ScrResults)
		UpdateGame(dt);  // also animates results confetti
	// Chime as each result star lands (timing matches DrawResults).
	if (m_screen == ScrResults && m_resStarsShown < m_resStars &&
	    m_now - m_screenStart >= (u32)(350 + m_resStarsShown * 450)) {
		m_resStarsShown++;
		sfx::Trigger(SfxStar);
	}
	m_dirty = true;  // every screen has some animation
}

static bool NameIsRtl(const char *s)
{
	u32 cps[8];
	int n = text::Utf8Decode(s, cps, 8);
	for (int i = 0; i < n; i++) {
		if (text::IsArabic(cps[i]))
			return true;
		if (text::IsLatinLetter(cps[i]))
			return false;
	}
	return false;
}

void App::KeyProfiles(const KeyEvent &e)
{
	int items = m_store.count + (m_store.count < kMaxProfiles ? 1 : 0);
	if (m_confirmDelete) {
		if (e.usage == 0x1C) {  // Y
			m_store.Remove(m_sel);
			Save();
			m_sel = Min(m_sel, m_store.count);
			sfx::Trigger(SfxSelect);
		}
		if (e.usage == 0x1C || e.usage == KeyEscape || e.usage == 0x11 /* N */)
			m_confirmDelete = false;
		return;
	}
	int old = m_sel;
	switch (e.usage) {
	case KeyLeft: m_sel = Max(0, m_sel - 1); break;
	case KeyRight: m_sel = Min(items - 1, m_sel + 1); break;
	case KeyUp: if (m_sel >= 4) m_sel -= 4; break;
	case KeyDown: m_sel = Min(items - 1, m_sel + 4); break;
	case KeyDelete:
	case KeyBackspace:
		if (m_sel < m_store.count)
			m_confirmDelete = true;
		break;
	case KeyEnter:
	case KeyPadEnter:
	case KeySpace:
		sfx::Trigger(SfxSelect);
		if (m_sel >= m_store.count) {
			m_nameLen = 0;
			m_nameLang = LangEn;
			m_nameColor = (u8)(m_store.count % pal::kAvatarColors);
			Go(ScrNewProfile);
		} else {
			m_player = m_sel;
			m_lang = (Lang)Player().lang;
			m_sel = m_lang;
			Go(ScrCourses);
		}
		return;
	}
	if (m_sel != old)
		sfx::Trigger(SfxMove);
}

void App::KeyNewProfile(const KeyEvent &e)
{
	switch (e.usage) {
	case KeyEscape:
		m_sel = m_store.count;
		Go(ScrProfiles);
		return;
	case KeyTab:
		m_nameLang = (Lang)((m_nameLang + 1) % LangCount);
		sfx::Trigger(SfxMove);
		return;
	case KeyLeft:
	case KeyRight:
		m_nameColor = (u8)((m_nameColor + (e.usage == KeyRight ? 1 : pal::kAvatarColors - 1)) %
				   pal::kAvatarColors);
		sfx::Trigger(SfxMove);
		return;
	case KeyBackspace:
		if (m_nameLen)
			m_nameLen--;
		return;
	case KeyEnter:
	case KeyPadEnter: {
		// Trim trailing spaces.
		while (m_nameLen && m_name[m_nameLen - 1] == ' ')
			m_nameLen--;
		if (!m_nameLen) {
			sfx::Trigger(SfxError);
			return;
		}
		Profile &p = m_store.players[m_store.count];
		MemZero(&p, sizeof p);
		text::Utf8Encode(m_name, m_nameLen, p.name, kMaxNameBytes);
		p.color = m_nameColor;
		p.lang = m_nameLang;
		p.nameLang = m_nameLang;
		m_player = m_store.count++;
		m_lang = m_nameLang;
		Save();
		sfx::Trigger(SfxSelect);
		m_sel = m_lang;
		Go(ScrCourses);
		return;
	}
	}
	const KeyDef *k = kbd::Find(e.usage);
	if (!k || (e.mods & (ModCtrl | ModAlt)))
		return;
	u32 out[2];
	int n = kbd::Output(*k, m_nameLang, (e.mods & ModShift) != 0, m_caps, out);
	for (int i = 0; i < n; i++) {
		u32 c = out[i];
		bool ok = text::IsLatinLetter(c) || text::IsArabicLetter(c) || text::IsUrduLetter(c) || (c >= '0' && c <= '9') ||
			  (c == ' ' && m_nameLen > 0) || c == '-';
		if (ok && m_nameLen < 16)
			m_name[m_nameLen++] = c;
	}
}

void App::KeyCourses(const KeyEvent &e)
{
	switch (e.usage) {
	case KeyLeft:
	case KeyRight: {
		int old = m_sel;
		m_sel = Clamp(m_sel + (e.usage == KeyRight ? 1 : -1), 0, LangCount - 1);
		m_lang = (Lang)m_sel;
		sfx::Trigger(m_sel != old ? SfxMove : SfxError);
		break;
	}
	case KeyEscape:
		m_sel = m_player;
		Go(ScrProfiles);
		break;
	case KeyEnter:
	case KeyPadEnter:
	case KeySpace:
		m_lang = (Lang)m_sel;
		Player().lang = m_lang;
		Save();
		sfx::Trigger(SfxSelect);
		m_sel = Min(Player().Unlocked(m_lang), curriculum::Count(m_lang)) - 1;
		Go(ScrMap);
		break;
	}
}

void App::KeyMap(const KeyEvent &e)
{
	int n = curriculum::Count(m_lang);
	int old = m_sel;
	u8 key = e.usage;
	if (Rtl() && (key == KeyLeft || key == KeyRight))
		key = key == KeyLeft ? KeyRight : KeyLeft;
	switch (key) {
	case KeyRight: m_sel = Min(m_sel + 1, n + 1); break;
	case KeyLeft: m_sel = Max(m_sel - 1, 0); break;
	case KeyDown: m_sel = m_sel >= n ? m_sel : (m_sel + kMapPerRow < n ? m_sel + kMapPerRow : n); break;
	case KeyUp:
		if (m_sel >= n)
			m_sel = Min(Player().Unlocked(m_lang), n) - 1;
		else if (m_sel >= kMapPerRow)
			m_sel -= kMapPerRow;
		break;
	case KeyEscape:
		m_sel = m_lang;
		Go(ScrCourses);
		return;
	case KeyEnter:
	case KeyPadEnter:
	case KeySpace:
		if (m_sel < n) {
			if (m_sel < Player().Unlocked(m_lang)) {
				sfx::Trigger(SfxSelect);
				StartIntro(m_sel);
			} else {
				sfx::Trigger(SfxError);
				Toast(T(S_Locked), pal::InkSoft);
			}
		} else if (m_sel == n) {
			if (Player().Completed(m_lang) > 0) {
				sfx::Trigger(SfxSelect);
				curriculum::BuildGamePool(m_lang, Player().Completed(m_lang), &m_pool);
				m_gameWords = m_gameWords && m_pool.wordCount >= 8;
				m_startLevel = Clamp(m_startLevel, 1, GameMaxStart());
				Go(ScrGameIntro);
			} else {
				sfx::Trigger(SfxError);
				Toast(T(S_GameLocked), pal::InkSoft);
			}
		} else {
			sfx::Trigger(SfxSelect);
			m_sel = 0;
			Go(ScrBadges);
		}
		return;
	}
	if (m_sel != old)
		sfx::Trigger(SfxMove);
}

void App::KeyBadges(const KeyEvent &e)
{
	int old = m_sel;
	u8 key = e.usage;
	if (Rtl() && (key == KeyLeft || key == KeyRight))
		key = key == KeyLeft ? KeyRight : KeyLeft;
	switch (key) {
	case KeyRight: m_sel = Min(m_sel + 1, B_Count - 1); break;
	case KeyLeft: m_sel = Max(m_sel - 1, 0); break;
	case KeyDown: m_sel = Min(m_sel + 5, B_Count - 1); break;
	case KeyUp: if (m_sel >= 5) m_sel -= 5; break;
	case KeyEscape:
	case KeyEnter:
		m_sel = curriculum::Count(m_lang) + 1;
		Go(ScrMap);
		return;
	}
	if (m_sel != old)
		sfx::Trigger(SfxMove);
}

// ---------------------------------------------------------------------------
// Shared widgets

void App::DrawBackground(Canvas &c) { c.VGradient(0, 0, W, H, pal::BgTop, pal::BgBottom); }

void App::DrawHeader(Canvas &c, const char *title)
{
	c.FillRect(0, 0, W, 72, pal::Navy);
	// Tall Nastaliq titles would stick out of the bar at the title size.
	static text::Layout l;
	text::LayoutUtf8(title, font::Title, Rtl(), &l);
	font::Size size = l.ascent > 44 ? font::Body : font::Title;
	c.TextCentered(title, size, W / 2, 0, 72, 0xFFFFFFFF, AlignCenter, Rtl());
	if (m_screen == ScrProfiles || m_screen == ScrNewProfile || m_screen == ScrSplash)
		return;
	const Profile &p = m_store.players[m_player];
	bool rtl = Rtl();
	// Player chip on the leading side.
	int ax = rtl ? W - 44 : 44;
	DrawAvatar(c, ax, 36, 24, p);
	DrawName(c, p, font::Body, rtl ? W - 80 : 80, 47, 0xFFFFFFFF, rtl ? AlignRight : AlignLeft);
	// Stars and points on the trailing side.
	char num[16];
	FormatNum(num, sizeof num, (u32)p.TotalStars());
	int sx = rtl ? 40 : W - 40;
	int w = text::MeasureUtf8(num, font::Body, false);
	c.FillStar(rtl ? sx : sx - w - 22, 34, 16, pal::Gold);
	c.TextCentered(num, font::Body, rtl ? sx + 22 : sx, 0, 72, 0xFFFFFFFF, rtl ? AlignLeft : AlignRight);
}

void App::DrawHint(Canvas &c, const char *hint)
{
	c.FillRect(0, H - 40, W, 40, 0x18000000 | 0x2F3E75);
	c.TextCentered(hint, font::Small, W / 2, H - 40, 40, pal::InkSoft, AlignCenter, Rtl());
}

void App::DrawArUr(Canvas &c, StrId id, font::Size size, int cx, int y, Color color)
{
	const char *ar = Str(id, LangAr), *ur = Str(id, LangUr);
	int wAr = text::MeasureUtf8(ar, size, true), wUr, gap = size >= font::Title ? 60 : 44;
	{
		text::UrduScope scope(true);
		wUr = text::MeasureUtf8(ur, size, true);
	}
	// Arabic on the right (read first), Urdu on the left.
	int x = cx - (wAr + gap + wUr) / 2;
	c.Text(ar, size, x + wUr + gap, y, color, AlignLeft, true);
	text::UrduScope scope(true);
	c.Text(ur, size, x, y, color, AlignLeft, true);
}

void App::DrawAllLangs(Canvas &c, StrId id, font::Size size, int cx, int y, Color color)
{
	const char *en = Str(id, LangEn), *ar = Str(id, LangAr), *ur = Str(id, LangUr);
	int wEn = text::MeasureUtf8(en, size, false), wAr = text::MeasureUtf8(ar, size, true), wUr;
	{
		text::UrduScope scope(true);
		wUr = text::MeasureUtf8(ur, size, true);
	}
	int gap = size >= font::Title ? 56 : 40;
	int x = cx - (wEn + wUr + wAr + 2 * gap) / 2;
	c.Text(en, size, x, y, color, AlignLeft);
	c.Text(ar, size, x + wEn + wUr + 2 * gap, y, color, AlignLeft, true);
	text::UrduScope scope(true);
	c.Text(ur, size, x + wEn + gap, y, color, AlignLeft, true);
}

void App::DrawName(Canvas &c, const Profile &p, font::Size size, int x, int y, Color color, Align align)
{
	text::UrduScope scope(p.nameLang == LangUr);
	if (p.nameLang == LangUr && size == font::Title)
		size = font::Body;  // Nastaliq names are tall
	c.Text(p.name, size, x, y, color, align, NameIsRtl(p.name));
}

void App::DrawAvatar(Canvas &c, int cx, int cy, int r, const Profile &p)
{
	c.FillCircle(cx, cy + r / 12, r, 0x40000000);
	c.FillCircle(cx, cy, r, pal::Avatar[p.color % pal::kAvatarColors]);
	u32 first[2];
	int n = text::Utf8Decode(p.name, first, 1);
	if (!n)
		return;
	first[0] = text::ToUpper(first[0]);
	char s[8];
	text::Utf8Encode(first, 1, s, sizeof s);
	font::Size size = r >= 40 ? font::Type : (r >= 26 ? font::Title : font::Body);
	text::UrduScope scope(p.nameLang == LangUr);
	c.TextCentered(s, size, cx, cy - r, 2 * r, 0xFFFFFFFF, AlignCenter, text::IsArabic(first[0]));
}

void App::DrawStars(Canvas &c, int cx, int cy, int r, int filled, int gap)
{
	for (int i = 0; i < 3; i++) {
		int x = cx + (i - 1) * gap;
		if (i < filled) {
			c.FillStar(x, cy + 1, r + 2, pal::GoldDark);
			c.FillStar(x, cy, r, pal::Gold);
		} else
			c.FillStar(x, cy, r, 0xFFD9DCE6);
	}
}

void App::DrawKeycap(Canvas &c, int cx, int cy, int size, u32 cp, Color color)
{
	int x = cx - size / 2, y = cy - size / 2;
	c.FillRoundRect(x, y + 6, size, size, size / 6, Darken(color, 70));
	c.FillRoundRect(x, y, size, size, size / 6, color);
	c.FillRoundRect(x + 6, y + 4, size - 12, size - 16, size / 8, Lighten(color, 60));
	char s[24];
	u32 show = text::ToUpper(cp);
	text::Utf8Encode(&show, 1, s, sizeof s);
	font::Size fs = size >= 110 ? font::Type : font::Title;
	if (cp == ' ') {
		StrCopy(s, T(S_SpaceKey), sizeof s);
		fs = font::Small;
	}
	c.TextCentered(s, fs, cx, y, size - 8, pal::Ink, AlignCenter, text::IsArabic(cp));
}

void App::DrawMedal(Canvas &c, int cx, int cy, int r, int b, bool earned)
{
	Color col = earned ? Rgb(kBadges[b].color) : 0xFFC9CCD8;
	Color ribbon = earned ? Darken(col, 60) : 0xFFB0B4C2;
	Point left[3] = {{cx - r / 2, cy + r / 2}, {cx - r / 8, cy + r / 2}, {cx - r * 2 / 3, cy + r * 3 / 2}};
	Point right[3] = {{cx + r / 8, cy + r / 2}, {cx + r / 2, cy + r / 2}, {cx + r * 2 / 3, cy + r * 3 / 2}};
	c.FillPolygon(left, 3, ribbon);
	c.FillPolygon(right, 3, ribbon);
	c.FillCircle(cx, cy + 4, r, 0x40000000);
	c.FillCircle(cx, cy, r, earned ? pal::Gold : 0xFFDADDE6);
	c.FillCircle(cx, cy, r * 82 / 100, col);
	const char *sym = kBadges[b].symbol;
	int w = text::MeasureUtf8(sym, font::Title, false);
	font::Size fs = font::Title;
	if (w > r * 3 / 2)
		fs = text::MeasureUtf8(sym, font::Body, false) > r * 3 / 2 ? font::Small : font::Body;
	c.TextCentered(sym, fs, cx, cy - r, 2 * r, earned ? 0xFFFFFFFF : 0xFFF4F5F8, AlignCenter);
}

void App::DrawDiagnostics(Canvas &c, int scroll, bool viewer)
{
	static const char *lines[320];
	int n = m_platform->Diagnostics(lines, ARRAY_LEN(lines));
	const int lineH = 25, perPage = viewer ? 26 : 23;
	c.FillRect(0, 0, W, H, pal::BgTop);
	c.Text(Str(S_PlugKeyboard, LangEn), font::Body, 40, 44, viewer ? pal::BgTop : pal::Accent, AlignLeft);
	if (!viewer) {
		c.Text(Str(S_PlugKeyboard, LangAr), font::Body, W - 40, 44, pal::Accent, AlignRight, true);
		text::UrduScope scope(true);
		c.Text(Str(S_PlugKeyboard, LangUr), font::Body, W / 2 + 60, 44, pal::Accent, AlignCenter, true);
	}
	int last = Max(0, n - Min(scroll, Max(0, n - perPage)));
	int first = Max(0, last - perPage);
	if (viewer)
		m_diagScroll = n - last;  // clamp for the next key press
	char head[160] = "Diagnostics - running ";
	StrAppendUInt(head, m_now / 1000, sizeof head);
	StrAppend(head, " s - lines ", sizeof head);
	StrAppendUInt(head, (u32)first + 1, sizeof head);
	StrAppend(head, "-", sizeof head);
	StrAppendUInt(head, (u32)last, sizeof head);
	StrAppend(head, " of ", sizeof head);
	StrAppendUInt(head, (u32)n, sizeof head);
	if (viewer)
		StrAppend(head, "    Up/Down/PgUp/PgDn/Home/End: scroll    F12: close", sizeof head);
	int top = viewer ? 12 : 66;
	c.FillRoundRect(24, top, W - 48, H - top - 12, 18, 0xFF1E2336);
	c.Text(head, font::Small, 44, top + 32, pal::Gold, AlignLeft);
	for (int i = first; i < last; i++)
		c.Text(lines[i], font::Small, 44, top + 32 + (i - first + 1) * lineH, 0xFFE8EAF2, AlignLeft);
}

void App::DrawToast(Canvas &c)
{
	if (m_now >= m_toastUntil)
		return;
	int remain = (int)(m_toastUntil - m_now);
	u32 alpha = (u32)Min(255, remain * 255 / 300);
	int w = text::MeasureUtf8(m_toast, font::Body, Rtl()) + 60;
	int x = (W - w) / 2, y = 88;
	c.FillRoundRect(x, y + 4, w, 56, 28, WithAlpha(0xFF000000, alpha / 5));
	c.FillRoundRect(x, y, w, 56, 28, WithAlpha(m_toastColor, alpha));
	c.TextCentered(m_toast, font::Body, W / 2, y, 56, WithAlpha(0xFFFFFFFF, alpha), AlignCenter, Rtl());
}

// ---------------------------------------------------------------------------
// Screens

bool App::Draw(Canvas &c)
{
	if (!m_dirty)
		return false;
	m_dirty = false;
	// Screens before a course is chosen show all languages; there only text
	// with Urdu-only letters (or drawn in an UrduScope) is set in Nastaliq.
	bool multilingual = m_screen == ScrSplash || m_screen == ScrProfiles || m_screen == ScrNewProfile;
	text::SetUrduContext(m_lang == LangUr && !multilingual);
	switch (m_screen) {
	case ScrSplash: DrawSplash(c); break;
	case ScrProfiles: DrawProfiles(c); break;
	case ScrNewProfile: DrawNewProfile(c); break;
	case ScrCourses: DrawCourses(c); break;
	case ScrMap: DrawMap(c); break;
	case ScrIntro: DrawIntro(c); break;
	case ScrTyping: DrawTyping(c); break;
	case ScrResults: DrawResults(c); break;
	case ScrGameIntro: DrawGameIntro(c); break;
	case ScrGame: DrawGame(c); break;
	case ScrBadges: DrawBadges(c); break;
	}
	DrawToast(c);
	text::SetUrduContext(false);
	if (m_diag)
		DrawDiagnostics(c, m_diagScroll, true);
	return true;
}

void App::DrawSplash(Canvas &c)
{
	DrawBackground(c);
	int t = (int)(m_now - m_screenStart);
	// A row of bouncing keycaps spelling the title's initials.
	static const u32 keys[] = {'t', 'y', 'p', 'e', 0x0643, 0x062A, 0x0628};
	for (int i = 0; i < ARRAY_LEN(keys); i++) {
		int bounce = ISin((t / 2 + i * 120) & 1023) * 18 / 1024;
		int x = W / 2 + (i - 3) * 120;
		bool left = i < 4;
		DrawKeycap(c, x, 220 - Abs(bounce), 100, keys[i], pal::Finger[left ? i % 4 : 8 - (i - 3)]);
	}
	if (!m_platform->KeyboardPresent() && t > 6000) {
		// Still no keyboard: show what the system found, for a parent to check.
		DrawDiagnostics(c, 0, false);
		return;
	}
	c.Text(Str(S_AppTitle, LangEn), font::Type, W / 2, 380, pal::Navy, AlignCenter);
	DrawArUr(c, S_AppTitle, font::Type, W / 2, 480, pal::Navy);
	if (!m_platform->KeyboardPresent()) {
		c.Text(Str(S_PlugKeyboard, LangEn), font::Body, W / 2, 590, pal::Accent, AlignCenter);
		DrawArUr(c, S_PlugKeyboard, font::Body, W / 2, 645, pal::Accent);
	}
}

void App::DrawProfiles(Canvas &c)
{
	DrawBackground(c);
	c.FillRect(0, 0, W, 72, pal::Navy);
	DrawAllLangs(c, S_AppTitle, font::Title, W / 2, 50, 0xFFFFFFFF);
	DrawAllLangs(c, S_WhoIsTyping, font::Body, W / 2, 140, pal::Navy);

	int items = m_store.count + (m_store.count < kMaxProfiles ? 1 : 0);
	const int cw = 250, ch = 210, gap = 30, x0 = (W - (4 * cw + 3 * gap)) / 2;
	for (int i = 0; i < items; i++) {
		int x = x0 + (i % 4) * (cw + gap), y = 180 + (i / 4) * (ch + 26);
		bool sel = i == m_sel && !m_confirmDelete;
		if (sel)
			y -= 6;
		c.FillRoundRect(x, y + 8, cw, ch, 26, pal::Shadow);
		c.FillRoundRect(x, y, cw, ch, 26, pal::Panel);
		if (sel)
			c.StrokeRoundRect(x - 2, y - 2, cw + 4, ch + 4, 28, 5, pal::Accent);
		if (i < m_store.count) {
			const Profile &p = m_store.players[i];
			DrawAvatar(c, x + cw / 2, y + 66, 46, p);
			DrawName(c, p, font::Title, x + cw / 2, y + 152, pal::Ink, AlignCenter);
			char num[16];
			FormatNumIn(num, sizeof num, (u32)p.TotalStars(), (Lang)p.lang);
			c.FillStar(x + cw / 2 - 30, y + 181, 13, pal::Gold);
			c.Text(num, font::Body, x + cw / 2 - 10, y + 192, pal::InkSoft, AlignLeft);
		} else {
			c.FillCircle(x + cw / 2, y + 70, 46, 0xFFEFF1F7);
			c.FillRect(x + cw / 2 - 22, y + 66, 44, 8, pal::Accent);
			c.FillRect(x + cw / 2 - 4, y + 48, 8, 44, pal::Accent);
			c.Text(Str(S_NewPlayer, LangEn), font::Body, x + cw / 2, y + 150, pal::Ink, AlignCenter);
			DrawArUr(c, S_NewPlayer, font::Small, x + cw / 2, y + 188, pal::Ink);
		}
	}
	if (!m_platform->StorageAvailable()) {
		c.FillRoundRect(W / 2 - 430, 560, 860, 76, 20, 0xFFFFE3E3);
		c.Text(Str(S_NoStorage, LangEn), font::Small, W / 2, 590, pal::Bad, AlignCenter);
		DrawArUr(c, S_NoStorage, font::Small, W / 2, 624, pal::Bad);
	}
	c.FillRect(0, H - 64, W, 64, 0x14000000);
	c.Text(Str(S_ProfilesHint, LangEn), font::Small, W / 2, H - 38, pal::InkSoft, AlignCenter);
	DrawArUr(c, S_ProfilesHint, font::Small, W / 2, H - 10, pal::InkSoft);

	if (m_confirmDelete && m_sel < m_store.count) {
		c.FillRect(0, 0, W, H, 0x90202848);
		int pw = 760, ph = 340, px = (W - pw) / 2, py = (H - ph) / 2;
		c.FillRoundRect(px, py, pw, ph, 30, pal::Panel);
		DrawAvatar(c, W / 2, py + 70, 40, m_store.players[m_sel]);
		DrawName(c, m_store.players[m_sel], font::Title, W / 2, py + 164, pal::Ink, AlignCenter);
		c.Text(Str(S_DeleteQuestion, LangEn), font::Body, W / 2, py + 216, pal::Bad, AlignCenter);
		DrawArUr(c, S_DeleteQuestion, font::Body, W / 2, py + 260, pal::Bad);
		DrawAllLangs(c, S_DeleteHint, font::Small, W / 2, py + 310, pal::InkSoft);
	}
}

void App::DrawNewProfile(Canvas &c)
{
	DrawBackground(c);
	c.FillRect(0, 0, W, 72, pal::Navy);
	DrawAllLangs(c, S_NewPlayer, font::Title, W / 2, 50, 0xFFFFFFFF);

	Profile preview;
	MemZero(&preview, sizeof preview);
	text::Utf8Encode(m_name, m_nameLen, preview.name, kMaxNameBytes);
	preview.color = m_nameColor;
	c.FillCircle(W / 2, 180, 62, 0xFFFFFFFF);
	if (m_nameLen)
		DrawAvatar(c, W / 2, 180, 56, preview);
	else
		c.FillCircle(W / 2, 180, 56, pal::Avatar[m_nameColor]);

	DrawAllLangs(c, S_TypeYourName, font::Title, W / 2, 305, pal::Navy);

	int bw = 720, bh = 100, bx = (W - bw) / 2, by = 330;
	c.FillRoundRect(bx, by + 6, bw, bh, 24, pal::Shadow);
	c.FillRoundRect(bx, by, bw, bh, 24, pal::Panel);
	c.StrokeRoundRect(bx, by, bw, bh, 24, 4, pal::Accent);
	bool rtl = m_nameLang != LangEn;
	text::UrduScope scope(m_nameLang == LangUr);
	static text::Layout l;
	text::LayoutText(m_name, m_nameLen, m_nameLang == LangUr ? font::Title : font::Type, rtl, &l);
	int tx = rtl ? bx + bw - 30 - l.width : bx + 30;
	c.DrawLayout(l, tx, by + 70, pal::Ink);
	if ((m_now / 500) % 2 == 0) {
		int cx = rtl ? tx - 6 : tx + l.width + 4;
		c.FillRect(cx, by + 20, 4, 62, pal::Accent);
	}
	// Language pill
	const char *langName = LangName(m_nameLang);
	int pw = 150, px = bx + bw + 20;
	c.FillRoundRect(px, by + 25, pw, 50, 25, pal::Navy);
	c.TextCentered(langName, font::Body, px + pw / 2, by + 25, 50, 0xFFFFFFFF, AlignCenter, rtl);

	// Colour swatches
	for (int i = 0; i < pal::kAvatarColors; i++) {
		int cx = W / 2 + (i - 4) * 80 + 40;
		if (i == m_nameColor)
			c.FillCircle(cx, 500, 34, pal::Navy);
		c.FillCircle(cx, 500, 27, pal::Avatar[i]);
	}
	DrawAllLangs(c, S_PickColor, font::Small, W / 2, 575, pal::InkSoft);

	c.FillRect(0, H - 64, W, 64, 0x14000000);
	c.Text(Str(S_NameHint, LangEn), font::Small, W / 2, H - 38, pal::InkSoft, AlignCenter);
	DrawArUr(c, S_NameHint, font::Small, W / 2, H - 10, pal::InkSoft);
}

void App::DrawCourses(Canvas &c)
{
	DrawBackground(c);
	DrawHeader(c, T(S_ChooseCourse));
	static const char *const kSample[LangCount] = {"Aa", "أ ب", "ا ب"};
	static const Color kTint[LangCount] = {0xFFE7F5FF, 0xFFFFF0E0, 0xFFE8F8EC};
	const int cw = 360, ch = 400, gap = 36, x0 = (W - LangCount * cw - (LangCount - 1) * gap) / 2;
	for (int i = 0; i < LangCount; i++) {
		Lang l = (Lang)i;
		bool rtl = l != LangEn;
		text::UrduScope scope(l == LangUr);
		int x = x0 + i * (cw + gap), y = 140;
		bool sel = i == m_sel;
		if (sel)
			y -= 8;
		c.FillRoundRect(x, y + 10, cw, ch, 34, pal::Shadow);
		c.FillRoundRect(x, y, cw, ch, 34, pal::Panel);
		if (sel)
			c.StrokeRoundRect(x - 3, y - 3, cw + 6, ch + 6, 36, 6, pal::Accent);
		c.FillRoundRect(x + 30, y + 30, cw - 60, 170, 26, kTint[i]);
		c.TextCentered(kSample[i], font::Huge, x + cw / 2, y + 30, 170, pal::Navy, AlignCenter, rtl);
		c.Text(LangName(l), font::Type, x + cw / 2, y + 270, pal::Ink, AlignCenter, rtl);
		const Profile &p = Player();
		int total = curriculum::Count(l), done = p.Completed(l);
		int bx = x + 50, bw = cw - 100, by = y + 310;
		c.FillRoundRect(bx, by, bw, 18, 9, 0xFFE6E8F0);
		if (done)
			c.FillRoundRect(rtl ? bx + bw - bw * done / total : bx, by, Max(18, bw * done / total), 18, 9,
					pal::Good);
		char num[32];
		FormatNumIn(num, sizeof num, (u32)p.CourseStars(l), l);
		c.FillStar(x + cw / 2 - 30, y + 360, 16, pal::Gold);
		c.Text(num, font::Body, x + cw / 2 - 6, y + 372, pal::InkSoft, AlignLeft);
	}
	DrawHint(c, T(S_CourseHint));
}

void App::DrawMap(Canvas &c)
{
	DrawBackground(c);
	DrawHeader(c, LangName(m_lang));
	const Profile &p = Player();
	int n = curriculum::Count(m_lang);
	int unlocked = p.Unlocked(m_lang);
	bool rtl = Rtl();
	auto nodeX = [&](int i) {
		int x = 175 + (i % kMapPerRow) * 155;
		return rtl ? W - x : x;
	};
	auto nodeY = [&](int i) { return 145 + (i / kMapPerRow) * 112; };

	// Paths between consecutive lessons in a row.
	for (int i = 0; i + 1 < n; i++) {
		if ((i + 1) % kMapPerRow == 0)
			continue;
		Color col = p.course[m_lang].stars[i] ? pal::Gold : 0xFFD5D9E6;
		c.FillRect(Min(nodeX(i), nodeX(i + 1)), nodeY(i) - 5, Abs(nodeX(i + 1) - nodeX(i)), 10, col);
	}
	for (int i = 0; i < n; i++) {
		const curriculum::LessonDef &d = curriculum::Get(m_lang, i);
		int x = nodeX(i), y = nodeY(i);
		int stars = p.course[m_lang].stars[i];
		bool locked = i >= unlocked;
		bool current = i == unlocked - 1 && !stars;
		int r = 36;
		if (i == m_sel)
			r = 42;
		Color fill = locked ? pal::Locked : (stars ? pal::Gold : pal::Accent);
		if (current) {
			int pulse = 6 + ISin((int)(m_now / 2) & 1023) * 5 / 1024;
			c.FillCircle(x, y, r + pulse + 4, WithAlpha(pal::Accent, 70));
		}
		if (i == m_sel)
			c.FillCircle(x, y, r + 7, pal::Navy);
		c.FillCircle(x, y + 4, r, Darken(fill, 50));
		c.FillCircle(x, y, r, fill);
		Color ink = locked ? 0xFF9AA0B5 : (stars ? pal::Ink : 0xFFFFFFFF);
		switch (d.kind) {
		case curriculum::NewKeys: {
			char label[24];
			u32 cps[8];
			int k = text::Utf8Decode(d.keys, cps, 8);
			if (k > 3) {  // digit rows: "1-5"
				u32 range[3] = {cps[0], '-', cps[k - 1]};
				text::Utf8Encode(range, 3, label, sizeof label);
			} else if (m_lang == LangUr) {
				// Separate letters: joined, Nastaliq would draw them as one tall word.
				u32 spaced[8];
				int m = 0;
				for (int i = 0; i < k; i++) {
					if (i)
						spaced[m++] = ' ';
					spaced[m++] = cps[i];
				}
				text::Utf8Encode(spaced, m, label, sizeof label);
			} else
				StrCopy(label, d.keys, sizeof label);
			font::Size ls = text::MeasureUtf8(label, font::Title, rtl) <= 2 * r - 8 ? font::Title
				      : text::MeasureUtf8(label, font::Body, rtl) <= 2 * r + 8 ? font::Body : font::Small;
			c.TextCentered(label, ls, x, y - r, 2 * r, ink, AlignCenter, rtl);
			break;
		}
		case curriculum::Review: c.FillStar(x, y + 2, 22, ink); break;
		case curriculum::Capitals: c.TextCentered("Aa", font::Title, x, y - r, 2 * r, ink, AlignCenter); break;
		case curriculum::Final: {
			Point crown[7] = {{x - 22, y + 14}, {x - 24, y - 14}, {x - 10, y},    {x, y - 20},
					  {x + 10, y},      {x + 24, y - 14}, {x + 22, y + 14}};
			c.FillPolygon(crown, 7, ink);
			break;
		}
		}
		if (!locked)
			DrawStars(c, x, y + 52, 9, stars, 22);
	}

	// Info panel for the selected lesson and the two tiles.
	int py = 590, ph = 82;
	int infoW = 700, infoX = rtl ? W - 40 - infoW : 40;
	c.FillRoundRect(infoX, py + 5, infoW, ph, 22, pal::Shadow);
	c.FillRoundRect(infoX, py, infoW, ph, 22, pal::Panel);
	int tx = rtl ? infoX + infoW - 28 : infoX + 28;
	Align al = rtl ? AlignRight : AlignLeft;
	if (m_sel < n) {
		char label[192];
		LessonLabel(m_sel, label, sizeof label);
		c.Text(label, font::Body, tx, py + 36, pal::Ink, al, rtl);
		char info[192] = "";
		if (m_sel >= unlocked)
			StrCopy(info, T(S_Locked), sizeof info);
		else if (p.course[m_lang].stars[m_sel]) {
			char num[16];
			StrAppend(info, T(S_Best2), sizeof info);
			StrAppend(info, ": ", sizeof info);
			FormatNum(num, sizeof num, p.course[m_lang].bestAcc[m_sel]);
			StrAppend(info, num, sizeof info);
			StrAppend(info, rtl ? "٪   " : "%   ", sizeof info);
			FormatNum(num, sizeof num, p.course[m_lang].bestWpm[m_sel]);
			StrAppend(info, num, sizeof info);
			StrAppend(info, " ", sizeof info);
			StrAppend(info, T(S_Wpm), sizeof info);
		} else
			StrCopy(info, T(S_PressStart), sizeof info);
		c.Text(info, font::Small, tx, py + 68, pal::InkSoft, al, rtl);
	} else {
		c.Text(T(m_sel == n ? S_BalloonGame : S_Badges), font::Body, tx, py + 36, pal::Ink, al, rtl);
		if (m_sel == n) {
			char info[192] = "";
			if (p.Completed(m_lang) == 0)
				StrCopy(info, T(S_GameLocked), sizeof info);
			else {
				char num[16];
				StrAppend(info, T(S_Best), sizeof info);
				StrAppend(info, ": ", sizeof info);
				FormatNum(num, sizeof num, p.course[m_lang].gameBest);
				StrAppend(info, num, sizeof info);
			}
			c.Text(info, font::Small, tx, py + 68, pal::InkSoft, al, rtl);
		}
	}
	for (int t = 0; t < 2; t++) {
		int tw = 230, tx0 = rtl ? 40 + (1 - t) * (tw + 20) : W - 40 - (2 - t) * (tw + 20) + 20;
		bool sel = m_sel == n + t;
		int ty = py - (sel ? 6 : 0);
		c.FillRoundRect(tx0, ty + 5, tw, ph, 22, pal::Shadow);
		c.FillRoundRect(tx0, ty, tw, ph, 22, t == 0 ? 0xFFE3F4FF : 0xFFFFF3D6);
		if (sel)
			c.StrokeRoundRect(tx0 - 2, ty - 2, tw + 4, ph + 4, 24, 5, pal::Accent);
		int ix = tx0 + 42, iy = ty + ph / 2;
		if (t == 0) {
			bool lockedGame = p.Completed(m_lang) == 0;
			c.Line(ix, iy + 18, ix + 3, iy + 34, 2, pal::InkSoft);
			c.FillEllipse(ix, iy - 4, 20, 25, lockedGame ? pal::Locked : pal::Bad);
			c.FillEllipse(ix - 7, iy - 13, 5, 7, 0x80FFFFFF);
		} else {
			int earned = 0;
			for (int b = 0; b < B_Count; b++)
				earned += p.HasBadge(b);
			DrawMedal(c, ix, iy - 8, 22, B_FirstLesson, earned > 0);
			char num[24], tot[8];
			FormatNum(num, sizeof num, (u32)earned);
			FormatNum(tot, sizeof tot, B_Count);
			StrAppend(num, "/", sizeof num);
			StrAppend(num, tot, sizeof num);
			c.Text(num, font::Small, tx0 + tw - 20, ty + ph - 12, pal::InkSoft, AlignRight);
		}
		const char *label = T(t == 0 ? S_BalloonGame : S_Badges);
		font::Size ls = text::MeasureUtf8(label, font::Body, rtl) <= tw - 90 ? font::Body : font::Small;
		c.TextCentered(label, ls, tx0 + 78 + (tw - 90) / 2, ty, ph - 14, pal::Ink, AlignCenter, rtl);
	}
	DrawHint(c, T(S_MapHint));
}

void App::DrawBadges(Canvas &c)
{
	DrawBackground(c);
	DrawHeader(c, T(S_Badges));
	const Profile &p = Player();
	bool rtl = Rtl();
	for (int b = 0; b < B_Count; b++) {
		int col = b % 5, row = b / 5;
		int cx = 180 + col * 230, cy = 170 + row * 150;
		if (rtl)
			cx = W - cx;
		if (b == m_sel)
			c.FillRoundRect(cx - 80, cy - 62, 160, 140, 24, 0xFFFFFFFF);
		DrawMedal(c, cx, cy - 8, 44, b, p.HasBadge(b));
		if (b == m_sel)
			c.StrokeRoundRect(cx - 80, cy - 62, 160, 140, 24, 4, pal::Accent);
	}
	int py = 534;
	c.FillRoundRect(140, py + 5, W - 280, 136, 24, pal::Shadow);
	c.FillRoundRect(140, py, W - 280, 136, 24, pal::Panel);
	c.TextCentered(kBadges[m_sel].name[m_lang], font::Title, W / 2, py + 6, 64, pal::Ink, AlignCenter, rtl);
	c.TextCentered(kBadges[m_sel].desc[m_lang], font::Body, W / 2, py + 70, 60, pal::InkSoft, AlignCenter, rtl);
	DrawHint(c, T(S_BadgesHint));
}
