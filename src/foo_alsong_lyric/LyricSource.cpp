#include "stdafx.h"

#include "LyricSource.h"

std::map<GUID, boost::shared_ptr<LyricSource> > LyricSourceManager::m_lyricSources;