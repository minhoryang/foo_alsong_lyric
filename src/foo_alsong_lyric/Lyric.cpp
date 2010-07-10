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

std::vector<LyricLine>::const_iterator Lyric::GetIteratorAt(unsigned int time) const
{
	std::vector<LyricLine>::const_iterator it;
	for(it = m_LyricLines.begin(); it != m_LyricLines.end() && it->time < time; it ++);
	
	return it;
}

int Lyric::IsValidIterator(std::vector<LyricLine>::const_iterator it) const
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
