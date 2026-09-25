// Balloon Pop: balloons carrying letters (or words) float up; type them to
// pop them before they escape. Uses only the keys the player has learned.
#include "app.h"
#include "palette.h"
#include "sfx.h"
#include "text.h"

static const int kSkyBottom = 660;
static const Color kBalloonColors[] = {0xFFFF6B6B, 0xFFFFA94D, 0xFFFCC419, 0xFF51CF66,
				       0xFF339AF0, 0xFF845EF7, 0xFFF06595, 0xFF20C997};

void App::KeyGameIntro(const KeyEvent &e)
{
	switch (e.usage) {
	case KeyTab:
	case KeyLeft:
	case KeyRight:
		if (m_pool.wordCount >= 8) {
			m_gameWords = !m_gameWords;
			sfx::Trigger(SfxMove);
		}
		break;
	case KeyEnter:
	case KeyPadEnter:
	case KeySpace:
		sfx::Trigger(SfxSelect);
		StartGame();
		break;
	case KeyEscape:
		m_sel = curriculum::Count(m_lang);
		Go(ScrMap);
		break;
	}
}

bool App::GameTarget(u32 *cp) const
{
	if (m_screen != ScrGame || m_gameOver)
		return false;
	if (m_lock >= 0 && m_balloons[m_lock].alive) {
		*cp = m_balloons[m_lock].text[m_balloons[m_lock].typed];
		return true;
	}
	int best = -1;
	for (int i = 0; i < kMaxBalloons; i++)
		if (m_balloons[i].alive && (best < 0 || m_balloons[i].y < m_balloons[best].y))
			best = i;
	if (best < 0)
		return false;
	*cp = m_balloons[best].text[0];
	return true;
}

void App::StartGame()
{
	for (int i = 0; i < kMaxBalloons; i++)
		m_balloons[i].alive = false;
	for (int i = 0; i < kMaxParticles; i++)
		m_particles[i].life = 0;
	m_score = m_pops = 0;
	m_lives = 3;
	m_level = 1;
	m_lock = -1;
	m_spawnMs = 600;
	m_gameOver = false;
	m_gameRecord = false;
	m_newBadges = 0;
	m_levelBannerUntil = m_now + 1200;
	Go(ScrGame);
}

void App::KeyGame(const KeyEvent &e)
{
	if (m_gameOver) {
		if (m_now - m_screenStart < 700)
			return;
		if (e.usage == KeyEnter || e.usage == KeyPadEnter || e.usage == KeySpace)
			StartGame();
		else if (e.usage == KeyEscape) {
			m_sel = curriculum::Count(m_lang);
			Go(ScrMap);
		}
		return;
	}
	if (e.usage == KeyEscape) {
		EndGame();
		return;
	}
	const KeyDef *k = kbd::Find(e.usage);
	if (!k || (e.mods & (ModCtrl | ModAlt)))
		return;
	u32 out[2];
	int n = kbd::Output(*k, m_lang, (e.mods & ModShift) != 0, m_caps, out);
	for (int i = 0; i < n; i++)
		GameType(out[i]);
}

void App::GameType(u32 cp)
{
	if (cp == ' ')
		return;
	// Continue the word that is already started.
	if (m_lock >= 0 && m_balloons[m_lock].alive) {
		Balloon &b = m_balloons[m_lock];
		if (b.text[b.typed] != cp) {
			sfx::Trigger(SfxError);
			return;
		}
		b.typed++;
	} else {
		// Pick the highest balloon (the one closest to escaping) that starts with cp.
		m_lock = -1;
		for (int i = 0; i < kMaxBalloons; i++) {
			const Balloon &b = m_balloons[i];
			if (b.alive && b.text[0] == cp && (m_lock < 0 || b.y < m_balloons[m_lock].y))
				m_lock = i;
		}
		if (m_lock < 0) {
			sfx::Trigger(SfxError);
			return;
		}
		m_balloons[m_lock].typed = 1;
	}
	Balloon &b = m_balloons[m_lock];
	if (b.typed < b.len) {
		sfx::Trigger(SfxKey);
		return;
	}
	// Pop!
	b.alive = false;
	m_lock = -1;
	m_score += 5 * b.len + 5 * m_level;
	m_pops++;
	Burst((int)b.x, (int)b.y, b.color, 26);
	sfx::Trigger(SfxPop);
	if (m_pops % 10 == 0) {
		m_level++;
		m_levelBannerUntil = m_now + 1200;
		sfx::Trigger(SfxLevelUp);
	}
}

void App::SpawnBalloon()
{
	int slot = -1;
	for (int i = 0; i < kMaxBalloons; i++)
		if (!m_balloons[i].alive) {
			slot = i;
			break;
		}
	if (slot < 0 || m_pool.letters.n == 0)
		return;
	Balloon &b = m_balloons[slot];
	b.typed = 0;
	if (m_gameWords && m_pool.wordCount) {
		b.len = text::Utf8Decode(m_pool.words[m_rng.Below(m_pool.wordCount)], b.text, 8);
	} else {
		b.len = 1;
		b.text[0] = m_pool.letters.cps[m_rng.Below(m_pool.letters.n)];
	}
	// Words starting with the same letter as a floating word would be ambiguous.
	if (m_gameWords)
		for (int i = 0; i < kMaxBalloons; i++)
			if (i != slot && m_balloons[i].alive && m_balloons[i].text[0] == b.text[0])
				return;
	static text::Layout l;
	text::LayoutText(b.text, b.len, m_gameWords ? font::Title : font::Type, Rtl(), &l);
	b.rx = (float)Max(46, l.width / 2 + 26);
	b.ry = b.rx < 60 ? 58.0f : 62.0f;
	b.x = (float)(int)(b.rx + 30 + (float)m_rng.Below(W - 60 - 2 * (int)b.rx));
	b.y = (float)(kSkyBottom + 10);
	float speed = m_gameWords ? 32.0f + 6.0f * (float)m_level : 50.0f + 8.0f * (float)m_level;
	b.vy = -speed * (0.85f + 0.3f * (float)m_rng.Below(100) / 100.0f);
	b.sway = (float)m_rng.Below(1024);
	b.color = kBalloonColors[m_rng.Below(ARRAY_LEN(kBalloonColors))];
	b.alive = true;
}

void App::Burst(int x, int y, Color color, int count)
{
	for (int i = 0, made = 0; i < kMaxParticles && made < count; i++) {
		Particle &p = m_particles[i];
		if (p.life > 0)
			continue;
		int ang = m_rng.Below(1024), spd = 80 + m_rng.Below(260);
		p.x = (float)x;
		p.y = (float)y;
		p.vx = (float)(ICos(ang) * spd) / 1024.0f;
		p.vy = (float)(ISin(ang) * spd) / 1024.0f - 120.0f;
		p.life = 700 + m_rng.Below(700);
		p.color = m_rng.Below(3) ? color : pal::Gold;
		made++;
	}
}

void App::UpdateGame(int dt)
{
	float s = (float)dt / 1000.0f;
	for (int i = 0; i < kMaxParticles; i++) {
		Particle &p = m_particles[i];
		if (p.life <= 0)
			continue;
		p.life -= dt;
		p.x += p.vx * s;
		p.y += p.vy * s;
		p.vy += 420.0f * s;
	}
	for (int i = 0; i < kClouds; i++) {
		m_clouds[i].x += (float)m_clouds[i].size * 0.15f * s;
		if (m_clouds[i].x > W + 150)
			m_clouds[i].x = -150;
	}
	if (m_screen != ScrGame || m_gameOver)
		return;

	m_spawnMs -= dt;
	if (m_spawnMs <= 0) {
		SpawnBalloon();
		int base = m_gameWords ? 3400 : 1900;
		m_spawnMs = Max(m_gameWords ? 1300 : 650, base - (m_level - 1) * 160) + m_rng.Below(400);
	}
	for (int i = 0; i < kMaxBalloons; i++) {
		Balloon &b = m_balloons[i];
		if (!b.alive)
			continue;
		b.y += b.vy * s;
		b.sway += (float)dt * 0.35f;
		if (b.y + b.ry < 60) {
			b.alive = false;
			if (m_lock == i)
				m_lock = -1;
			m_lives--;
			sfx::Trigger(SfxLose);
			if (m_lives <= 0)
				EndGame();
		}
	}
}

void App::EndGame()
{
	m_gameOver = true;
	Profile &p = Player();
	CourseProgress &cp = p.course[m_lang];
	m_gameRecord = (u32)m_score > cp.gameBest && m_score > 0;
	if (m_gameRecord)
		cp.gameBest = (u32)m_score;
	p.points += (u32)m_score / 2;
	u32 before = p.badges;
	AwardBadges(before);
	Save();
	if (m_gameRecord)
		for (int i = 0; i < 5; i++)
			Burst(300 + m_rng.Below(W - 600), 200 + m_rng.Below(150), pal::Avatar[m_rng.Below(8)], 20);
	m_screenStart = m_now;
}

// ---------------------------------------------------------------------------

static void DrawCloud(Canvas &c, int x, int y, int s)
{
	Color col = 0xE0FFFFFF;
	c.FillEllipse(x, y, s, s * 6 / 10, col);
	c.FillEllipse(x - s * 7 / 10, y + s / 6, s * 6 / 10, s * 4 / 10, col);
	c.FillEllipse(x + s * 7 / 10, y + s / 6, s * 6 / 10, s * 4 / 10, col);
}

static void DrawSky(Canvas &c, const App::Cloud *clouds, int n, u32 now)
{
	c.VGradient(0, 0, App::W, kSkyBottom, 0xFF8FD3FF, 0xFFE3F6FF);
	c.FillCircle(App::W - 150, 170, 58, 0x60FFE066);
	c.FillCircle(App::W - 150, 170, 44, 0xFFFFE066);
	for (int i = 0; i < n; i++)
		DrawCloud(c, (int)clouds[i].x, (int)clouds[i].y, clouds[i].size);
	// Rolling hills
	c.FillRect(0, kSkyBottom, App::W, App::H - kSkyBottom, 0xFF69C779);
	for (int i = 0; i < 6; i++)
		c.FillEllipse(i * 260 + 60, kSkyBottom + 10, 190, 60, i % 2 ? 0xFF5BB86B : 0xFF7CD68A);
	(void)now;
}

void App::DrawGameIntro(Canvas &c)
{
	DrawSky(c, m_clouds, kClouds, m_now);
	bool rtl = Rtl();
	c.FillRoundRect(190, 70, W - 380, 540, 36, 0xF0FFFFFF);
	c.TextCentered(T(S_BalloonGame), font::Type, W / 2, 90, 80, pal::Navy, AlignCenter, rtl);
	// Demo balloons
	for (int i = 0; i < 3; i++) {
		int bx = W / 2 + (i - 1) * 150;
		int by = 250 + ISin((int)(m_now / 3 + i * 300) & 1023) * 12 / 1024;
		Color col = kBalloonColors[(i * 3) % ARRAY_LEN(kBalloonColors)];
		c.Line(bx, by + 55, bx + 6, by + 120, 3, 0xFF8C93A8);
		c.FillEllipse(bx, by, 48, 58, col);
		c.FillEllipse(bx - 16, by - 20, 10, 16, 0x70FFFFFF);
		if (m_pool.letters.n) {
			char s[8];
			u32 cp = m_pool.letters.cps[(i * 5) % m_pool.letters.n];
			text::Utf8Encode(&cp, 1, s, sizeof s);
			c.TextCentered(s, font::Type, bx, by - 58, 110, 0xFFFFFFFF, AlignCenter, rtl);
		}
	}
	c.TextCentered(T(m_gameWords ? S_GameIntroWords : S_GameIntroLetters), font::Body, W / 2, 390, 50, pal::Ink,
		       AlignCenter, rtl);
	// Mode switch
	bool wordsOk = m_pool.wordCount >= 8;
	for (int i = 0; i < 2; i++) {
		bool on = (i == 1) == m_gameWords;
		int bx = W / 2 + (rtl ? (i == 0 ? 20 : -220) : (i == 0 ? -220 : 20)), by = 460;
		Color col = on ? pal::Accent : (i == 1 && !wordsOk ? 0xFFE9EBF2 : 0xFFF1F3F8);
		c.FillRoundRect(bx, by, 200, 56, 28, col);
		c.TextCentered(T(i == 0 ? S_Letters : S_Words), font::Body, bx + 100, by, 56,
			       on ? 0xFFFFFFFF : (i == 1 && !wordsOk ? pal::Faint : pal::Ink), AlignCenter, rtl);
	}
	char buf[64] = "", num[16];
	StrAppend(buf, T(S_Best), sizeof buf);
	StrAppend(buf, ": ", sizeof buf);
	FormatNum(num, sizeof num, Player().course[m_lang].gameBest);
	StrAppend(buf, num, sizeof buf);
	c.TextCentered(buf, font::Body, W / 2, 540, 50, pal::InkSoft, AlignCenter, rtl);
	DrawHint(c, T(S_GameModeHint));
}

void App::DrawGame(Canvas &c)
{
	DrawSky(c, m_clouds, kClouds, m_now);
	bool rtl = Rtl();
	for (int i = 0; i < kMaxBalloons; i++) {
		const Balloon &b = m_balloons[i];
		if (!b.alive)
			continue;
		int sway = ISin((int)b.sway & 1023) * 10 / 1024;
		int x = (int)b.x + sway, y = (int)b.y, rx = (int)b.rx, ry = (int)b.ry;
		c.Line(x, y + ry, x - sway, y + ry + 70, 3, 0xFF7C8398);
		Point knot[3] = {{x - 8, y + ry + 10}, {x + 8, y + ry + 10}, {x, y + ry - 4}};
		c.FillPolygon(knot, 3, Darken(b.color, 40));
		c.FillEllipse(x, y, rx, ry, b.color);
		c.FillEllipse(x - rx / 3, y - ry / 3, Max(6, rx / 5), ry / 4, 0x70FFFFFF);
		static text::Layout l;
		font::Size size = m_gameWords ? font::Title : font::Type;
		text::LayoutText(b.text, b.len, size, rtl, &l);
		int tx = x - l.width / 2, base = y + (m_gameWords ? 16 : 20);
		for (int g = 0; g < l.count; g++) {
			const text::PlacedGlyph &pg = l.glyphs[g];
			bool typed = pg.logical < b.typed && i == m_lock;
			c.DrawGlyph(pg, tx + pg.x + 2, base + 2, 0x50000000);
			c.DrawGlyph(pg, tx + pg.x, base, typed ? 0xFF2B3045 : 0xFFFFFFFF);
		}
		if (i == m_lock)
			c.StrokeRoundRect(x - rx - 6, y - ry - 6, 2 * rx + 12, 2 * ry + 12, ry + 6, 4, 0xFFFFFFFF);
	}
	for (int i = 0; i < kMaxParticles; i++) {
		const Particle &p = m_particles[i];
		if (p.life > 0)
			c.FillCircle((int)p.x, (int)p.y, 5, WithAlpha(p.color, (u32)Min(255, p.life / 3)));
	}
	// HUD: score, level, lives.
	char buf[64], num[16];
	c.FillRoundRect(20, 16, 250, 60, 30, 0xC0FFFFFF);
	buf[0] = 0;
	StrAppend(buf, T(S_Score), sizeof buf);
	StrAppend(buf, " ", sizeof buf);
	FormatNum(num, sizeof num, (u32)m_score);
	StrAppend(buf, num, sizeof buf);
	c.TextCentered(buf, font::Body, 145, 16, 60, pal::Navy, AlignCenter, rtl);
	buf[0] = 0;
	StrAppend(buf, T(S_Level), sizeof buf);
	StrAppend(buf, " ", sizeof buf);
	FormatNum(num, sizeof num, (u32)m_level);
	StrAppend(buf, num, sizeof buf);
	c.FillRoundRect(W / 2 - 110, 16, 220, 60, 30, 0xC0FFFFFF);
	c.TextCentered(buf, font::Body, W / 2, 16, 60, pal::Navy, AlignCenter, rtl);
	c.FillRoundRect(W - 230, 16, 210, 60, 30, 0xC0FFFFFF);
	for (int i = 0; i < 3; i++)
		c.FillHeart(W - 175 + i * 55, 46, 40, i < m_lives ? pal::Bad : 0xFFD5D9E6);

	if (m_now < m_levelBannerUntil && !m_gameOver) {
		int t = (int)(m_levelBannerUntil - m_now);
		c.Text(buf, font::Type, W / 2, 380, WithAlpha(pal::Navy, (u32)Min(255, t / 3)), AlignCenter, rtl);
	}
	if (m_gameOver) {
		c.FillRect(0, 0, W, H, 0x80202848);
		int py = 110, ph = 500;
		c.FillRoundRect(W / 2 - 330, py, 660, ph, 36, pal::Panel);
		c.TextCentered(T(S_GameOver), font::Type, W / 2, py + 15, 90, pal::Navy, AlignCenter, rtl);
		FormatNum(num, sizeof num, (u32)m_score);
		c.TextCentered(num, font::Huge, W / 2, py + 105, 140, pal::Accent, AlignCenter, rtl);
		buf[0] = 0;
		if (m_gameRecord)
			StrAppend(buf, T(S_NewRecord), sizeof buf);
		else {
			StrAppend(buf, T(S_Best), sizeof buf);
			StrAppend(buf, ": ", sizeof buf);
			FormatNum(num, sizeof num, Player().course[m_lang].gameBest);
			StrAppend(buf, num, sizeof buf);
		}
		c.TextCentered(buf, font::Title, W / 2, py + 250, 60, m_gameRecord ? pal::Good : pal::InkSoft, AlignCenter,
			       rtl);
		if (m_newBadges) {
			int count = 0;
			for (int b = 0; b < B_Count; b++)
				count += (m_newBadges >> b) & 1;
			c.TextCentered(T(S_NewBadge), font::Body, W / 2, py + 312, 40, pal::Accent, AlignCenter, rtl);
			int x = W / 2 - (count - 1) * 40;
			for (int b = 0; b < B_Count; b++)
				if ((m_newBadges >> b) & 1) {
					DrawMedal(c, x, py + 385, 28, b, true);
					x += 80;
				}
		}
		c.TextCentered(T(S_ResultsHintFail), font::Small, W / 2, py + ph - 50, 36, pal::InkSoft, AlignCenter, rtl);
	} else
		DrawHint(c, T(S_GameHint));
}
