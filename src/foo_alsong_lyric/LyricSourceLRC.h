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

#pragma once

#include "LyricSource.h"

class LyricSourceLRC : public LyricSource
{
private:
	std::wstring getSavePath(const metadb_handle_ptr &track);
	std::map<std::string, std::string> m_config;
public:
	virtual boost::shared_ptr<Lyric> Get(const metadb_handle_ptr &track);
	virtual DWORD Save(const metadb_handle_ptr &track, Lyric &lyric);
	virtual int SearchLyricGetCount(const std::string &Artist, const std::string &Title)
	{
		return 0;//no search
	}
	virtual boost::shared_ptr<LyricSearchResult> SearchLyric(const std::string &Artist, const std::string Title, int nPage = 0)
	{
		return boost::shared_ptr<LyricSearchResult>(); //no search
	}

	virtual std::string GetName()
	{
		return std::string("LRC Lyric");
	}

	virtual GUID GetGUID()
	{
		// {544A02C2-AC0F-434F-85E8-A9427268E75B}
		static const GUID guid_lrc = 
		{ 0x544a02c2, 0xac0f, 0x434f, { 0x85, 0xe8, 0xa9, 0x42, 0x72, 0x68, 0xe7, 0x5b } };

		return guid_lrc;
	}

	virtual std::map<std::string, std::string> GetConfig()
	{
		return m_config;
	}

	virtual void SetConfig(const std::map<std::string, std::string> &setting)
	{
		m_config = setting;
	}

	virtual std::map<std::string, std::pair<ConfigItemType, std::vector<std::string> > > GetConfigItems(int type);
};