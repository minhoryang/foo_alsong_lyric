#pragma once

#include "pugixml/pugixml.hpp"
#include <string>

struct LyricLine
{
	LyricLine(DWORD _time, const std::string &_lyric) : time(_time), lyric(_lyric) {}
	LyricLine() : time(0), lyric("") {}
	DWORD time;
	std::string lyric;

	int operator ==(const LyricLine &line) const
	{
		return (lyric == line.lyric) && (time == line.time);
	}
};

class Lyric
{
protected:
	std::string m_Title;
	std::string m_Album;
	std::string m_Artist;
	std::string m_Registrant;
	std::string m_Lyric;
	
	std::vector<LyricLine> m_LyricLines;
	std::vector<LyricLine>::iterator m_LyricIterator;

	DWORD Split(const char *Delimiter);
public:
	Lyric();
	Lyric(const char *raw);
	~Lyric();

	void Clear();
	void SaveToFile(WCHAR *SaveTo, CHAR *fmt);
	
	std::vector<LyricLine>::iterator GetIteratorAt(unsigned int time); //returns lyric right after time.
	
	std::string GetRawLyric()
	{
		return m_Lyric;
	}

	std::string GetTitle()
	{
		return m_Title;
	}

	std::string GetAlbum()
	{
		return m_Album;
	}

	std::string GetArtist()
	{
		return m_Artist;
	}

	std::string GetRegistrant()
	{
		return m_Registrant;
	}

	int IsEndOfLyric(std::vector<LyricLine>::iterator it)
	{
		return it == m_LyricLines.end();
	}

	int IsBeginOfLyric(std::vector<LyricLine>::iterator it)
	{
		return it == m_LyricLines.begin();
	}

	int HasLyric()
	{
		return m_LyricLines.size() != 0;
	}

	int IsValidIterator(std::vector<LyricLine>::iterator);

	static Lyric LyricFromFile(WCHAR *LoadFrom, CHAR *fmt);
};
