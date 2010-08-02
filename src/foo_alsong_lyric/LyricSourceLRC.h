#pragma once

#include "LyricSource.h"

class LyricSourceLRC : public LyricSource
{
private:
	std::wstring getSavePath(const metadb_handle_ptr &track);
public:
	virtual boost::shared_ptr<Lyric> Get(const metadb_handle_ptr &track);
	virtual DWORD Save(const metadb_handle_ptr &track, Lyric &lyric);
	virtual int SearchLyricGetCount(const std::string &Artist, const std::string &Title)
	{
		return 0;//no search
	}
	virtual boost::shared_ptr<LyricSearchResult> SearchLyric(const std::string &Artist, const std::string Title, int nPage = 0)
	{
		return boost::shared_ptr<LyricSearchResult>(); //no search
	}
};