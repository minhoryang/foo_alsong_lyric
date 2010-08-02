#pragma once

#include "Lyric.h"
#include "EncodingFunc.h"

class LRCLyric : public Lyric
{
private:
	wchar_t m_filename[255];
public:
	LRCLyric(const wchar_t *filename)
	{
		HANDLE hf = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_ALWAYS, NULL, NULL);
		if(hf == INVALID_HANDLE_VALUE)
			return;
		DWORD size = SetFilePointer(hf, 0, 0, FILE_END);
		SetFilePointer(hf, 0, 0, FILE_BEGIN);
		char *data = new char[size];
		DWORD dwRead;
		ReadFile(hf, (void *)data, size, &dwRead, NULL);

		CloseHandle(hf);

		if(data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) //utf8 bom
		{
			m_Lyric.assign(data + 3);
			delete [] data;
		}
		else if(data[0] == 0xFF && data[1] == 0xFE) //utf16 bom
		{
			m_Lyric = EncodingFunc::ToUTF8((wchar_t *)data);
			delete [] data;
		}
		else
		{
			m_Lyric.assign(data);
			delete [] data;
		}

		Split("\r\n");

		lstrcpy(m_filename, filename);
	}

	virtual int GetInternalID() const
	{
		return (int)m_filename;
	}
};