// Tiny synthesiser for the game's sound effects. Trigger() is called from
// the main loop; Render() from the audio interrupt. It only uses integer
// arithmetic so it is safe in interrupt context.
#pragma once
#include "base.h"

enum SfxId { SfxKey, SfxError, SfxCombo, SfxStar, SfxFanfare, SfxPop, SfxLose, SfxMove, SfxSelect,
	     SfxLevelUp, SfxBadge, SfxCount };

namespace sfx {

void Init(int sampleRate);
void SetEnabled(bool on);
bool Enabled();
void Trigger(SfxId id);
// Fills mono 16-bit samples.
void Render(s16 *out, int frames);

} // namespace sfx
