#include "logbuffer.h"

CLogBuffer::CLogBuffer(void) : m_nNext(0), m_nColumn(0), m_nCount(0), m_bEscape(FALSE)
{
	m_Lines[0][0] = '\0';
}

int CLogBuffer::Write(const void *pBuffer, size_t nCount)
{
	const char *p = (const char *)pBuffer;
	for (size_t i = 0; i < nCount; i++) {
		char c = p[i];
		// Drop the colour escape sequences the logger emits ("\x1b[...m").
		if (m_bEscape) {
			if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
				m_bEscape = FALSE;
			continue;
		}
		if (c == '\x1b') {
			m_bEscape = TRUE;
			continue;
		}
		if (c == '\n') {
			m_Lines[m_nNext][m_nColumn] = '\0';
			m_nNext = (m_nNext + 1) % kLines;
			m_nColumn = 0;
			m_Lines[m_nNext][0] = '\0';
			if (m_nCount < kLines - 1)
				m_nCount++;
			continue;
		}
		if (c >= ' ' && c < 0x7F && m_nColumn < kColumns)
			m_Lines[m_nNext][m_nColumn++] = c;
	}
	return (int)nCount;
}

int CLogBuffer::GetLines(const char **ppLines, int nMax)
{
	int n = m_nCount < nMax ? m_nCount : nMax;
	int nFirst = (m_nNext - n + kLines) % kLines;
	for (int i = 0; i < n; i++)
		ppLines[i] = m_Lines[(nFirst + i) % kLines];
	return n;
}
