// Bare-metal kernel: brings up only the hardware the typing tutor needs
// (HDMI framebuffer, USB keyboard, SD card, sound) and runs the app.
// There is no operating system, no network stack and no wireless driver.
#pragma once
#include "../core/app.h"
#include "logbuffer.h"
#include <circle/actled.h>
#include <circle/bcmframebuffer.h>
#include <circle/bcmrandom.h>
#include <circle/devicenameservice.h>
#include <circle/exceptionhandler.h>
#include <circle/interrupt.h>
#include <circle/koptions.h>
#include <circle/logger.h>
#include <circle/sound/soundbasedevice.h>
#include <circle/timer.h>
#include <circle/types.h>
#include <circle/usb/usbhcidevice.h>
#include <SDCard/emmc.h>
#include <fatfs/ff.h>

class CKernel : public Platform
{
public:
	CKernel(void);

	boolean Initialize(void);
	void Run(void);

	// Platform
	u32 Millis() override;
	u32 Random() override;
	bool LoadFile(const char *name, char *buf, int cap, int *len) override;
	bool SaveFile(const char *name, const char *buf, int len) override;
	bool KeyboardPresent() override;
	bool StorageAvailable() override;
	int Diagnostics(const char **lines, int max) override;

private:
	void StartSound(void);
	void PumpSound(void);
	void Present(void);
	void AddStatus(const char *pLine);
	static void PanicHandler(void);

	// do not change this order
	CActLED m_ActLED;
	CKernelOptions m_Options;
	CDeviceNameService m_DeviceNameService;
	CLogBuffer m_LogBuffer;
	CExceptionHandler m_ExceptionHandler;
	CInterruptSystem m_Interrupt;
	CTimer m_Timer;
	CLogger m_Logger;
	CUSBHCIDevice m_USBHCI;
	CEMMCDevice m_EMMC;
	FATFS m_FileSystem;
	boolean m_bStorageOK;
	CBcmRandomNumberGenerator m_Random;

	CBcmFrameBuffer *m_pFrameBuffer;
	boolean m_bDoubleBuffered;
	boolean m_bSwapRedBlue;
	unsigned m_nBackBuffer;
	u32 *m_pCanvas;

	CSoundBaseDevice *m_pSound;

	char m_Status[6][80];
	int m_nStatus;

	static CKernel *s_pThis;
};
