#include "curriculum.h"
#include "content.h"
#include "text.h"

namespace curriculum {

static const LessonDef kLessonsEn[] = {
	{NewKeys, "fj", 8},   // index-finger anchors (feel the bumps)
	{NewKeys, "dk", 8},   // middle fingers
	{NewKeys, "sl", 8},   // ring fingers
	{NewKeys, "a;", 8},   // little fingers
	{NewKeys, "gh", 8},   // index fingers reach inward
	{Review, "", 10},     // whole home row
	{NewKeys, "ei", 10},  // top row, middle fingers
	{NewKeys, "ru", 10},  // top row, index fingers
	{NewKeys, "to", 10},
	{Review, "", 12},
	{NewKeys, "cn", 12},  // bottom row
	{NewKeys, "wm", 12},
	{NewKeys, "yp", 12},
	{NewKeys, "vb", 12},
	{Review, "", 14},
	{NewKeys, "qzx", 14},
	{Review, "", 15},     // every letter
	{Capitals, "", 15},   // shift with the opposite hand
	{NewKeys, ".,", 15},
	{NewKeys, "'?", 15},
	{NewKeys, "12345", 15},
	{NewKeys, "67890", 15},
	{Final, "", 18},
};

static const LessonDef kLessonsAr[] = {
	{NewKeys, "بت", 8},   // index-finger anchors (F/J bumps)
	{NewKeys, "ين", 8},
	{NewKeys, "سم", 8},
	{NewKeys, "شك", 8},
	{NewKeys, "لا", 8},   // index fingers reach inward
	{NewKeys, "ط", 8},    // right little finger reach
	{Review, "", 10},
	{NewKeys, "ثه", 10},  // top row, middle fingers
	{NewKeys, "قع", 10},  // top row, index fingers
	{NewKeys, "فغ", 10},
	{Review, "", 12},
	{NewKeys, "صخ", 12},  // ring fingers
	{NewKeys, "ضح", 12},  // little fingers
	{NewKeys, "جد", 12},
	{Review, "", 12},
	{NewKeys, "رو", 14},  // bottom row
	{NewKeys, "ةى", 14},
	{NewKeys, "زء", 14},
	{NewKeys, "ؤئ", 14},
	{NewKeys, "ظذ", 14},
	{Review, "", 15},     // every letter
	{NewKeys, "أإآ", 15}, // hamza forms with Shift
	{NewKeys, "،؟.", 15},
	{NewKeys, "12345", 15},
	{NewKeys, "67890", 15},
	{Final, "", 18},
};

static const char *const kNamesEn[] = {"Sam", "Ali", "Omar", "Sara", "Lina", "Adam", "Maya",
				       "Noah", "Zaid", "Huda", "Yusuf", "Mona", "Hana", "Leo",
				       "Amir", "Rosa", "Kofi", "Iris", "Jack", "Emma"};

static const char *const kContractionsEn[] = {"it's", "let's", "don't", "can't", "I'm",
					      "we're", "you're", "that's", "isn't", "he's",
					      "she's", "what's", "I'll", "won't"};

int Count(Lang lang) { return lang == LangAr ? ARRAY_LEN(kLessonsAr) : ARRAY_LEN(kLessonsEn); }

const LessonDef &Get(Lang lang, int i) { return lang == LangAr ? kLessonsAr[i] : kLessonsEn[i]; }

bool CharSet::Has(u32 cp) const
{
	for (int i = 0; i < n; i++)
		if (cps[i] == cp)
			return true;
	return false;
}

void CharSet::Add(u32 cp)
{
	if (!Has(cp) && n < ARRAY_LEN(cps))
		cps[n++] = cp;
}

static void AddUtf8(CharSet *set, const char *s)
{
	u32 cps[16];
	int n = text::Utf8Decode(s, cps, 16);
	for (int i = 0; i < n; i++)
		set->Add(cps[i]);
}

void Allowed(Lang lang, int lesson, CharSet *out)
{
	out->n = 0;
	out->Add(' ');
	bool capitals = false;
	for (int i = 0; i <= lesson; i++) {
		AddUtf8(out, Get(lang, i).keys);
		capitals |= Get(lang, i).kind == Capitals;
	}
	if (capitals)
		for (int i = 0, n = out->n; i < n; i++)
			if (out->cps[i] >= 'a' && out->cps[i] <= 'z')
				out->Add(text::ToUpper(out->cps[i]));
}

void NewChars(Lang lang, int lesson, CharSet *out)
{
	out->n = 0;
	AddUtf8(out, Get(lang, lesson).keys);
}

static bool IsLetter(u32 cp) { return text::IsLatinLetter(cp) || text::IsArabicLetter(cp); }
static bool IsDigit(u32 cp) { return cp >= '0' && cp <= '9'; }

int LastHomeRowLesson(Lang lang)
{
	int last = -1;
	for (int i = 0; i < Count(lang); i++) {
		CharSet s;
		NewChars(lang, i, &s);
		for (int k = 0; k < s.n; k++) {
			bool shift;
			const KeyDef *key = kbd::KeyFor(s.cps[k], lang, &shift);
			if (!key || key->row != 2)
				return last;
		}
		last = i;
	}
	return last;
}

int LastLetterLesson(Lang lang)
{
	int last = 0;
	for (int i = 0; i < Count(lang); i++) {
		CharSet s;
		NewChars(lang, i, &s);
		for (int k = 0; k < s.n; k++)
			if (IsLetter(s.cps[k]) && s.cps[k] != text::kAlefHamzaAbove &&
			    s.cps[k] != text::kAlefHamzaBelow && s.cps[k] != text::kAlefMadda)
				last = i;
	}
	return last;
}

// ---------------------------------------------------------------------------

namespace {

struct Line
{
	u32 *buf;
	int len;
	int target;

	bool Fits(int n) const { return len == 0 ? n <= target : len + 1 + n <= target; }
	bool Add(const u32 *t, int n)
	{
		if (!Fits(n))
			return false;
		if (len)
			buf[len++] = ' ';
		for (int i = 0; i < n; i++)
			buf[len++] = t[i];
		return true;
	}
};

struct Gen
{
	Lang lang;
	Rng rng;
	CharSet allowed, fresh, letters, freshLetters;  // freshLetters: new keys to drill (no digits)
	bool usedSentence[64];
	const char *const *words;
	int wordCount;
	int target;

	bool WordOk(const char *w, bool needFresh, int *n, u32 *cps)
	{
		*n = text::Utf8Decode(w, cps, kMaxLineLen);
		bool hasFresh = false;
		for (int i = 0; i < *n; i++) {
			if (!allowed.Has(cps[i]))
				return false;
			hasFresh |= fresh.Has(cps[i]);
		}
		return !needFresh || hasFresh;
	}

	int CountWords(bool needFresh)
	{
		int c = 0, n;
		u32 cps[kMaxLineLen];
		for (int i = 0; i < wordCount; i++)
			c += WordOk(words[i], needFresh, &n, cps);
		return c;
	}

	// Random eligible word, avoiding an immediate repeat.
	int RandomWord(bool needFresh, u32 *cps, const char **last)
	{
		int n = 0;
		for (int tries = 0; tries < 4000; tries++) {
			const char *w = words[rng.Below(wordCount)];
			if (w == *last && tries < 3000)
				continue;
			if (WordOk(w, needFresh, &n, cps)) {
				*last = w;
				return n;
			}
		}
		return 0;
	}

	int Group(const CharSet &pool, const CharSet &must, int len, u32 *out)
	{
		for (int i = 0; i < len; i++)
			out[i] = pool.cps[rng.Below(pool.n)];
		if (must.n)
			out[rng.Below(len)] = must.cps[rng.Below(must.n)];
		return len;
	}

	void FillGroups(Line &line, const CharSet &must)
	{
		u32 tok[8];
		for (int guard = 0; guard < 50; guard++) {
			int n = Group(letters, must, rng.Range(2, 4), tok);
			if (!line.Add(tok, n))
				break;
		}
	}

	void FillWords(Line &line, bool needFresh, bool capitalize = false)
	{
		u32 tok[kMaxLineLen];
		const char *last = 0;
		for (int guard = 0; guard < 50; guard++) {
			int n = RandomWord(needFresh, tok, &last);
			if (!n)
				break;
			if (capitalize)
				tok[0] = text::ToUpper(tok[0]);
			if (!line.Fits(n)) {
				if (line.len > line.target * 2 / 3)
					break;
				continue;
			}
			line.Add(tok, n);
		}
	}

	// Drill of the new keys: "fff jjj fjf jfj ff jj fj jf ..."
	void FillDrill(Line &line, bool withAnchors)
	{
		u32 tok[8];
		const CharSet &f = freshLetters;
		if (!withAnchors) {
			for (int i = 0; i < f.n; i++) {
				tok[0] = tok[1] = tok[2] = f.cps[i];
				line.Add(tok, 3);
			}
			if (f.n >= 2) {
				tok[0] = f.cps[0], tok[1] = f.cps[1], tok[2] = f.cps[0];
				line.Add(tok, 3);
				tok[0] = f.cps[1], tok[1] = f.cps[0], tok[2] = f.cps[1];
				line.Add(tok, 3);
			}
		}
		CharSet pool = f;
		if (withAnchors || f.n == 1)
			for (int i = 0; i < letters.n && pool.n < f.n + 4; i++)
				pool.Add(letters.cps[i]);
		for (int guard = 0; guard < 40; guard++) {
			int n = Group(pool, f, rng.Range(2, 3), tok);
			if (!line.Add(tok, n))
				break;
		}
	}

	// Word followed by a punctuation mark, e.g. "fall, sad." (or a contraction).
	void FillPunct(Line &line)
	{
		u32 tok[kMaxLineLen];
		const char *last = 0;
		for (int guard = 0; guard < 50; guard++) {
			u32 p = fresh.cps[rng.Below(fresh.n)];
			int n;
			if (p == '\'') {
				const char *c = kContractionsEn[rng.Below(ARRAY_LEN(kContractionsEn))];
				if (!WordOk(c, false, &n, tok))
					continue;
			} else {
				n = RandomWord(false, tok, &last);
				if (!n)
					break;
				tok[n++] = p;
			}
			if (!line.Add(tok, n))
				break;
		}
	}

	void FillNumbers(Line &line, int style)
	{
		CharSet digits;
		for (int i = 0; i < allowed.n; i++)
			if (IsDigit(allowed.cps[i]))
				digits.Add(allowed.cps[i]);
		u32 tok[kMaxLineLen];
		const char *last = 0;
		for (int guard = 0; guard < 50; guard++) {
			int n;
			if (style == 0) {  // repeated new digits: 111 22 12
				n = Group(fresh, fresh, rng.Range(2, 3), tok);
			} else if (style == 1) {  // any learned digits
				n = Group(digits, fresh, rng.Range(1, 4), tok);
			} else {  // "3 cats"
				n = Group(digits, fresh, rng.Range(1, 2), tok);
				if (!line.Fits(n + 4))
					break;
				line.Add(tok, n);
				n = RandomWord(false, tok, &last);
			}
			if (!n || !line.Add(tok, n))
				break;
		}
	}

	// Adapts a sentence to the keys taught so far: drops a final '.' and
	// lower-cases the first letter if those have not been taught yet.
	bool SentenceOk(const char *s, bool needFresh, int *n, u32 *cps, bool *adapted)
	{
		*n = text::Utf8Decode(s, cps, kMaxLineLen);
		*adapted = false;
		if (*n > 1 && cps[*n - 1] == '.' && !allowed.Has('.')) {
			(*n)--;
			*adapted = true;
		}
		if (cps[0] >= 'A' && cps[0] <= 'Z' && !allowed.Has(cps[0])) {
			cps[0] += 32;
			*adapted = true;
		}
		bool hasFresh = false;
		for (int i = 0; i < *n; i++) {
			if (!allowed.Has(cps[i]))
				return false;
			hasFresh |= fresh.Has(cps[i]);
		}
		return !needFresh || hasFresh;
	}

	bool FillSentences(Line &line, bool needFresh)
	{
		int count;
		const char *const *s = content::SentencesFor(lang, &count);
		u32 tok[kMaxLineLen];
		int start = rng.Below(count);
		bool any = false;
		for (int k = 0; k < count; k++) {
			int idx = (start + k) % count;
			int n;
			bool adapted;
			if (usedSentence[idx % 64] || !SentenceOk(s[idx], needFresh, &n, tok, &adapted) || !line.Fits(n))
				continue;
			line.Add(tok, n);
			usedSentence[idx % 64] = true;
			any = true;
			if (adapted)
				break;  // without a full stop, one sentence per line reads best
		}
		return any;
	}
};

} // namespace

void Generate(Lang lang, int lesson, u32 seed, Exercise *out)
{
	static Gen g;
	g.lang = lang;
	g.rng.Seed(seed * 2654435761u + (u32)lesson * 97 + 1);
	Allowed(lang, lesson, &g.allowed);
	NewChars(lang, lesson, &g.fresh);
	g.letters.n = 0;
	g.freshLetters.n = 0;
	for (int i = 0; i < g.allowed.n; i++)
		if (IsLetter(g.allowed.cps[i]) && !(g.allowed.cps[i] >= 'A' && g.allowed.cps[i] <= 'Z'))
			g.letters.Add(g.allowed.cps[i]);
	bool anyLetter = false;
	for (int i = 0; i < g.fresh.n; i++)
		anyLetter |= IsLetter(g.fresh.cps[i]);
	// Drill every new key of a letter lesson, including ';' on the home row.
	for (int i = 0; i < g.fresh.n && anyLetter; i++)
		g.freshLetters.Add(g.fresh.cps[i]);
	MemZero(g.usedSentence, sizeof g.usedSentence);
	g.words = content::WordsFor(lang, &g.wordCount);
	g.target = lang == LangAr ? 28 : 32;

	// For review lessons, emphasise the keys from the two previous new-key lessons.
	CharSet recent;
	for (int i = lesson - 1, found = 0; i >= 0 && found < 2; i--)
		if (Get(lang, i).kind == NewKeys) {
			CharSet s;
			NewChars(lang, i, &s);
			for (int k = 0; k < s.n; k++)
				if (IsLetter(s.cps[k]))
					recent.Add(s.cps[k]);
			found++;
		}

	const LessonDef &def = Get(lang, lesson);
	bool digits = g.fresh.n && IsDigit(g.fresh.cps[0]);
	bool punct = g.fresh.n && !digits && g.freshLetters.n == 0;
	out->lines = 5;
	for (int li = 0; li < out->lines; li++) {
		Line line = {out->text[li], 0, g.target};
		switch (def.kind) {
		case NewKeys:
			if (digits)
				g.FillNumbers(line, li < 2 ? 0 : (li < 3 ? 1 : 2));
			else if (punct) {
				if (li < 2 || !g.FillSentences(line, true))
					g.FillPunct(line);
			} else if (li == 0)
				g.FillDrill(line, false);
			else if (li == 1)
				g.FillDrill(line, true);
			else if (li == 2)
				g.FillGroups(line, g.freshLetters);
			else if (g.CountWords(true) >= 3)
				g.FillWords(line, li == 3 || g.CountWords(false) < 6);
			else
				g.FillGroups(line, g.freshLetters);
			break;
		case Review:
			if (li == 1)
				g.FillGroups(line, recent);
			else if (li == 4 && g.FillSentences(line, false))
				;
			else if (g.CountWords(false) >= 5)
				g.FillWords(line, false);
			else
				g.FillGroups(line, recent);
			break;
		case Capitals:
			if (li == 1 || li == 3) {
				int last = -1;
				for (int guard = 0; guard < 20; guard++) {
					int pick = g.rng.Below(ARRAY_LEN(kNamesEn));
					if (pick == last)
						continue;
					last = pick;
					u32 tok[16];
					int n = text::Utf8Decode(kNamesEn[pick], tok, 16);
					if (!line.Add(tok, n))
						break;
				}
			} else
				g.FillWords(line, false, true);
			break;
		case Final:
			line.target = kMaxLineLen - 2;
			if (!g.FillSentences(line, false))
				g.FillWords(line, false);
			break;
		}
		// Never leave a line empty (e.g. no eligible words).
		if (line.len == 0)
			g.FillGroups(line, g.freshLetters.n ? g.freshLetters : g.letters);
		out->len[li] = (u8)line.len;
	}
}

void BuildGamePool(Lang lang, int lessonsDone, GamePool *out)
{
	int lesson = Clamp(lessonsDone - 1, 0, Count(lang) - 1);
	out->stage = 0;
	for (int i = 0; i <= lesson; i++) {
		CharSet s;
		NewChars(lang, i, &s);
		for (int k = 0; k < s.n; k++)
			if (IsLetter(s.cps[k]))
				out->stage = i;
	}
	CharSet allowed;
	Allowed(lang, lesson, &allowed);
	out->letters.n = 0;
	for (int i = 0; i < allowed.n; i++)
		if (IsLetter(allowed.cps[i]) && !(allowed.cps[i] >= 'A' && allowed.cps[i] <= 'Z'))
			out->letters.Add(allowed.cps[i]);
	int count;
	const char *const *words = content::WordsFor(lang, &count);
	out->wordCount = 0;
	for (int i = 0; i < count && out->wordCount < ARRAY_LEN(out->words); i++) {
		u32 cps[kMaxLineLen];
		int n = text::Utf8Decode(words[i], cps, kMaxLineLen);
		bool ok = n >= 2 && n <= 7;
		for (int k = 0; ok && k < n; k++)
			ok = allowed.Has(cps[k]);
		if (ok)
			out->words[out->wordCount++] = words[i];
	}
}

} // namespace curriculum
