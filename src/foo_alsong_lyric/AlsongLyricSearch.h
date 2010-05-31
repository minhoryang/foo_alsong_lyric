#pragma once

#include "Lyric.h"
#include "AlsongLyric.h"

class AlsongLyricSearchResult
{
private:
	boost::shared_ptr<pugi::xml_document> m_Document;
	pugi::xml_node m_LyricNode;
	std::map<int, AlsongLyric> m_LyricResultMap;
public:
	AlsongLyricSearchResult() {}
	AlsongLyricSearchResult(boost::shared_ptr<pugi::xml_document> data);
	AlsongLyric &Get();
	AlsongLyric &Get(int nInfo);
};

class AlsongLyricSearch
{
public:
	static int SearchLyricGetCount(const std::string &Artist, const std::string &Title);
	static AlsongLyricSearchResult SearchLyric(const std::string &Artist, const std::string Title, int nPage = 0);
};