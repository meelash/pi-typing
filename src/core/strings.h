// User-interface text in English and Arabic.
#pragma once
#include "base.h"
#include "keyboard.h"

enum StrId {
	S_AppTitle,
	S_PlugKeyboard,
	S_WhoIsTyping,
	S_NewPlayer,
	S_TypeYourName,
	S_NameHint,
	S_PickColor,
	S_DeleteQuestion,
	S_DeleteHint,
	S_ChooseCourse,
	S_Lesson,
	S_NewKeys,
	S_Review,
	S_ReviewAll,
	S_Capitals,
	S_Final,
	S_PressStart,
	S_BalloonGame,
	S_Badges,
	S_Accuracy,
	S_Speed,
	S_Wpm,
	S_Points,
	S_Combo,
	S_BestCombo,
	S_Stars3,
	S_Stars2,
	S_Stars1,
	S_Stars0,
	S_ResultsHint,
	S_ResultsHintFail,
	S_LessonComplete,
	S_TryAgainTitle,
	S_Paused,
	S_PauseHint,
	S_UseOtherShift,
	S_CapsLockOn,
	S_GameOver,
	S_Score,
	S_Best,
	S_Level,
	S_NewRecord,
	S_GameIntroLetters,
	S_GameIntroWords,
	S_GameModeHint,
	S_GameLocked,
	S_StartLevel,
	S_BestWithLetters,
	S_LevelsBeaten,
	S_YourBestScore,
	S_TopScore,
	S_NewTopScore,
	S_Letters,
	S_Words,
	S_MapHint,
	S_ProfilesHint,
	S_CourseHint,
	S_TrickyKeys,
	S_NoMistakes,
	S_NewBadge,
	S_Use,
	S_HomeRowTip,
	S_ReviewIntro,
	S_CapitalsIntro,
	S_FinalIntro,
	S_Best2,
	S_Locked,
	S_SoundOn,
	S_SoundOff,
	S_BadgesHint,
	S_GameHint,
	S_EscPause,
	S_SpaceKey,
	S_NoStorage,
	S_Tip0,
	S_Tip1,
	S_Tip2,
	S_Tip3,
	S_Tip4,
	S_Tip5,
	S_Tip6,
	S_FingerLPinky,
	S_FingerLRing,
	S_FingerLMiddle,
	S_FingerLIndex,
	S_FingerThumb,
	S_FingerRIndex,
	S_FingerRMiddle,
	S_FingerRRing,
	S_FingerRPinky,
	S_Rank0,
	S_Rank1,
	S_Rank2,
	S_Rank3,
	S_Rank4,
	S_Rank5,
	S_Rank6,
	S_Rank7,
	S_NewRank,
	S_Count
};

const int kTipCount = 7;
const int kRankCount = 8;

const char *Str(StrId id, Lang lang);

// Stars needed for each rank.
extern const u16 kRankStars[kRankCount];

enum BadgeId {
	B_FirstLesson,
	B_HomeRow,
	B_Perfect,
	B_Combo50,
	B_Combo100,
	B_Speed20,
	B_Speed30,
	B_Stars30,
	B_AllLetters,
	B_Balloon100,
	B_Balloon300,
	B_Bilingual,
	B_Graduate,
	B_Count
};

struct BadgeText
{
	const char *symbol;  // short text drawn on the medal
	u32 color;
	const char *name[LangCount];
	const char *desc[LangCount];
};

extern const BadgeText kBadges[B_Count];
