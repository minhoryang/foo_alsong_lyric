#pragma once

#include "LyricSearchResult.h"
#include "AlsongLyric.h"

class LyricSearchResultAlsong : public LyricSearchResult
{
private:
	boost::shared_ptr<pugi::xml_document> m_Document;
	pugi::xml_node m_LyricNode;
	std::map<int, AlsongLyric> m_LyricResultMap;
public:
	LyricSearchResultAlsong(boost::shared_ptr<pugi::xml_document> data);
	virtual Lyric *Get();
	virtual Lyric *Get(int id);
};