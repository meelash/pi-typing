// Lesson intro, typing and results screens, plus the on-screen keyboard and hands.
#include "app.h"
#include "palette.h"
#include "sfx.h"
#include "text.h"

static const int kKbUnit = 56;
static const int kKbX = (App::W - 15 * kKbUnit) / 2;

void App::StartIntro(int lesson)
{
	m_lesson = lesson;
	m_attempt = 0;
	Go(ScrIntro);
}

void App::StartLesson()
{
	m_attempt++;
	curriculum::Generate(m_lang, m_lesson, m_rng.Next(), &m_ex);
	m_line = m_pos = 0;
	MemZero(m_mark, sizeof m_mark);
	m_errHere = false;
	m_started = false;
	m_paused = false;
	m_pausedMs = 0;
	m_typed = m_errors = m_combo = m_maxCombo = m_points = 0;
	m_errN = 0;
	m_wrongKey = 0;
	m_wrongUntil = m_comboUntil = m_shiftTipUntil = 0;
	m_comboShown = 0;
	Go(ScrTyping);
}

void App::KeyIntro(const KeyEvent &e)
{
	if (e.usage == KeyEscape) {
		m_sel = m_lesson;
		Go(ScrMap);
	} else if (e.usage == KeyEnter || e.usage == KeyPadEnter || e.usage == KeySpace) {
		sfx::Trigger(SfxSelect);
		StartLesson();
	}
}

static bool IsComboMilestone(int c) { return c == 10 || c == 25 || (c >= 50 && c % 50 == 0); }

void App::KeyTyping(const KeyEvent &e)
{
	if (m_paused) {
		if (e.usage == KeyEnter || e.usage == KeyPadEnter || e.usage == KeySpace) {
			m_paused = false;
			m_pausedMs += m_now - m_pauseBegan;
		} else if (e.usage == KeyEscape) {
			m_paused = false;
			m_sel = m_lesson;
			Go(ScrMap);
		}
		return;
	}
	if (e.usage == KeyEscape) {
		m_paused = true;
		m_pauseBegan = m_now;
		return;
	}
	const KeyDef *k = kbd::Find(e.usage);
	if (!k || (e.mods & (ModCtrl | ModAlt)))
		return;
	bool shift = (e.mods & ModShift) != 0;
	u32 out[2];
	int n = kbd::Output(*k, m_lang, shift, m_caps, out);
	if (n == 0)
		return;  // Tab, Enter, Backspace... (mistakes are not erased: accuracy first)

	if (!m_started) {
		m_started = true;
		m_startMs = m_now;
	}
	const u32 *line = m_ex.text[m_line];
	int len = m_ex.len[m_line];
	u32 expected = line[m_pos];
	bool ok = out[0] == expected && (n == 1 || (m_pos + 1 < len && line[m_pos + 1] == out[1]));
	if (!ok) {
		m_errors++;
		m_errHere = true;
		m_combo = 0;
		m_wrongKey = e.usage;
		m_wrongUntil = m_now + 350;
		sfx::Trigger(SfxError);
		int slot = -1;
		for (int i = 0; i < m_errN; i++)
			if (m_errCp[i] == expected)
				slot = i;
		if (slot < 0 && m_errN < ARRAY_LEN(m_errCp)) {
			slot = m_errN++;
			m_errCp[slot] = expected;
			m_errCount[slot] = 0;
		}
		if (slot >= 0 && m_errCount[slot] < 255)
			m_errCount[slot]++;
		return;
	}

	// Pedagogy: capitals should use the Shift key on the opposite hand.
	bool needShift;
	const KeyDef *target = kbd::KeyFor(expected, m_lang, &needShift);
	if (needShift && target && target->finger != Thumb) {
		bool leftLetter = kbd::IsLeftHand(target->finger);
		bool usedRight = (e.mods & ModRShift) != 0, usedLeft = (e.mods & ModLShift) != 0;
		if ((leftLetter && usedLeft && !usedRight) || (!leftLetter && usedRight && !usedLeft))
			m_shiftTipUntil = m_now + 2500;
	}

	for (int i = 0; i < n; i++) {
		m_mark[m_line][m_pos] = m_errHere ? 2 : 1;
		m_errHere = false;
		m_pos++;
		m_typed++;
		m_combo++;
		m_points += 1 + Min(m_combo, 50) / 10;
	}
	m_maxCombo = Max(m_maxCombo, m_combo);
	if (IsComboMilestone(m_combo)) {
		m_comboShown = m_combo;
		m_comboUntil = m_now + 1300;
		sfx::Trigger(SfxCombo);
	} else
		sfx::Trigger(SfxKey);

	if (m_pos >= len) {
		m_line++;
		m_pos = 0;
		if (m_line >= m_ex.lines)
			FinishLesson();
	}
}

void App::AwardBadges(u32 before)
{
	Profile &p = Player();
	auto give = [&](int b, bool cond) {
		if (cond)
			p.badges |= 1u << b;
	};
	give(B_FirstLesson, p.Completed(LangEn) + p.Completed(LangAr) > 0);
	give(B_Combo50, p.bestCombo >= 50);
	give(B_Combo100, p.bestCombo >= 100);
	give(B_Stars30, p.TotalStars() >= 30);
	give(B_Bilingual, p.CourseStars(LangEn) > 0 && p.CourseStars(LangAr) > 0);
	for (int l = 0; l < LangCount; l++) {
		Lang lang = (Lang)l;
		const CourseProgress &cp = p.course[l];
		bool home = true, letters = true;
		for (int i = 0; i <= curriculum::LastHomeRowLesson(lang); i++)
			home &= cp.stars[i] > 0;
		for (int i = 0; i <= curriculum::LastLetterLesson(lang); i++)
			letters &= cp.stars[i] > 0;
		give(B_HomeRow, home);
		give(B_AllLetters, letters);
		give(B_Graduate, cp.stars[curriculum::Count(lang) - 1] > 0);
		give(B_Balloon100, cp.gameBest >= 100);
		give(B_Balloon300, cp.gameBest >= 300);
		for (int i = 0; i < curriculum::Count(lang); i++) {
			give(B_Perfect, cp.bestAcc[i] >= 100);
			give(B_Speed20, cp.bestWpm[i] >= 20);
			give(B_Speed30, cp.bestWpm[i] >= 30);
		}
	}
	m_newBadges = p.badges & ~before;
	if (m_newBadges)
		sfx::Trigger(SfxBadge);
}

void App::FinishLesson()
{
	Profile &p = Player();
	u32 ms = Max<u32>(1000, m_now - m_startMs - m_pausedMs);
	int total = m_typed + m_errors;
	m_resAcc = total ? (m_typed * 100) / total : 0;
	// Words per minute: five characters make a word.
	m_resWpm = (int)((u32)m_typed * 12000u / ms);
	const curriculum::LessonDef &def = curriculum::Get(m_lang, m_lesson);
	m_resStars = m_resAcc >= 98 && m_resWpm >= def.targetWpm ? 3 : m_resAcc >= 95 ? 2 : m_resAcc >= 90 ? 1 : 0;
	m_resPoints = m_points + m_resStars * 25;

	u32 before = p.badges;
	CourseProgress &cp = p.course[m_lang];
	m_resBest = m_resStars > cp.stars[m_lesson] && cp.stars[m_lesson] > 0;
	cp.stars[m_lesson] = (u8)Max<int>(cp.stars[m_lesson], m_resStars);
	if (m_resStars > 0) {
		cp.bestAcc[m_lesson] = (u8)Max<int>(cp.bestAcc[m_lesson], m_resAcc);
		cp.bestWpm[m_lesson] = (u8)Max<int>(cp.bestWpm[m_lesson], Min(m_resWpm, 255));
	}
	p.points += (u32)m_resPoints;
	p.keys += (u32)m_typed;
	p.bestCombo = (u16)Max<int>(p.bestCombo, m_maxCombo);
	AwardBadges(before);
	Save();

	// Confetti for a good result.
	for (int i = 0; i < kMaxParticles; i++)
		m_particles[i].life = 0;
	m_gameOver = true;
	if (m_resStars >= 2)
		for (int i = 0; i < 6; i++)
			Burst(200 + m_rng.Below(W - 400), 80 + m_rng.Below(120), pal::Avatar[m_rng.Below(8)], 22);
	m_resStarsShown = 0;
	sfx::Trigger(m_resStars ? SfxFanfare : SfxMove);
	Go(ScrResults);
}

void App::KeyResults(const KeyEvent &e)
{
	int n = curriculum::Count(m_lang);
	if (e.usage == KeyEscape) {
		m_sel = Min(Player().Unlocked(m_lang), n) - 1;
		Go(ScrMap);
	} else if (e.usage == 0x15 /* R */) {
		StartLesson();
	} else if (e.usage == KeyEnter || e.usage == KeyPadEnter || e.usage == KeySpace) {
		if (m_now - m_screenStart < 800)
			return;  // don't skip the results by accident
		if (m_resStars == 0)
			StartLesson();
		else if (m_lesson + 1 < n)
			StartIntro(m_lesson + 1);
		else {
			m_sel = n - 1;
			Go(ScrMap);
		}
	}
}

// ---------------------------------------------------------------------------
// Keyboard and hands

void App::DrawKeyboard(Canvas &c, int x0, int y0, int u, u8 target, u8 shiftKey, bool pulse)
{
	curriculum::CharSet learned, fresh;
	curriculum::Allowed(m_lang, m_lesson, &learned);
	curriculum::NewChars(m_lang, m_lesson, &fresh);
	c.FillRoundRect(x0 - 14, y0 - 14, 15 * u + 24, 5 * u + 24, 22, 0xFF3A4163);
	for (int i = 0; i < kbd::g_KeyCount; i++) {
		const KeyDef &k = kbd::g_Keys[i];
		int x = x0 + k.x4 * u / 4, y = y0 + k.row * u;
		int w = k.w4 * u / 4 - 4, h = u - 4;
		u32 base = k.chars[m_lang][0], shifted = k.chars[m_lang][1];
		u32 out[2];
		int nOut = kbd::Output(k, m_lang, false, false, out);
		bool known = k.finger == Thumb || (nOut && learned.Has(out[0])) || learned.Has(shifted) ||
			     (base >= 'a' && base <= 'z' && learned.Has(text::ToUpper(base)));
		bool isNew = nOut && (fresh.Has(out[0]) || fresh.Has(shifted));
		bool special = !base;
		Color face = special ? 0xFFD3D7E4 : (known ? Mix(pal::Finger[k.finger], 0xFFFFFFFF, 110) : 0xFFE9EBF2);
		if (isNew)
			face = pal::Finger[k.finger];
		bool isTarget = k.usage == target || k.usage == shiftKey;
		if (isTarget) {
			int glow = pulse ? 4 + ISin((int)(m_now * 2) & 1023) * 3 / 1024 : 4;
			c.FillRoundRect(x - glow - 2, y - glow - 2, w + 2 * glow + 4, h + 2 * glow + 4, 14, 0xFFFFFFFF);
			face = pal::Finger[k.finger];
		}
		if (k.usage == m_wrongKey && m_now < m_wrongUntil)
			face = pal::Bad;
		c.FillRoundRect(x, y + 3, w, h, 10, Darken(face, 60));
		c.FillRoundRect(x, y, w, h - 2, 10, face);
		Color ink = known || isTarget ? pal::Ink : 0xFFA3A8BC;
		const char *special_ = kbd::SpecialLabel(k.usage);
		if (special_) {
			c.TextCentered(special_, font::Small, x + w / 2, y, h, pal::InkSoft, AlignCenter);
			continue;
		}
		if (k.usage == KeySpace)
			continue;
		char label[8];
		u32 show = base;
		if (m_lang == LangEn && base >= 'a' && base <= 'z')
			show = text::ToUpper(base);
		text::Utf8Encode(&show, 1, label, sizeof label);
		bool arabic = text::IsArabic(show);
		bool latinLetter = base >= 'a' && base <= 'z';
		// Small shifted symbol in the corner (not for plain upper-case letters).
		u32 shiftedOut[2];
		bool shiftTaught = kbd::Output(k, m_lang, true, false, shiftedOut) && learned.Has(shiftedOut[0]) &&
				   (shiftedOut[0] != text::kLam || learned.Has(shiftedOut[1]));
		if (shifted && !latinLetter && (m_lang == LangEn ? true : shiftTaught)) {
			char sl[8];
			text::Utf8Encode(&shifted, 1, sl, sizeof sl);
			c.Text(sl, font::Small, x + 7, y + 22, ink, AlignLeft, text::IsArabic(shifted));
			c.TextCentered(label, arabic ? font::Body : font::Body, x + w / 2 + 6, y + 10, h - 10, ink,
				       AlignCenter, arabic);
		} else
			c.TextCentered(label, font::Body, x + w / 2, y, h - 4, ink, AlignCenter, arabic);
		// Home-row bumps on F and J.
		if (k.usage == 0x09 || k.usage == 0x0D)
			c.FillRoundRect(x + w / 2 - 8, y + h - 8, 16, 3, 1, ink);
	}
}

void App::DrawHand(Canvas &c, int x, int y, bool left, int active)
{
	// Stylised hand seen from above, fingers coloured like typing gloves.
	// Finger layout for the left hand, mirrored for the right hand.
	struct F
	{
		int dx, top, len;
	};
	static const F fingers[4] = {{14, 58, 72}, {46, 26, 104}, {78, 12, 118}, {110, 30, 100}};
	auto mx = [&](int dx, int w) { return left ? x + dx : x + 170 - dx - w; };
	Color palm = 0xFFE4E7F1;
	c.FillRoundRect(mx(10, 132), y + 100, 132, 110, 40, palm);
	for (int i = 0; i < 4; i++) {
		int finger = left ? i : 8 - i;
		bool on = finger == active;
		Color col = on ? pal::Finger[finger] : Mix(pal::Finger[finger], 0xFFFFFFFF, 150);
		int fx = mx(fingers[i].dx, 30);
		if (on)
			c.FillRoundRect(fx - 5, y + fingers[i].top - 5, 40, fingers[i].len + 10, 20, pal::Navy);
		c.FillRoundRect(fx, y + fingers[i].top, 30, fingers[i].len, 15, col);
	}
	// Thumb
	bool on = active == Thumb;
	Color tc = on ? pal::Finger[Thumb] : Mix(pal::Finger[Thumb], 0xFFFFFFFF, 150);
	int bx = left ? x + 120 : x + 50, tx = left ? x + 152 : x + 18;
	if (on) {
		c.Line(bx, y + 170, tx, y + 118, 40, pal::Navy);
		c.FillCircle(tx, y + 118, 20, pal::Navy);
	}
	c.Line(bx, y + 170, tx, y + 118, 30, tc);
	c.FillCircle(tx, y + 118, 15, tc);
	c.FillCircle(bx, y + 170, 15, tc);
}

void App::DrawHands(Canvas &c, int top, int active)
{
	DrawHand(c, 22, top, true, active);
	DrawHand(c, W - 22 - 170, top, false, active);
}

// ---------------------------------------------------------------------------

void App::DrawIntro(Canvas &c)
{
	DrawBackground(c);
	char title[128];
	LessonLabel(m_lesson, title, sizeof title);
	DrawHeader(c, title);
	bool rtl = Rtl();
	const curriculum::LessonDef &d = curriculum::Get(m_lang, m_lesson);
	curriculum::CharSet fresh;
	curriculum::NewChars(m_lang, m_lesson, &fresh);

	int activeFinger = -1;
	if (d.kind == curriculum::NewKeys && fresh.n) {
		// Big keycaps for each new key with the finger that presses it.
		int n = fresh.n, size = n > 3 ? 92 : 120, gap = n > 3 ? 120 : 190;
		int cycle = (int)((m_now - m_screenStart) / 1200) % n;
		// Show the keys in their keyboard order (left to right), whatever the script direction.
		int order[16];
		for (int i = 0; i < n; i++)
			order[i] = i;
		auto keyX = [&](int i) {
			bool s;
			const KeyDef *k = kbd::KeyFor(fresh.cps[i], m_lang, &s);
			return k ? k->row * 100 + k->x4 : 0;
		};
		for (int i = 0; i < n; i++)
			for (int j = i + 1; j < n; j++)
				if (keyX(order[j]) % 100 < keyX(order[i]) % 100)
					Swap(order[i], order[j]);
		for (int i = 0; i < n; i++) {
			int idx = order[i];
			int cx = W / 2 + (i * 2 - (n - 1)) * gap / 2;
			bool shift;
			const KeyDef *k = kbd::KeyFor(fresh.cps[idx], m_lang, &shift);
			int finger = k ? (int)k->finger : (int)Thumb;
			int lift = idx == cycle ? 10 : 0;
			DrawKeycap(c, cx, 170 - lift, size, fresh.cps[idx], pal::Finger[finger]);
			if (idx == cycle)
				activeFinger = finger;
			c.Text(T((StrId)(S_FingerLPinky + finger)), font::Small, cx, 262, pal::InkSoft, AlignCenter, rtl);
		}
	} else {
		StrId msg = d.kind == curriculum::Review ? S_ReviewIntro
			    : d.kind == curriculum::Capitals ? S_CapitalsIntro : S_FinalIntro;
		c.FillRoundRect(140, 110, W - 280, 150, 30, pal::Panel);
		c.TextCentered(T(msg), font::Title, W / 2, 110, 150, pal::Navy, AlignCenter, rtl);
	}
	int kbY = 305;
	DrawKeyboard(c, kKbX, kbY, kKbUnit, 0, 0, true);
	DrawHands(c, kbY + 30, activeFinger);
	StrId tip = m_lesson == 0 ? S_HomeRowTip : (StrId)(S_Tip0 + (m_lesson % kTipCount));
	c.TextCentered(T(tip), font::Body, W / 2, 596, 50, pal::InkSoft, AlignCenter, rtl);
	int pulse = ISin((int)(m_now / 2) & 1023) * 20 / 1024;
	c.FillRoundRect(W / 2 - 170, 642 - pulse / 10, 340, 52, 26, pal::Accent);
	c.TextCentered(T(S_PressStart), font::Body, W / 2, 642 - pulse / 10, 52, 0xFFFFFFFF, AlignCenter, rtl);
}

void App::DrawTyping(Canvas &c)
{
	DrawBackground(c);
	bool rtl = Rtl();
	char title[128];
	LessonLabel(m_lesson, title, sizeof title);
	c.FillRect(0, 0, W, 72, pal::Navy);
	c.TextCentered(title, font::Body, rtl ? W - 30 : 30, 0, 72, 0xFFFFFFFF, rtl ? AlignRight : AlignLeft, rtl);
	// Combo and points on the trailing side.
	char num[16], buf[64];
	FormatNum(num, sizeof num, (u32)m_points);
	int px = rtl ? 30 : W - 30;
	c.TextCentered(num, font::Title, px, 0, 72, pal::Gold, rtl ? AlignLeft : AlignRight);
	int pw = text::MeasureUtf8(num, font::Title, false);
	c.FillStar(rtl ? px + pw + 24 : px - pw - 24, 35, 15, pal::Gold);
	if (m_combo >= 5) {
		buf[0] = 0;
		StrAppend(buf, T(S_Combo), sizeof buf);
		StrAppend(buf, " ", sizeof buf);
		FormatNum(num, sizeof num, (u32)m_combo);
		StrAppend(buf, num, sizeof buf);
		Color cc = m_combo >= 50 ? pal::Bad : m_combo >= 25 ? pal::Accent : pal::Highlight;
		int cx = rtl ? px + pw + 60 : px - pw - 60;
		c.TextCentered(buf, font::Body, cx, 0, 72, cc, rtl ? AlignLeft : AlignRight, rtl);
	}

	// Progress bar
	int total = 0, done = 0;
	for (int i = 0; i < m_ex.lines; i++) {
		total += m_ex.len[i];
		if (i < m_line)
			done += m_ex.len[i];
	}
	done += m_pos;
	c.FillRect(0, 72, W, 8, 0xFFD5D9E6);
	int bw = total ? W * done / total : 0;
	c.FillRect(rtl ? W - bw : 0, 72, bw, 8, pal::Good);

	// Text panel
	int panelY = 96, panelH = 190;
	c.FillRoundRect(40, panelY + 6, W - 80, panelH, 26, pal::Shadow);
	c.FillRoundRect(40, panelY, W - 80, panelH, 26, pal::Panel);
	if (m_line < m_ex.lines) {
		static text::Layout l;
		font::Size size = font::Type;
		text::LayoutText(m_ex.text[m_line], m_ex.len[m_line], size, rtl, &l);
		if (l.width > W - 140) {
			size = font::Title;
			text::LayoutText(m_ex.text[m_line], m_ex.len[m_line], size, rtl, &l);
		}
		int x0 = (W - l.width) / 2, base = panelY + 105;
		bool flash = m_now < m_wrongUntil;
		// Highlight box behind the current character.
		for (int i = 0; i < l.count; i++) {
			const text::PlacedGlyph &g = l.glyphs[i];
			if (m_pos < g.logical || m_pos >= g.logical + g.span)
				continue;
			int gw = g.glyph ? g.glyph->adv : 20;
			Color box = m_errHere ? (flash ? pal::Bad : 0xFFFFC9C9) : pal::Highlight;
			c.FillRoundRect(x0 + g.x - 4, base - l.ascent + 4, gw + 8, l.ascent + l.descent - 4, 10, box);
			c.FillRect(x0 + g.x, base + l.descent - 6, gw, 5, m_errHere ? pal::Bad : pal::Accent);
		}
		for (int i = 0; i < l.count; i++) {
			const text::PlacedGlyph &g = l.glyphs[i];
			int li = g.logical + g.span - 1;
			Color col = pal::Ink;
			if (li < m_pos)
				col = m_mark[m_line][li] == 2 ? pal::Bad : pal::Good;
			else if (g.logical > m_pos)
				col = 0xFF4A5070;
			c.DrawGlyph(g, x0 + g.x, base, col);
		}
		// Next line preview
		if (m_line + 1 < m_ex.lines) {
			static text::Layout nl;
			text::LayoutText(m_ex.text[m_line + 1], m_ex.len[m_line + 1], font::Body, rtl, &nl);
			c.DrawLayout(nl, (W - nl.width) / 2, panelY + 165, pal::Faint);
		}
	}
	// Combo celebration
	if (m_now < m_comboUntil) {
		int t = (int)(m_comboUntil - m_now);
		u32 a = (u32)Min(255, t * 255 / 400);
		buf[0] = 0;
		StrAppend(buf, T(S_Combo), sizeof buf);
		StrAppend(buf, " ", sizeof buf);
		FormatNum(num, sizeof num, (u32)m_comboShown);
		StrAppend(buf, num, sizeof buf);
		StrAppend(buf, "!", sizeof buf);
		int rise = (1300 - t) / 30;
		c.Text(buf, font::Title, rtl ? 90 : W - 90, panelY + 50 - rise, WithAlpha(pal::Accent, a),
		       rtl ? AlignLeft : AlignRight, rtl);
	}

	// Keyboard with the next key and its finger.
	u8 target = 0, shiftKey = 0;
	int finger = -1;
	u32 expected;
	if (ExpectedChar(&expected)) {
		bool shift;
		const KeyDef *k = kbd::KeyFor(expected, m_lang, &shift);
		if (k) {
			target = k->usage;
			finger = k->finger;
			if (shift)
				shiftKey = kbd::IsLeftHand(k->finger) ? KeyRightShift : KeyLeftShift;
		}
	}
	int kbY = 318;
	DrawKeyboard(c, kKbX, kbY, kKbUnit, target, shiftKey, true);
	DrawHands(c, kbY + 30, finger);

	// Guidance line
	int gy = 610;
	if (m_caps && m_lang == LangEn) {
		c.TextCentered(T(S_CapsLockOn), font::Body, W / 2, gy, 50, pal::Bad, AlignCenter, rtl);
	} else if (m_now < m_shiftTipUntil) {
		c.TextCentered(T(S_UseOtherShift), font::Body, W / 2, gy, 50, pal::Accent, AlignCenter, rtl);
	} else if (finger >= 0) {
		buf[0] = 0;
		StrAppend(buf, T(S_Use), sizeof buf);
		StrAppend(buf, " ", sizeof buf);
		StrAppend(buf, T((StrId)(S_FingerLPinky + finger)), sizeof buf);
		int w = text::MeasureUtf8(buf, font::Body, rtl);
		c.FillCircle(W / 2 + (rtl ? w / 2 + 24 : -w / 2 - 24), gy + 25, 12, pal::Finger[finger]);
		c.TextCentered(buf, font::Body, W / 2, gy, 50, pal::Ink, AlignCenter, rtl);
	}
	if (!m_started)
		c.TextCentered(T((StrId)(S_Tip0 + m_attempt % kTipCount)), font::Small, W / 2, gy + 44, 40,
			       pal::InkSoft, AlignCenter, rtl);
	else
		c.TextCentered(T(S_EscPause), font::Small, W / 2, gy + 44, 40, pal::Faint, AlignCenter, rtl);

	if (m_paused) {
		c.FillRect(0, 0, W, H, 0xA0202848);
		c.FillRoundRect(W / 2 - 330, 230, 660, 240, 30, pal::Panel);
		c.TextCentered(T(S_Paused), font::Type, W / 2, 250, 110, pal::Navy, AlignCenter, rtl);
		c.TextCentered(T(S_PauseHint), font::Body, W / 2, 370, 60, pal::InkSoft, AlignCenter, rtl);
	}
}

void App::DrawResults(Canvas &c)
{
	DrawBackground(c);
	char title[128];
	LessonLabel(m_lesson, title, sizeof title);
	DrawHeader(c, title);
	bool rtl = Rtl();
	int t = (int)(m_now - m_screenStart);
	int px = 230, py = 90, pw = W - 460, ph = 566;
	c.FillRoundRect(px, py + 8, pw, ph, 34, pal::Shadow);
	c.FillRoundRect(px, py, pw, ph, 34, pal::Panel);
	c.TextCentered(T(m_resStars ? S_LessonComplete : S_TryAgainTitle), font::Title, W / 2, py + 8, 70,
		       pal::Navy, AlignCenter, rtl);

	// Stars pop in one after another.
	for (int i = 0; i < 3; i++) {
		int cx = W / 2 + (rtl ? 1 - i : i - 1) * 130, cy = py + 128;
		int appear = 350 + i * 450;
		bool filled = i < m_resStars && t >= appear;
		int r = 46;
		if (filled && t < appear + 250)
			r = 46 + (appear + 250 - t) / 10;
		if (filled) {
			c.FillStar(cx, cy + 5, r, pal::GoldDark);
			c.FillStar(cx, cy, r, pal::Gold);
		} else
			c.FillStar(cx, cy, 46, 0xFFE3E6EE);
	}

	// Stats
	char num[16], val[32];
	struct Stat
	{
		StrId label;
		int value;
		const char *suffix;
	} stats[3] = {{S_Accuracy, m_resAcc, rtl ? "٪" : "%"}, {S_Speed, m_resWpm, ""}, {S_Points, m_resPoints, ""}};
	for (int i = 0; i < 3; i++) {
		int cx = W / 2 + (rtl ? 1 - i : i - 1) * 250;
		c.FillRoundRect(cx - 110, py + 196, 220, 112, 20, 0xFFF3F5FA);
		c.TextCentered(T(stats[i].label), font::Small, cx, py + 198, 36, pal::InkSoft, AlignCenter, rtl);
		FormatNum(num, sizeof num, (u32)stats[i].value);
		val[0] = 0;
		if (i == 2)
			StrAppend(val, "+", sizeof val);
		StrAppend(val, num, sizeof val);
		StrAppend(val, stats[i].suffix, sizeof val);
		c.TextCentered(val, font::Type, cx, py + 224, 62, i == 0 && m_resAcc < 90 ? pal::Bad : pal::Ink,
			       AlignCenter, rtl);
		if (i == 1)
			c.TextCentered(T(S_Wpm), font::Small, cx, py + 276, 30, pal::InkSoft, AlignCenter, rtl);
	}
	StrId msg = (StrId)(S_Stars0 - m_resStars);
	c.TextCentered(T(msg), font::Body, W / 2, py + 316, 46, pal::Ink, AlignCenter, rtl);

	// Keys that caused mistakes, most frequent first.
	int order[16];
	for (int i = 0; i < m_errN; i++)
		order[i] = i;
	for (int i = 0; i < m_errN; i++)
		for (int j = i + 1; j < m_errN; j++)
			if (m_errCount[order[j]] > m_errCount[order[i]])
				Swap(order[i], order[j]);
	int shown = Min(m_errN, 5);
	if (shown == 0) {
		c.TextCentered(T(S_NoMistakes), font::Body, W / 2, py + 370, 60, pal::Good, AlignCenter, rtl);
	} else {
		const char *label = T(S_TrickyKeys);
		int lw = text::MeasureUtf8(label, font::Body, rtl);
		int totalW = lw + 24 + shown * 76;
		int x = (W - totalW) / 2;
		if (rtl) {
			c.TextCentered(label, font::Body, x + totalW, py + 368, 64, pal::InkSoft, AlignRight, rtl);
			for (int i = 0; i < shown; i++) {
				u32 cp = m_errCp[order[i]];
				bool shift;
				const KeyDef *k = kbd::KeyFor(cp, m_lang, &shift);
				DrawKeycap(c, x + totalW - lw - 24 - 36 - i * 76, py + 400, 60, cp,
					   pal::Finger[k ? (int)k->finger : (int)Thumb]);
			}
		} else {
			c.TextCentered(label, font::Body, x, py + 368, 64, pal::InkSoft, AlignLeft);
			for (int i = 0; i < shown; i++) {
				u32 cp = m_errCp[order[i]];
				bool shift;
				const KeyDef *k = kbd::KeyFor(cp, m_lang, &shift);
				DrawKeycap(c, x + lw + 24 + 36 + i * 76, py + 400, 60, cp, pal::Finger[k ? (int)k->finger : (int)Thumb]);
			}
		}
	}
	// New badges
	if (m_newBadges) {
		int count = 0;
		for (int b = 0; b < B_Count; b++)
			count += (m_newBadges >> b) & 1;
		int x = W / 2 - (count - 1) * 45;
		c.TextCentered(T(S_NewBadge), font::Body, W / 2, py + 440, 36, pal::Accent, AlignCenter, rtl);
		for (int b = 0; b < B_Count; b++)
			if ((m_newBadges >> b) & 1) {
				DrawMedal(c, x, py + 504, 26, b, true);
				x += 90;
			}
	}
	for (int i = 0; i < kMaxParticles; i++) {
		const Particle &p = m_particles[i];
		if (p.life > 0)
			c.FillRect((int)p.x, (int)p.y, 9, 14, WithAlpha(p.color, (u32)Min(255, p.life / 4)));
	}
	DrawHint(c, T(m_resStars ? S_ResultsHint : S_ResultsHintFail));
}
