/*
* foo_alsong_lyric														
* Copyright (C) 2007-2010 Inseok Lee <dlunch@gmail.com>
*
* This library is free software; you can redistribute it and/or modify it 
* under the terms of the GNU Lesser General Public License as published 
* by the Free Software Foundation; version 2.1 of the License.
*
* This library is distributed in the hope that it will be useful, 
* but WITHOUT ANY WARRANTY; without even the implied warranty of 
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
* See the GNU Lesser General Public License for more details.
*
* You can receive a copy of the GNU Lesser General Public License from 
* http://www.gnu.org/
*/

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
	if(m_config["lrcsavepath"].length())
	{
		class LRCTitleFormatCallback : public main_thread_callback
		{
		private:
			HANDLE m_event;
			std::string *m_out;
			const std::string &m_format;
			const metadb_handle_ptr &m_track;
		public:
			LRCTitleFormatCallback(HANDLE event, std::string *out, const std::string &format, const metadb_handle_ptr &track) : m_event(event), m_format(format), m_out(out), m_track(track) {}

			virtual void callback_run()
			{
				core_api::ensure_main_thread();
				pfc::string8 out;
				service_ptr_t<titleformat_object> script;
				static_api_ptr_t<titleformat_compiler>()->compile(script, m_format.c_str());
				m_track->format_title(NULL, out, script, NULL);
				m_out->assign(out.get_ptr());
				SetEvent(m_event);
			}
		};
		wpath = wpath.substr(wpath.find_last_of(L"\\") + 1);
		HANDLE event = CreateEvent(NULL, TRUE, FALSE, NULL);
		std::string out;
		service_ptr_t<LRCTitleFormatCallback> p_callback = new service_impl_t<LRCTitleFormatCallback>(event, &out, m_config["lrcsavepath"], track);
		static_api_ptr_t<main_thread_callback_manager>()->add_callback(p_callback);
		WaitForSingleObject(event, INFINITE);
		CloseHandle(event);
	}
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
		str << "[" << std::setfill('0') << std::setw(2) << it->time / 60 / 100 << ":" << std::setw(2) << it->time / 100 % 60 << "." << std::setw(2) << it->time % 100 << "]" << it->lyric << "\r\n";
		WriteFile(hf, str.str().c_str(), str.str().size(), &unused, NULL);
	}

	CloseHandle(hf);

	return 1;
}

std::map<std::string, std::pair<LyricSource::ConfigItemType, std::vector<std::string> > > LyricSourceLRC::GetConfigItems(int type)
{
	std::map<std::string, std::pair<ConfigItemType, std::vector<std::string> > > ret;
	ret["lrcsavepath"] = std::make_pair(ITEM_TYPE_STRING, std::vector<std::string>(1, EncodingFunc::ToUTF8(L"LRC 저장 위치")));

	return ret;
}

LyricSourceFactory<LyricSourceLRC> LyricSourceLRCFactory;
