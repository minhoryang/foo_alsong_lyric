#include "stdafx.h"

#include "LyricManager.h"
#include "Lyric.h"
#include "AlsongLyric.h"
#include "LyricSourceAlsong.h"

//TODO: USLT 태그(4바이트 타임스탬프, 1바이트 길이, 문자열(유니코드), 0x08 순으로 들어있음)

LyricManager *LyricManagerInstance = NULL;

LyricManager::LyricManager() : m_Seconds(0)
{
	static_api_ptr_t<play_callback_manager> pcm;
	pcm->register_callback(this, flag_on_playback_all, false);
}

LyricManager::~LyricManager()
{
	if(m_fetchthread)
	{
		m_fetchthread->interrupt();
		m_fetchthread->join();
		m_fetchthread.reset();
	}
	if(m_countthread)
	{
		m_countthread->interrupt();
		m_countthread->join();
		m_countthread.reset();
	}
	static_api_ptr_t<play_callback_manager> pcm;
	pcm->unregister_callback(this);
}

void LyricManager::on_playback_seek(double p_time)
{
	if(m_countthread)
	{
		m_countthread->interrupt();
		m_countthread->join();
		m_countthread.reset();
	
		m_SecondLock.lock();
		m_Seconds = (int)p_time;
		m_Tick = boost::posix_time::microsec_clock::universal_time() - boost::posix_time::microseconds((int)(p_time - m_Seconds) * 1000000);
		m_SecondLock.unlock();
		m_countthread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&LyricManager::CountLyric, this)));
	}
}

void LyricManager::on_playback_new_track(metadb_handle_ptr p_track)
{
	if(m_fetchthread)
	{
		m_fetchthread->interrupt();
		m_fetchthread->join();
		m_fetchthread.reset();
	}
	if(m_countthread)
	{
		m_countthread->interrupt();
		m_countthread->join();
		m_countthread.reset();
	}
	m_CurrentLyric.Clear();
	m_LyricLine = m_CurrentLyric.GetIteratorAt(0);
	m_fetchthread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&LyricManager::FetchLyric, this, p_track)));
	m_track = p_track;
	m_Tick = boost::posix_time::microsec_clock::universal_time();
	m_Seconds = 0;
}

void LyricManager::on_playback_stop(play_control::t_stop_reason reason)
{
	if(m_countthread)
	{
		m_countthread->interrupt();
		m_countthread->join();
		m_countthread.reset();
	}

	m_CurrentLyric.Clear();
	m_LyricLine = m_CurrentLyric.GetIteratorAt(0);
	m_track.release();
}

void LyricManager::on_playback_time(double p_time)
{
	m_SecondLock.lock();
	m_Tick = boost::posix_time::microsec_clock::universal_time();
	m_Seconds = (int)p_time;
	m_SecondLock.unlock();
}

void LyricManager::on_playback_pause(bool p_state)
{
	static long long microsec;
	if(p_state == true && m_countthread)
	{
		m_countthread->interrupt();
		m_countthread->join();
		m_countthread.reset();
		microsec = (boost::posix_time::microsec_clock::universal_time() - m_Tick).fractional_seconds() / 10000;
	}
	else if(p_state == false && m_CurrentLyric.HasLyric())
	{
		m_Tick = boost::posix_time::microsec_clock::universal_time() - (boost::posix_time::seconds(1) - boost::posix_time::microseconds(microsec * 1000000));
		m_countthread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&LyricManager::CountLyric, this)));
	}
}

std::vector<LyricLine> LyricManager::GetLyricBefore(int n)
{
	if(m_CurrentLyric.HasLyric() && m_CurrentLyric.IsValidIterator(m_LyricLine) && !m_CurrentLyric.IsBeginOfLyric(m_LyricLine))
	{
		std::vector<LyricLine> ret;
		int cnt;
		std::vector<LyricLine>::const_iterator it = m_LyricLine - 1;
		for(cnt = 0; (cnt < n); cnt ++, it --)
		{
			ret.push_back(*it);
			if(m_CurrentLyric.IsBeginOfLyric(it))
				break;
		}

		std::reverse(ret.begin(), ret.end());

		return ret;
	}
	return std::vector<LyricLine>();
}

std::vector<LyricLine> LyricManager::GetLyric()
{
	if(m_CurrentLyric.HasLyric() && m_CurrentLyric.IsValidIterator(m_LyricLine))
	{
		std::vector<LyricLine> ret;
		std::vector<LyricLine>::const_iterator it;
		for(it = m_LyricLine; !m_CurrentLyric.IsEndOfLyric(it) && it->time == m_LyricLine->time; it ++)
			ret.push_back(*it);

		return ret;
	}
	return std::vector<LyricLine>(1, LyricLine(0, m_Status));
}

std::vector<LyricLine> LyricManager::GetLyricAfter(int n)
{
	if(m_CurrentLyric.HasLyric() && m_CurrentLyric.IsValidIterator(m_LyricLine) && !m_CurrentLyric.IsEndOfLyric(m_LyricLine))
	{
		std::vector<LyricLine> ret;
		int cnt;
		std::vector<LyricLine>::const_iterator it;
		for(it = m_LyricLine + 1, cnt = 0; (cnt < n) && !m_CurrentLyric.IsEndOfLyric(it); cnt ++, it ++)
			ret.push_back(*it);

		return ret;
	}
	return std::vector<LyricLine>();
}

void LyricManager::CountLyric()
{
	long long microsec = (boost::posix_time::microsec_clock::universal_time() - m_Tick).fractional_seconds() / 10000;	//sec:0, microsec:10
																														//0				<-- m_LyricPos
	if(m_Seconds == 0)																									//0   some song
		m_LyricLine = m_CurrentLyric.GetIteratorAt(0);																	//0				
	else																												//100 blah			
		m_LyricLine = m_CurrentLyric.GetIteratorAt(int(m_Seconds * 100 + microsec));
	if(m_CurrentLyric.IsEndOfLyric(m_LyricLine))
	{
		m_LyricLine --;
		RedrawHandler(); //Point to last lyric
		return;
	}

	if(!m_CurrentLyric.IsBeginOfLyric(m_LyricLine))
	{
		m_LyricLine --;
		while(!m_CurrentLyric.IsBeginOfLyric(m_LyricLine) && m_LyricLine->time == 0) m_LyricLine --;//point to last visible line
		while(!m_CurrentLyric.IsBeginOfLyric(m_LyricLine) && (m_LyricLine - 1)->time == m_LyricLine->time) m_LyricLine --;
	}
	RedrawHandler();
	try
	{
		while(!m_CurrentLyric.IsEndOfLyric(m_LyricLine))
		{
			m_SecondLock.lock();

			microsec = (boost::posix_time::microsec_clock::universal_time() - m_Tick).fractional_seconds() / 10000;
			std::vector<LyricLine>::const_iterator tempit = m_LyricLine;
			while(!m_CurrentLyric.IsEndOfLyric(tempit) && (int)tempit->time - (m_Seconds * 100 + microsec) < 0) tempit ++;
			while(!m_CurrentLyric.IsEndOfLyric(tempit) && tempit->time == 0) tempit ++;
			if(m_CurrentLyric.IsEndOfLyric(tempit))
			{
				m_SecondLock.unlock();
				break;
			}

			boost::posix_time::microseconds ms = boost::posix_time::microseconds(tempit->time - (m_Seconds * 100 + microsec));
			m_SecondLock.unlock();

			boost::this_thread::sleep(ms * 10000);
			m_LyricLine = tempit;
			if(boost::this_thread::interruption_requested())
				break;
			RedrawHandler();
		}
		RedrawHandler(); //Point to last lyric
	}
	catch(...)
	{ //ignore exception
	}
}

DWORD LyricManager::FetchLyric(const metadb_handle_ptr &track)
{
	m_CurrentLyric.Clear();

	m_Status = std::string(pfc::stringcvt::string_utf8_from_wide(TEXT("가사 다운로드 중...")));
	if(boost::this_thread::interruption_requested())
		return false;
	RedrawHandler();

	try
	{
		m_CurrentLyric = LyricSourceAlsong().Get(track);
		m_LyricLine = m_CurrentLyric.GetIteratorAt(0);
	}
	catch(std::exception e)
	{
		m_Status = std::string(pfc::stringcvt::string_utf8_from_wide(TEXT("가사 다운로드 중 예외 발생 : "))) + e.what();
	}
	
	if(boost::this_thread::interruption_requested())
		return false;
	RedrawHandler();

	if(!m_CurrentLyric.HasLyric())
	{
		m_Status = std::string(pfc::stringcvt::string_utf8_from_wide(TEXT("실시간 가사를 찾을 수 없습니다.")));
		RedrawHandler();
		return false;
	}

	m_countthread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&LyricManager::CountLyric, this)));

	//cleanup
	m_fetchthread->detach();
	m_fetchthread.reset();
	return true;
}

void LyricManager::Reload(const metadb_handle_ptr &track)
{
	if(track == LyricManagerInstance->m_track)
	{
		if(LyricManagerInstance->m_fetchthread)
		{
			LyricManagerInstance->m_fetchthread->interrupt();
			LyricManagerInstance->m_fetchthread->join();
			LyricManagerInstance->m_fetchthread.reset();
		}
		if(LyricManagerInstance->m_countthread)
		{
			LyricManagerInstance->m_countthread->interrupt();
			LyricManagerInstance->m_countthread->join();
			LyricManagerInstance->m_countthread.reset();
		}
		LyricManagerInstance->m_fetchthread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&LyricManager::FetchLyric, LyricManagerInstance, track)));
	}
}
