#pragma once

#include "Lyric.h"
#include "LyricSource.h"

class LyricManager : public play_callback
{
private:
	boost::signals2::signal<void ()> RedrawHandler;
	boost::shared_ptr<boost::thread> m_fetchthread;
	boost::shared_ptr<boost::thread> m_countthread;

	int m_Seconds;
	metadb_handle_ptr m_track;
	boost::posix_time::ptime m_Tick;
	boost::mutex m_SecondLock; //lock tick/seconds
	boost::shared_ptr<Lyric> m_CurrentLyric; //current song's lyric
	std::vector<LyricLine>::const_iterator m_LyricLine; //current visible line.
	std::string m_Status;

	void CountLyric();
	DWORD FetchLyric(const metadb_handle_ptr &track);
	std::vector<boost::shared_ptr<LyricSource> > m_lyricSources;
	std::vector<boost::shared_ptr<LyricSource> > m_lyricSaveSources;
public:
	LyricManager();
	~LyricManager();

	boost::signals2::connection AddRedrawHandler(const boost::signals2::signal<void ()>::slot_type &Handler)
	{
		return RedrawHandler.connect(Handler);
	}

	static void Reload(const metadb_handle_ptr &p_track);

	std::vector<LyricLine> GetLyricBefore(int n); //이전 가사. n:줄수
	std::vector<LyricLine> GetLyric(); //현재 표시할 가사 보여주기
	std::vector<LyricLine> GetLyricAfter(int n); //다음가사. n:줄

	// play_callback methods (the ones we're interested in)
	virtual void on_playback_seek(double p_time);
	virtual void on_playback_new_track(metadb_handle_ptr p_track);
	virtual void on_playback_stop(play_control::t_stop_reason reason);
	virtual void on_playback_time(double p_time);
	virtual void on_playback_pause(bool p_state);

	// play_callback methods (the rest)
	virtual void on_playback_dynamic_info_track(const file_info & p_info) {}
	virtual void on_playback_starting(play_control::t_track_command p_command, bool p_paused) {}
	virtual void on_playback_edited(metadb_handle_ptr p_track) {}
	virtual void on_playback_dynamic_info(const file_info & p_info) {}
	virtual void on_volume_change(float p_new_val) {}
};

extern LyricManager *LyricManagerInstance; //singleton. must created on on_init

