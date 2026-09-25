// Practice vocabulary and sentences for each course language.
#pragma once
#include "base.h"
#include "keyboard.h"

namespace content {

const char *const *WordsFor(Lang lang, int *count);
const char *const *SentencesFor(Lang lang, int *count);

} // namespace content
