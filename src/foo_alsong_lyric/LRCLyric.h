#pragma once

#include "Lyric.h"
#include "EncodingFunc.h"

class LRCLyric : public Lyric
{
private:
	wchar_t m_filename[255];
public:
	LRCLyric(std::string rawlyric, std::wstring filename)
	{
		m_Lyric.assign(rawlyric);
		Split("\r\n");

		lstrcpy(m_filename, filename.c_str());
	}

	virtual int GetInternalID() const
	{
		return (int)m_filename;
	}
};