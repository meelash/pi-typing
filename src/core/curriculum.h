// Touch-typing curriculum and practice generation.
//
// Both courses follow the classic touch-typing progression:
//  1. Home row, starting from the index-finger anchor keys (F/J, ب/ت), then
//     adding one finger pair at a time outward to the little fingers.
//  2. Top row and bottom row keys, introduced in pairs by finger, with the
//     most frequent letters first so real words become available early.
//  3. Review lessons after each group, then Shift (capitals / hamza forms),
//     punctuation and numbers, and a final review with full sentences.
// Each new-key lesson moves from isolated drills of the new keys, to mixed
// groups with previously learned keys, to real words that use only letters
// already taught. Accuracy is required to progress; speed earns bonus stars.
#pragma once
#include "base.h"
#include "keyboard.h"

namespace curriculum {

enum Kind : u8 { NewKeys, Review, Capitals, Final };

struct LessonDef
{
	Kind kind;
	const char *keys;  // UTF-8 characters introduced by this lesson
	u8 targetWpm;      // speed needed (with high accuracy) for the third star
};

const int kMaxLessons = 32;
const int kMaxLines = 6;
const int kMaxLineLen = 48;

int Count(Lang lang);
const LessonDef &Get(Lang lang, int lesson);

struct CharSet
{
	u32 cps[128];
	int n = 0;
	bool Has(u32 cp) const;
	void Add(u32 cp);
};

// Characters available in the given lesson (everything taught so far).
void Allowed(Lang lang, int lesson, CharSet *out);
// Characters introduced in the given lesson (empty for review lessons).
void NewChars(Lang lang, int lesson, CharSet *out);
// Last lesson before the first one that introduces a key off the home row.
int LastHomeRowLesson(Lang lang);
// Index of the last lesson that introduces letters (for the "all letters" badge).
int LastLetterLesson(Lang lang);

struct Exercise
{
	u32 text[kMaxLines][kMaxLineLen];
	u8 len[kMaxLines];
	int lines;
};

void Generate(Lang lang, int lesson, u32 seed, Exercise *out);

// Pool of practice items (single letters or words) for the balloon game.
struct GamePool
{
	const char *words[256];
	int wordCount;
	CharSet letters;
};
void BuildGamePool(Lang lang, int lessonsDone, GamePool *out);

} // namespace curriculum
