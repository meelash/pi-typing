#include "kernel.h"
#include "usbinput.h"
#include "../core/sfx.h"
#include <circle/sound/hdmisoundbasedevice.h>
#include <circle/sound/pwmsoundbasedevice.h>
#include <circle/bcmpropertytags.h>
#include <circle/string.h>
#include <circle/util.h>

// The debug build (make debug) logs USB details at debug level.
#ifndef DIAG_LOG_LEVEL
#define DIAG_LOG_LEVEL LogNotice
#endif

static const unsigned kSampleRate = 48000;
static const unsigned kSoundChunk = 384 * 4;       // HDMI needs a multiple of 384
static const unsigned kSoundQueueMs = 100;
static const unsigned kSoundAheadFrames = kSampleRate * 45 / 1000;  // ~45 ms latency

CKernel *CKernel::s_pThis = 0;

CKernel::CKernel(void)
:	m_Timer(&m_Interrupt),
	m_Logger(DIAG_LOG_LEVEL, &m_Timer),
	m_USBHCI(&m_Interrupt, &m_Timer, TRUE),  // TRUE: plug-and-play
	m_EMMC(&m_Interrupt, &m_Timer, &m_ActLED),
	m_bStorageOK(FALSE),
	m_pFrameBuffer(0),
	m_bDoubleBuffered(FALSE),
	m_bSwapRedBlue(FALSE),
	m_nBackBuffer(0),
	m_pCanvas(0),
	m_pSound(0),
	m_nStatus(0)
{
	s_pThis = this;
}

boolean CKernel::Initialize(void)
{
	// There is no console: log messages are kept in memory and shown on
	// screen only when no keyboard is found or the system stops.
	if (!m_Logger.Initialize(&m_LogBuffer) || !m_Interrupt.Initialize() || !m_Timer.Initialize())
		return FALSE;
	m_Logger.RegisterPanicHandler(PanicHandler);

	// Fixed 1280x720 canvas; the GPU scales it to the connected display.
	// A virtual height of two screens allows flipping between buffers.
	m_pFrameBuffer = new CBcmFrameBuffer(App::W, App::H, 32, App::W, App::H * 2);
	if (m_pFrameBuffer->Initialize())
		m_bDoubleBuffered = TRUE;
	else {
		delete m_pFrameBuffer;
		m_pFrameBuffer = new CBcmFrameBuffer(App::W, App::H, 32);
		if (!m_pFrameBuffer->Initialize())
			return FALSE;
	}
	m_pCanvas = new u32[App::W * App::H];

	// The canvas is 0xAARRGGBB (BGR byte order). Ask the firmware which order the
	// framebuffer uses, and swap red and blue on output if it is RGB.
	static const u32 kGetPixelOrder = 0x00040006;
	CBcmPropertyTags Tags;
	TPropertyTagSimple PixelOrder;
	if (Tags.GetTag(kGetPixelOrder, &PixelOrder, sizeof PixelOrder))
		m_bSwapRedBlue = PixelOrder.nValue == 1;

	AddStatus(m_bDoubleBuffered ? "Screen: 1280x720, double-buffered" : "Screen: 1280x720");

	if (!m_USBHCI.Initialize()) {
		AddStatus("USB host controller: FAILED to start");
		return FALSE;
	}
	AddStatus("USB host controller: started");

	// Progress is optional: the tutor still runs if the card is read-only.
	if (m_EMMC.Initialize() && f_mount(&m_FileSystem, "SD:", 1) == FR_OK)
		m_bStorageOK = TRUE;
	AddStatus(m_bStorageOK ? "SD card: mounted" : "SD card: not available");

	return TRUE;
}

void CKernel::AddStatus(const char *pLine)
{
	if (m_nStatus < 6)
		strncpy(m_Status[m_nStatus++], pLine, sizeof m_Status[0] - 1);
}

int CKernel::Diagnostics(const char **lines, int max)
{
	int n = 0;
	for (int i = 0; i < m_nStatus && n < max; i++)
		lines[n++] = m_Status[i];
	if (n < max)
		lines[n++] = "--- log ---";
	return n + m_LogBuffer.GetLines(lines + n, max - n);
}

// Called when the system stops on a fatal error: show the log instead of a frozen screen.
void CKernel::PanicHandler(void)
{
	CKernel *k = s_pThis;
	if (!k || !k->m_pCanvas || !k->m_pFrameBuffer)
		return;
	Canvas c(k->m_pCanvas, App::W, App::H);
	c.Clear(0xFF1E2336);
	c.Text("The typing tutor stopped because of an error. Please take a photo of this screen.",
	       font::Small, 40, 50, 0xFFFF8080);
	static const char *lines[48];
	int n = k->Diagnostics(lines, 48);
	int first = n > 25 ? n - 25 : 0;
	for (int i = first; i < n; i++)
		c.Text(lines[i], font::Small, 40, 90 + (i - first) * 25, 0xFFE8EAF2);
	k->m_bDoubleBuffered = FALSE;  // no vsync wait here
	k->m_pFrameBuffer->SetVirtualOffset(0, 0);
	k->m_nBackBuffer = 0;
	k->Present();
}

u32 CKernel::Millis() { return (u32)(CTimer::GetClockTicks64() / 1000); }

u32 CKernel::Random() { return m_Random.GetNumber(); }

bool CKernel::KeyboardPresent() { return usbinput::KeyboardPresent(); }

bool CKernel::StorageAvailable() { return m_bStorageOK; }

// Saving is crash-safe: write "<name>.new", keep the previous file as
// "<name>.bak", then rename. Loading falls back through the same names.
bool CKernel::LoadFile(const char *name, char *buf, int cap, int *len)
{
	if (!m_bStorageOK)
		return false;
	static const char *const suffix[] = {"", ".new", ".bak"};
	for (unsigned i = 0; i < 3; i++) {
		CString path;
		path.Format("SD:/%s%s", name, suffix[i]);
		FIL File;
		if (f_open(&File, path, FA_READ | FA_OPEN_EXISTING) != FR_OK)
			continue;
		UINT nRead = 0;
		FRESULT res = f_read(&File, buf, (UINT)cap, &nRead);
		f_close(&File);
		if (res == FR_OK && nRead > 0) {
			*len = (int)nRead;
			return true;
		}
	}
	return false;
}

bool CKernel::SaveFile(const char *name, const char *buf, int len)
{
	if (!m_bStorageOK)
		return false;
	CString path, tmp, bak;
	path.Format("SD:/%s", name);
	tmp.Format("SD:/%s.new", name);
	bak.Format("SD:/%s.bak", name);
	FIL File;
	bool bOK = f_open(&File, tmp, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK;
	if (bOK) {
		UINT nWritten = 0;
		FRESULT res = f_write(&File, buf, (UINT)len, &nWritten);
		bOK = f_close(&File) == FR_OK && res == FR_OK && nWritten == (UINT)len;
	}
	if (bOK) {
		f_unlink(bak);
		f_rename(path, bak);  // fails harmlessly on the first save
		bOK = f_rename(tmp, path) == FR_OK;
	}
	if (!bOK)
		m_bStorageOK = FALSE;  // show the warning on the players screen
	return bOK;
}

void CKernel::StartSound(void)
{
	// cmdline.txt: sounddev=sndhdmi (default), sndpwm (headphone jack) or none
	const char *pDevice = m_Options.GetSoundDevice();
	if (strcmp(pDevice, "none") == 0) {
		AddStatus("Sound: off");
		return;
	}
	if (strcmp(pDevice, "sndpwm") == 0)
		m_pSound = new CPWMSoundBaseDevice(&m_Interrupt, kSampleRate, kSoundChunk);
	else
		m_pSound = new CHDMISoundBaseDevice(&m_Interrupt, kSampleRate, kSoundChunk);
	sfx::Init(kSampleRate);
	if (!m_pSound->AllocateQueue(kSoundQueueMs)) {
		delete m_pSound;
		m_pSound = 0;
		return;
	}
	m_pSound->SetWriteFormat(SoundFormatSigned16, 1);
	PumpSound();
	if (!m_pSound->Start()) {
		delete m_pSound;
		m_pSound = 0;
		AddStatus("Sound: could not start");
		return;
	}
	AddStatus(strcmp(pDevice, "sndpwm") == 0 ? "Sound: headphone jack" : "Sound: HDMI");
}

void CKernel::PumpSound(void)
{
	if (!m_pSound)
		return;
	static s16 Samples[512];
	unsigned nQueued = m_pSound->GetQueueFramesAvail();
	while (nQueued < kSoundAheadFrames) {
		unsigned n = kSoundAheadFrames - nQueued;
		if (n > 512)
			n = 512;
		sfx::Render(Samples, (int)n);
		m_pSound->Write(Samples, n * sizeof(s16));
		nQueued += n;
	}
}

void CKernel::Present(void)
{
	u8 *pBase = (u8 *)(uintptr)m_pFrameBuffer->GetBuffer();
	unsigned nPitch = m_pFrameBuffer->GetPitch();
	unsigned nOffsetY = m_bDoubleBuffered && m_nBackBuffer ? App::H : 0;
	for (unsigned y = 0; y < (unsigned)App::H; y++) {
		u32 *pDest = (u32 *)(pBase + (nOffsetY + y) * nPitch);
		const u32 *pSrc = m_pCanvas + y * App::W;
		if (!m_bSwapRedBlue) {
			memcpy(pDest, pSrc, App::W * 4);
			continue;
		}
		for (unsigned x = 0; x < (unsigned)App::W; x++) {
			u32 c = pSrc[x];
			pDest[x] = (c & 0xFF00FF00) | ((c >> 16) & 0xFF) | ((c & 0xFF) << 16);
		}
	}
	if (m_bDoubleBuffered) {
		m_pFrameBuffer->SetVirtualOffset(0, nOffsetY);
		m_pFrameBuffer->WaitForVerticalSync();
		m_nBackBuffer ^= 1;
	}
}

void CKernel::Run(void)
{
	static App app(this);
	app.Init();
	Canvas canvas(m_pCanvas, App::W, App::H);
	StartSound();

	while (true) {
		usbinput::Update(&m_USBHCI);
		unsigned char ucUsage, ucMods;
		while (usbinput::Poll(&ucUsage, &ucMods))
			app.OnKey(KeyEvent{ucUsage, ucMods});
		app.Update();
		if (app.Draw(canvas))
			Present();
		else
			m_Timer.MsDelay(5);
		PumpSound();
	}
}
