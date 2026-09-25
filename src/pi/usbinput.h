// USB keyboard input in raw HID mode. Kept in its own translation unit
// because Circle's keyboard headers define names that clash with the core.
#pragma once

class CUSBHCIDevice;

namespace usbinput {

// Call from the main loop (task level): handles keyboard plug/unplug.
void Update(CUSBHCIDevice *pHCI);

bool KeyboardPresent();

// Next key press (HID usage code and modifier byte); false if none queued.
bool Poll(unsigned char *pUsage, unsigned char *pModifiers);

} // namespace usbinput
