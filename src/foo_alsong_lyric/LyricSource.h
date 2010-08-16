#pragma once

#include "Lyric.h"
#include "LyricSearchResult.h"

interface LyricSource
{
public:
	virtual boost::shared_ptr<Lyric> Get(const metadb_handle_ptr &track) = 0;
	virtual DWORD Save(const metadb_handle_ptr &track, Lyric &lyric) = 0;
	virtual int SearchLyricGetCount(const std::string &Artist, const std::string &Title) = 0;
	virtual boost::shared_ptr<LyricSearchResult> SearchLyric(const std::string &Artist, const std::string Title, int nPage = 0) = 0;

	virtual std::string GetName() = 0;
	virtual GUID GetGUID() = 0;

	virtual std::map<std::string, std::string> GetConfig() = 0;
	virtual void SetConfig(const std::map<std::string, std::string> &setting) = 0;

	enum ConfigItemType
	{
		ITEM_TYPE_STRING,
		ITEM_TYPE_ENUM,
	};

	virtual std::map<std::string, std::pair<ConfigItemType, std::vector<std::string> > > GetConfigItems(int type) = 0; //type 1:get, type 2:save
};

class LyricSourceManager
{
private:
	static std::map<GUID, boost::shared_ptr<LyricSource> > m_lyricSources;
public:
	static void Add(boost::shared_ptr<LyricSource> source)
	{
		m_lyricSources[source->GetGUID()] = source;
	}

	static boost::shared_ptr<LyricSource> Get(GUID guid)
	{
		if(m_lyricSources.find(guid) != m_lyricSources.end())
			return m_lyricSources[guid];

		return boost::shared_ptr<LyricSource>();
	}

	static std::vector<boost::shared_ptr<LyricSource> > List()
	{
		std::vector<boost::shared_ptr<LyricSource> > ret;
		for(std::map<GUID, boost::shared_ptr<LyricSource> >::iterator it = m_lyricSources.begin(); it != m_lyricSources.end(); it ++)
			ret.push_back(it->second);

		return ret;
	}
};

template<typename T>
class LyricSourceFactory
{
public:
	LyricSourceFactory()
	{
		LyricSourceManager::Add(boost::shared_ptr<T>(new T()));
	}
};
