#pragma once

#include "Lyric.h"

interface LyricSource
{
public:
	virtual Lyric Get(const metadb_handle_ptr &track) = 0;
	virtual DWORD Save(const metadb_handle_ptr &track, const Lyric &Lyric) = 0;
};