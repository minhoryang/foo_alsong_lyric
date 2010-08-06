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

	virtual std::string GetName()
	{
		return std::string("LRC Lyric");
	}

	virtual GUID GetGUID()
	{
		// {544A02C2-AC0F-434F-85E8-A9427268E75B}
		static const GUID guid_lrc = 
		{ 0x544a02c2, 0xac0f, 0x434f, { 0x85, 0xe8, 0xa9, 0x42, 0x72, 0x68, 0xe7, 0x5b } };

		return guid_lrc;
	}
};