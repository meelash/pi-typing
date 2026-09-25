#include "usbinput.h"
#include <circle/devicenameservice.h>
#include <circle/logger.h>
#include <circle/string.h>
#include <circle/usb/usbhcidevice.h>
#include <circle/usb/usbkeyboard.h>

namespace usbinput {

// Several keyboards can be plugged in at once (e.g. a wired one and a
// wireless receiver); all of them type into the tutor.
static const unsigned kMaxKeyboards = 4;
static const unsigned kMaxKeyboardNumber = 8;  // look for ukbd1..ukbd8

struct TKeyboard
{
	CUSBKeyboardDevice *volatile pDevice;
	unsigned char PrevKeys[6];
};

static TKeyboard s_Keyboards[kMaxKeyboards];

// Key presses, filled by the USB interrupt handlers, drained by Poll().
static const unsigned kQueueSize = 64;
static volatile unsigned char s_Usage[kQueueSize], s_Mods[kQueueSize];
static volatile unsigned s_nHead = 0, s_nTail = 0;

static void Push(unsigned char ucUsage, unsigned char ucMods)
{
	unsigned nNext = (s_nHead + 1) % kQueueSize;
	if (nNext == s_nTail)
		return;  // full: drop the key
	s_Usage[s_nHead] = ucUsage;
	s_Mods[s_nHead] = ucMods;
	s_nHead = nNext;
}

// Called in interrupt context with every report of one keyboard.
static void KeyStatusHandler(unsigned char ucModifiers, const unsigned char RawKeys[6], void *pArg)
{
	TKeyboard *pKeyboard = (TKeyboard *)pArg;
	// 0x01..0x03 are error codes (e.g. too many keys pressed at once).
	for (unsigned i = 0; i < 6; i++)
		if (RawKeys[i] > 0 && RawKeys[i] < 4)
			return;
	for (unsigned i = 0; i < 6; i++) {
		unsigned char ucKey = RawKeys[i];
		if (!ucKey)
			continue;
		bool bNew = true;
		for (unsigned j = 0; j < 6; j++)
			if (pKeyboard->PrevKeys[j] == ucKey)
				bNew = false;
		if (bNew)
			Push(ucKey, ucModifiers);
	}
	for (unsigned i = 0; i < 6; i++)
		pKeyboard->PrevKeys[i] = RawKeys[i];
}

static void KeyboardRemoved(CDevice *pDevice, void *pContext)
{
	TKeyboard *pKeyboard = (TKeyboard *)pContext;
	pKeyboard->pDevice = 0;
	for (unsigned i = 0; i < 6; i++)
		pKeyboard->PrevKeys[i] = 0;
}

void Update(CUSBHCIDevice *pHCI)
{
	pHCI->UpdatePlugAndPlay();

	for (unsigned n = 1; n <= kMaxKeyboardNumber; n++) {
		CString Name;
		Name.Format("ukbd%u", n);
		CUSBKeyboardDevice *pDevice =
			(CUSBKeyboardDevice *) CDeviceNameService::Get()->GetDevice(Name, FALSE);
		if (!pDevice)
			continue;

		TKeyboard *pFree = 0;
		bool bKnown = false;
		for (unsigned i = 0; i < kMaxKeyboards; i++) {
			if (s_Keyboards[i].pDevice == pDevice)
				bKnown = true;
			else if (!s_Keyboards[i].pDevice && !pFree)
				pFree = &s_Keyboards[i];
		}
		if (bKnown || !pFree)
			continue;

		for (unsigned i = 0; i < 6; i++)
			pFree->PrevKeys[i] = 0;
		pFree->pDevice = pDevice;
		pDevice->RegisterRemovedHandler(KeyboardRemoved, pFree);
		pDevice->RegisterKeyStatusHandlerRaw(KeyStatusHandler, FALSE, pFree);
		CLogger::Get()->Write("input", LogNotice, "Keyboard %s ready", (const char *) Name);
	}
}

bool KeyboardPresent()
{
	for (unsigned i = 0; i < kMaxKeyboards; i++)
		if (s_Keyboards[i].pDevice)
			return true;
	return false;
}

bool Poll(unsigned char *pUsage, unsigned char *pModifiers)
{
	if (s_nTail == s_nHead)
		return false;
	*pUsage = s_Usage[s_nTail];
	*pModifiers = s_Mods[s_nTail];
	s_nTail = (s_nTail + 1) % kQueueSize;
	return true;
}

} // namespace usbinput
