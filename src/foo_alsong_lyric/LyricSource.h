#pragma once

#include "Lyric.h"
#include "LyricSearchResult.h"

interface LyricSource
{
public:
	virtual boost::shared_ptr<Lyric> Get(const metadb_handle_ptr &track) = 0;
	virtual DWORD Save(const metadb_handle_ptr &track, Lyric &lyric) = 0;
	virtual int SearchLyricGetCount(const std::string &Artist, const std::string &Title) = 0;
	virtual boost::shared_ptr<LyricSearchResult> SearchLyric(const std::string &Artist, const std::string Title, int nPage = 0) = 0;
};