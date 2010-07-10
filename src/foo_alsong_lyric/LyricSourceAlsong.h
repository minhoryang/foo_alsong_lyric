#pragma once

#include "LyricSource.h"

class LyricSourceAlsong : public LyricSource
{
private:
	DWORD GetFileHash(const metadb_handle_ptr &track, CHAR *Hash);
public:
	virtual Lyric Get(const metadb_handle_ptr &track);
	virtual DWORD Save(const metadb_handle_ptr &track, const Lyric &lyric);
};