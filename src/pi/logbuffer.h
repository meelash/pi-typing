// Keeps the most recent log lines in memory so they can be shown on screen
// when something goes wrong (there is no console on this system).
#pragma once
#include <circle/device.h>
#include <circle/types.h>

class CLogBuffer : public CDevice
{
public:
	CLogBuffer(void);

	int Write(const void *pBuffer, size_t nCount) override;

	// Completed lines, oldest first; returns how many were stored in ppLines.
	int GetLines(const char **ppLines, int nMax);

private:
	static const int kLines = 300;
	static const int kColumns = 110;

	char m_Lines[kLines][kColumns + 1];
	int m_nNext;   // line currently being written
	int m_nColumn;
	int m_nCount;  // completed lines stored
	boolean m_bEscape;
};
