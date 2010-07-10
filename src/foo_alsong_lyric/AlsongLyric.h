#pragma once

#include "Lyric.h"

class AlsongLyric : public Lyric //alsong specific
{
private:
	int m_nInfoID;
public:
	AlsongLyric() {}
	AlsongLyric(const pugi::xml_node &node);

	int GetnInfo() const
	{
		return m_nInfoID;
	}
};