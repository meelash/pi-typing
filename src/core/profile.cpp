#include "profile.h"
#include "strings.h"

int Profile::CourseStars(Lang lang) const
{
	int s = 0;
	for (int i = 0; i < curriculum::Count(lang); i++)
		s += course[lang].stars[i];
	return s;
}

int Profile::TotalStars() const
{
	int s = 0;
	for (int l = 0; l < LangCount; l++)
		s += CourseStars((Lang)l);
	return s;
}

int Profile::Unlocked(Lang lang) const
{
	int n = curriculum::Count(lang);
	for (int i = 0; i < n; i++)
		if (!course[lang].stars[i])
			return i + 1;
	return n;
}

int Profile::Completed(Lang lang) const
{
	int c = 0;
	for (int i = 0; i < curriculum::Count(lang); i++)
		c += course[lang].stars[i] > 0;
	return c;
}

int Profile::Rank() const
{
	int stars = TotalStars(), r = 0;
	for (int i = 0; i < kRankCount; i++)
		if (stars >= kRankStars[i])
			r = i;
	return r;
}

void Store::Reset()
{
	MemZero(this, sizeof *this);
	sound = true;
}

void Store::Remove(int index)
{
	for (int i = index; i + 1 < count; i++)
		players[i] = players[i + 1];
	if (count > 0)
		count--;
}

namespace {

struct Writer
{
	char *buf;
	int cap, len;
	bool ok = true;

	void Put(const char *s)
	{
		while (*s) {
			if (len >= cap - 1) {
				ok = false;
				return;
			}
			buf[len++] = *s++;
		}
		buf[len] = 0;
	}
	void Num(u32 v)
	{
		char t[12] = "";
		StrAppendUInt(t, v, sizeof t);
		Put(t);
	}
	void Field(const char *key, u32 v)
	{
		Put(key);
		Put("=");
		Num(v);
		Put("\n");
	}
	void List(const char *prefix, const char *key, const u8 *v, int n)
	{
		Put(prefix);
		Put(key);
		Put("=");
		for (int i = 0; i < n; i++) {
			if (i)
				Put(",");
			Num(v[i]);
		}
		Put("\n");
	}
};

const char *const kLangKey[LangCount] = {"en.", "ar.", "ur."};

void ParseList(const char *s, u8 *out, int n)
{
	for (int i = 0; i < n && *s; i++) {
		u32 v = 0;
		while (*s >= '0' && *s <= '9')
			v = v * 10 + (u32)(*s++ - '0');
		out[i] = (u8)Min<u32>(v, 255);
		if (*s == ',')
			s++;
	}
}

} // namespace

int Store::Serialize(char *buf, int cap) const
{
	Writer w{buf, cap, 0};
	w.Put("# Typing Adventure progress file\ntyping-adventure 1\n");
	w.Field("sound", sound);
	for (int p = 0; p < count; p++) {
		const Profile &pr = players[p];
		w.Put("[player]\nname=");
		w.Put(pr.name);
		w.Put("\n");
		w.Field("color", pr.color);
		w.Field("lang", pr.lang);
		w.Field("namelang", pr.nameLang);
		w.Field("points", pr.points);
		w.Field("badges", pr.badges);
		w.Field("keys", pr.keys);
		w.Field("combo", pr.bestCombo);
		for (int l = 0; l < LangCount; l++) {
			const CourseProgress &c = pr.course[l];
			int n = curriculum::Count((Lang)l);
			w.List(kLangKey[l], "stars", c.stars, n);
			w.List(kLangKey[l], "acc", c.bestAcc, n);
			w.List(kLangKey[l], "wpm", c.bestWpm, n);
			w.List(kLangKey[l], "balloon", c.gameLevel[0], n);
			w.List(kLangKey[l], "balloonw", c.gameLevel[1], n);
			w.Put(kLangKey[l]);
			w.Field("balloonscore", c.gameBest);
		}
	}
	return w.ok ? w.len : -1;
}

bool Store::Parse(const char *buf, int len)
{
	Reset();
	static char line[512];
	bool header = false;
	Profile *cur = 0;
	int i = 0;
	while (i < len) {
		int n = 0;
		while (i < len && buf[i] != '\n') {
			if (n < (int)sizeof line - 1 && buf[i] != '\r')
				line[n++] = buf[i];
			i++;
		}
		i++;
		line[n] = 0;
		if (n == 0 || line[0] == '#')
			continue;
		if (StrEq(line, "typing-adventure 1")) {
			header = true;
			continue;
		}
		if (!header)
			return false;
		if (StrEq(line, "[player]")) {
			if (count >= kMaxProfiles)
				break;
			cur = &players[count++];
			continue;
		}
		char *eq = line;
		while (*eq && *eq != '=')
			eq++;
		if (!*eq)
			continue;
		*eq = 0;
		const char *key = line, *val = eq + 1;
		u32 v = 0;
		bool num = ParseUInt(val, &v);
		if (!cur) {
			if (StrEq(key, "sound") && num)
				sound = v != 0;
			continue;
		}
		if (StrEq(key, "name"))
			StrCopy(cur->name, val, kMaxNameBytes);
		else if (StrEq(key, "color") && num)
			cur->color = (u8)v;
		else if (StrEq(key, "lang") && num)
			cur->lang = (u8)(v < LangCount ? v : 0);
		else if (StrEq(key, "namelang") && num)
			cur->nameLang = (u8)(v < LangCount ? v : 0);
		else if (StrEq(key, "points") && num)
			cur->points = v;
		else if (StrEq(key, "badges") && num)
			cur->badges = v;
		else if (StrEq(key, "keys") && num)
			cur->keys = v;
		else if (StrEq(key, "combo") && num)
			cur->bestCombo = (u16)v;
		else
			for (int l = 0; l < LangCount; l++) {
				const char *p = kLangKey[l];
				if (key[0] != p[0] || key[1] != p[1] || key[2] != '.')
					continue;
				const char *k = key + 3;
				CourseProgress &c = cur->course[l];
				if (StrEq(k, "stars"))
					ParseList(val, c.stars, curriculum::kMaxLessons);
				else if (StrEq(k, "acc"))
					ParseList(val, c.bestAcc, curriculum::kMaxLessons);
				else if (StrEq(k, "wpm"))
					ParseList(val, c.bestWpm, curriculum::kMaxLessons);
				else if (StrEq(k, "balloon"))
					ParseList(val, c.gameLevel[0], curriculum::kMaxLessons);
				else if (StrEq(k, "balloonw"))
					ParseList(val, c.gameLevel[1], curriculum::kMaxLessons);
				else if (StrEq(k, "balloonscore") && num)
					c.gameBest = v;
				// "game" (the old points-based record) is dropped: it used another scale.
			}
	}
	return header;
}
