#include "stdafx.h"

#include "LyricSourceLRC.h"
#include "EncodingFunc.h"
#include "LRCLyric.h"

std::wstring LyricSourceLRC::getSavePath(const metadb_handle_ptr &track)
{
	std::string path = track->get_path();
	if(path.find_first_of("file://") == std::string::npos)
		return std::wstring(L"");

	std::wstring wpath = EncodingFunc::ToUTF16(path.substr(7));
	wpath = wpath.substr(0, wpath.find_last_of(L"."));
	if(track->get_subsong_index() != 0)
		wpath += (track->get_subsong_index() + L'0');
	wpath += L".lrc";
	return wpath;
}

boost::shared_ptr<Lyric> LyricSourceLRC::Get(const metadb_handle_ptr &track)
{
	std::wstring path = getSavePath(track);

	return boost::shared_ptr<Lyric>(new LRCLyric(path.c_str()));
}

DWORD LyricSourceLRC::Save(const metadb_handle_ptr &track, Lyric &lyric)
{
	//stub
	return 0;
}