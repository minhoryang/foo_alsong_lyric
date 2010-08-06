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

	virtual std::string GetName()
	{
		return std::string("Alsong Lyric");
	}

	virtual GUID GetGUID()
	{
		// {1953AB44-5AA4-48BE-AB4F-4BEA308F620D}
		static const GUID guid_alsong = 
		{ 0x1953ab44, 0x5aa4, 0x48be, { 0xab, 0x4f, 0x4b, 0xea, 0x30, 0x8f, 0x62, 0xd } };

		return guid_alsong;
	}
};
