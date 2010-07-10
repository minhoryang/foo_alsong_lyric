#pragma once

#include "Lyric.h"

class AlsongLyric : public Lyric
{
private:
	int m_nInfoID;
public:
	AlsongLyric(const pugi::xml_node &node);
	AlsongLyric() {}
	int GetInternalID() const
	{
		return m_nInfoID;
	}
};