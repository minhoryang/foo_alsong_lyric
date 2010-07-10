#pragma once

#include "Lyric.h"

class LyricSearchResult
{
public:
	virtual Lyric &Get() = 0;
	virtual Lyric &Get(int id) = 0;
};