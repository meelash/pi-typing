// Players and their progress, saved as a small text file on the SD card.
#pragma once
#include "base.h"
#include "curriculum.h"
#include "keyboard.h"

const int kMaxProfiles = 8;
const int kMaxNameBytes = 64;

struct CourseProgress
{
	u8 stars[curriculum::kMaxLessons];
	u8 bestAcc[curriculum::kMaxLessons];
	u8 bestWpm[curriculum::kMaxLessons];
	u32 gameBest;
};

struct Profile
{
	char name[kMaxNameBytes];  // UTF-8
	u8 color;
	u8 lang;        // last course used
	u32 points;
	u32 badges;     // bit per BadgeId
	u32 keys;       // correct keys typed in lessons
	u16 bestCombo;
	CourseProgress course[LangCount];

	int TotalStars() const;
	int CourseStars(Lang lang) const;
	// Lessons are unlocked in order: finishing one (1+ star) opens the next.
	int Unlocked(Lang lang) const;
	int Completed(Lang lang) const;
	int Rank() const;
	bool HasBadge(int b) const { return (badges >> b) & 1; }
};

struct Store
{
	Profile players[kMaxProfiles];
	int count;
	bool sound;

	void Reset();
	int Serialize(char *buf, int cap) const;
	bool Parse(const char *buf, int len);
	void Remove(int index);
};
