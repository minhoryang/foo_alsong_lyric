#pragma once

#include "Lyric.h"

class AlsongLyric : public Lyric //alsong specific
{
private:
	int m_nInfoID;
	static DWORD GetFileHash(metadb_handle_ptr track, CHAR *Hash);
public:
	AlsongLyric() {}
	AlsongLyric(const pugi::xml_node &node);

	int GetnInfo()
	{
		return m_nInfoID;
	}

	static AlsongLyric LyricFromAlsong(const metadb_handle_ptr &track);
	static DWORD LyricToAlsong(metadb_handle_ptr track, const AlsongLyric &Lyric);
};