#include "stdafx.h"

#include "AlsongLyric.h"

AlsongLyric::AlsongLyric(const pugi::xml_node &node)
{
	if(!node.child("strInfoID").child_value())
	{ //getlyric
		m_Title = node.child("strTitle").child_value();
		m_Artist = node.child("strArtist").child_value();
		m_Album = node.child("strAlbum").child_value();
		m_Registrant = node.child("strRegisterFirstName").child_value();
		m_Lyric = node.child("strLyric").child_value();

		if(!m_Album.compare(m_Title))
			m_Album.clear();
		m_nInfoID = -1;
	}
	//search
	else
	{
		m_Album = node.child("strAlbumName").child_value();
		m_Title = node.child("strTitle").child_value();
		m_Artist = node.child("strArtistName").child_value();
		m_Registrant = node.child("strRegisterFirstName").child_value();
		m_Lyric = node.child("strLyric").child_value();
		m_nInfoID = boost::lexical_cast<int>(node.child("strInfoID").child_value());
	}

	Split("<br>");
}
