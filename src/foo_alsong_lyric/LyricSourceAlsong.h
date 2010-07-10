#pragma once

#include "LyricSource.h"

class LyricSourceAlsong : public LyricSource
{
private:
	DWORD GetFileHash(const metadb_handle_ptr &track, CHAR *Hash);
public:
	virtual boost::shared_ptr<Lyric> Get(const metadb_handle_ptr &track);
	virtual DWORD Save(const metadb_handle_ptr &track, Lyric &lyric);
	virtual int SearchLyricGetCount(const std::string &Artist, const std::string &Title);
	virtual boost::shared_ptr<LyricSearchResult> SearchLyric(const std::string &Artist, const std::string Title, int nPage = 0);
};
