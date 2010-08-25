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

class LyricSourceAlsong : public LyricSource
{
private:
	DWORD GetFileHash(const metadb_handle_ptr &track, CHAR *Hash);
	std::map<std::string, std::string> m_config;
public:
	virtual boost::shared_ptr<Lyric> Get(const metadb_handle_ptr &track);
	virtual DWORD Save(const metadb_handle_ptr &track, Lyric &lyric);
	virtual int SearchLyricGetCount(const std::string &Artist, const std::string &Title);
	virtual boost::shared_ptr<LyricSearchResult> SearchLyric(const std::string &Artist, const std::string Title, int nPage = 0);

	virtual std::string GetName()
	{
		return std::string("Alsong Lyric");
	}

	virtual GUID GetGUID()
	{
		// {1953AB44-5AA4-48BE-AB4F-4BEA308F620D}
		static const GUID guid_alsong = 
		{ 0x1953ab44, 0x5aa4, 0x48be, { 0xab, 0x4f, 0x4b, 0xea, 0x30, 0x8f, 0x62, 0xd } };

		return guid_alsong;
	}


	virtual std::map<std::string, std::string> GetConfig()
	{
		return m_config;
	}

	virtual void SetConfig(const std::map<std::string, std::string> &setting)
	{
		m_config = setting;
	}

	virtual std::map<std::string, ConfigItemType> GetConfigItems(int type);
	virtual std::string GetConfigDescription(std::string item);
	virtual std::string GetConfigLabel(std::string item);
	virtual std::vector<std::string> GetConfigEnumeration(std::string item);
	virtual std::string IsConfigValid(std::map<std::string, std::string>);
};
