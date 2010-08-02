#include "stdafx.h"

#include "LyricSearchResultAlsong.h"

LyricSearchResultAlsong::LyricSearchResultAlsong(boost::shared_ptr<pugi::xml_document> data)
{
	m_Document = data;
	m_LyricNode = m_Document->first_element_by_path("soap:Envelope/soap:Body/GetResembleLyric2Response/GetResembleLyric2Result/ST_GET_RESEMBLELYRIC2_RETURN"); //TODO: Test
	m_LyricResultMap[-1] = AlsongLyric();
}

Lyric *LyricSearchResultAlsong::Get()
{
	if(!m_LyricNode)
		return &m_LyricResultMap.find(-1)->second; //invalid item

	AlsongLyric ret(m_LyricNode);
	m_LyricNode = m_LyricNode.next_sibling("ST_GET_RESEMBLELYRIC2_RETURN");
	m_LyricResultMap[ret.GetInternalID()] = ret;

	return dynamic_cast<Lyric *>(&m_LyricResultMap.find(ret.GetInternalID())->second);
}

Lyric *LyricSearchResultAlsong::Get(int id)
{
	if(m_LyricResultMap.find(id) != m_LyricResultMap.end())
		return dynamic_cast<Lyric *>(&m_LyricResultMap.find(id)->second);
	else
		return dynamic_cast<Lyric *>(&m_LyricResultMap.find(-1)->second);
}
