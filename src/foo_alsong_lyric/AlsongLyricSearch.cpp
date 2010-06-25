#include "stdafx.h"

#include "Lyric.h"
#include "Socket.h"
#include "AlsongLyricSearch.h"
#include "SoapHelper.h"

AlsongLyricSearchResult::AlsongLyricSearchResult(boost::shared_ptr<pugi::xml_document> data)
{
	m_Document = data;
	m_LyricNode = m_Document->first_element_by_path("soap:Envelope/soap:Body/GetResembleLyric2Response/GetResembleLyric2Result/ST_GET_RESEMBLELYRIC2_RETURN"); //TODO: Test
	m_LyricResultMap[-1] = AlsongLyric();
}

AlsongLyric &AlsongLyricSearchResult::Get()
{
	if(!m_LyricNode)
		return m_LyricResultMap.find(-1)->second; //invalid item

	AlsongLyric ret(m_LyricNode);
	m_LyricNode = m_LyricNode.next_sibling("ST_GET_RESEMBLELYRIC2_RETURN");
	m_LyricResultMap[ret.GetnInfo()] = ret;

	return m_LyricResultMap.find(ret.GetnInfo())->second;
}

AlsongLyric &AlsongLyricSearchResult::Get(int nInfo)
{
	if(m_LyricResultMap.find(nInfo) != m_LyricResultMap.end())
		return m_LyricResultMap.find(nInfo)->second;
	else
		return m_LyricResultMap.find(-1)->second;
}

AlsongLyricSearchResult AlsongLyricSearch::SearchLyric(const std::string &Artist, const std::string Title, int nPage)
{
	SoapHelper helper;
	helper.SetMethod("ns1:GetResembleLyric2");
	helper.AddParameter("ns1:strTitle", Title.c_str());
	helper.AddParameter("ns1:strArtistName", Artist.c_str());
	helper.AddParameter("ns1:nCurPage", boost::lexical_cast<std::string>(nPage).c_str());

	AlsongLyricSearchResult ret(helper.Execute());

	return ret;
}

int AlsongLyricSearch::SearchLyricGetCount(const std::string &Artist, const std::string &Title)
{
	SoapHelper helper;
	helper.SetMethod("ns1:GetResembleLyric2Count");
	helper.AddParameter("ns1:strTitle", Title.c_str());
	helper.AddParameter("ns1:strArtistName", Artist.c_str());

	return boost::lexical_cast<int>(helper.Execute()->first_element_by_path("soap:Envelope/soap:Body/GetResembleLyric2CountResponse/GetResembleLyric2CountResult/strResembleLyricCount").child_value()); //TODO: Test
}
