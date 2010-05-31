#include "stdafx.h"

#include "Lyric.h"
#include "SoapHelper.h"
#include "pugixml/pugixml.hpp"

Lyric::Lyric()
{

}

Lyric::Lyric(const char *raw)
{
	m_Lyric = raw;
	Split("\r\n");
}

Lyric::~Lyric()
{

}

Lyric Lyric::LyricFromFile(WCHAR *LoadFrom, CHAR *fmt)
{
	if(!StrCmpIA(fmt, "lrc"))
	{
		HANDLE hFile = CreateFile(LoadFrom, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
		if(hFile == INVALID_HANDLE_VALUE)
			return FALSE;

		DWORD dwRead;
		CHAR *tmp;
		DWORD Size = SetFilePointer(hFile, 0, 0, FILE_END);
		SetFilePointer(hFile, 0, 0, FILE_BEGIN);
		if(Size == 0)
			return FALSE;

		tmp = new CHAR[Size + 10];

		ReadFile(hFile, tmp, Size, &dwRead, NULL);
		if(dwRead != Size)
			return FALSE;

		CloseHandle(hFile);

		Lyric lyric((const char *)tmp);
		delete[] tmp;

		return lyric;
	}

	return Lyric();
}

void Lyric::SaveToFile(WCHAR *SaveTo, CHAR *fmt)
{
	if(m_Lyric.size() == 0)
		return;
	if(!StrCmpIA(fmt, "lrc"))
	{
		HANDLE hFile = CreateFile(SaveTo, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, NULL, NULL);
		if(hFile == INVALID_HANDLE_VALUE)
			return;
		unsigned int i;
		DWORD dwWritten;

		for(i = 0; i < m_Lyric.size(); i ++)
		{
			CHAR temp[255];
			wsprintfA(temp, "[%02d:%02d.%02d]", (int)(m_LyricLines[i].time / 100) / 60, ((int)m_LyricLines[i].time / 100) % 60, (int)(m_LyricLines[i].time % 100));
			WriteFile(hFile, (void *)temp, strlen(temp), &dwWritten, NULL);
			WriteFile(hFile, m_LyricLines[i].lyric.c_str(), m_LyricLines[i].lyric.length(), &dwWritten, NULL);
			WriteFile(hFile, "\r\n", 2, &dwWritten, NULL);
		}

		CloseHandle(hFile);
	}
	else if(!StrCmpIA(fmt, "txt"))
	{
		HANDLE hFile = CreateFile(SaveTo, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, NULL, NULL);
		unsigned int i;
		DWORD dwWritten;

		for(i = 0; i < m_Lyric.size(); i ++)
		{
			WriteFile(hFile, m_LyricLines[i].lyric.c_str(), m_LyricLines[i].lyric.length(), &dwWritten, NULL);
			WriteFile(hFile, "\r\n", 2, &dwWritten, NULL);
		}

		CloseHandle(hFile);
	}
}

DWORD Lyric::Split(const char *Delimiter)
{
	int i;

	m_LyricLines.clear();

	const char *nowpos = m_Lyric.c_str();
	const char *lastpos = nowpos;
	int pos;
	for(i = 0; ; i ++) //<br>ÀÚ¸£±â
	{
		pos = pfc::strstr_ex(nowpos + 1, lstrlenA(nowpos + 1), Delimiter, lstrlenA(Delimiter));
		if(pos == -1)
			break;
		nowpos = nowpos + 1 + pos;

		int time = StrToIntA(lastpos + 1) * 60 * 100 + StrToIntA(lastpos + 4) * 100 + StrToIntA(lastpos + 7);
		//lastpos += 10; //strlen("[34:56.78]");

		m_LyricLines.push_back(LyricLine(time, std::string(lastpos, nowpos - lastpos)));
		lastpos = nowpos + lstrlenA(Delimiter);
	}

	m_LyricIterator = m_LyricLines.begin();

	return S_OK;
}

void Lyric::Clear()
{
	m_Title.clear();
	m_Album.clear();
	m_Artist.clear();
	m_Registrant.clear();
	m_Lyric.clear();
	m_LyricLines.clear();
	m_LyricIterator = m_LyricLines.begin();
}

std::vector<LyricLine>::iterator Lyric::GetIteratorAt(unsigned int time)
{
	std::vector<LyricLine>::iterator it;
	for(it = m_LyricLines.begin(); it != m_LyricLines.end() && it->time < time; it ++);
	
	return it;
}

int Lyric::IsValidIterator(std::vector<LyricLine>::iterator it)
{
	try
	{
		return std::find(m_LyricLines.begin(), m_LyricLines.end(), *it) != m_LyricLines.end();
	}
	catch(...)
	{
		return false;
	}
}
