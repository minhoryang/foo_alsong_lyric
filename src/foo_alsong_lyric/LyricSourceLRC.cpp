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
		wpath += boost::lexical_cast<std::wstring>(track->get_subsong_index());
	wpath += L".lrc";
	return wpath;
}

boost::shared_ptr<Lyric> LyricSourceLRC::Get(const metadb_handle_ptr &track)
{
	std::wstring path = getSavePath(track);
	std::string lyric;

	HANDLE hf = CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if(hf == INVALID_HANDLE_VALUE)
		return boost::shared_ptr<Lyric>(new LRCLyric(lyric, path));
	DWORD size = SetFilePointer(hf, 0, 0, FILE_END);
	SetFilePointer(hf, 0, 0, FILE_BEGIN);
	char *data = new char[size];
	DWORD dwRead;
	ReadFile(hf, (void *)data, size, &dwRead, NULL);

	CloseHandle(hf);

	if(data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) //utf8 bom
	{
		lyric.assign(data + 3);
		delete [] data;
	}
	else if(data[0] == 0xFF && data[1] == 0xFE) //utf16 bom
	{
		lyric = EncodingFunc::ToUTF8((wchar_t *)data);
		delete [] data;
	}
	else
	{
		lyric.assign(data);
		delete [] data;
	}


	return boost::shared_ptr<Lyric>(new LRCLyric(lyric, path));
}

DWORD LyricSourceLRC::Save(const metadb_handle_ptr &track, Lyric &lyric)
{
	std::wstring path = getSavePath(track);

	HANDLE hf = CreateFile(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, NULL, NULL);
	if(hf == INVALID_HANDLE_VALUE)
		return 0;

	DWORD unused;
	for(std::vector<LyricLine>::const_iterator it = lyric.GetIteratorAt(0); !lyric.IsEndOfLyric(it); it ++)
	{
		std::stringstream str;
		str << "[" << std::setfill('0') << std::setw(2) << it->time / 60 / 100 << ":" << std::setw(2) << it->time / 100 % 100 << "." << std::setw(2) << it->time % 100 << "]" << it->lyric << "\r\n";
		WriteFile(hf, str.str().c_str(), str.str().size(), &unused, NULL);
	}

	CloseHandle(hf);

	return 1;
}