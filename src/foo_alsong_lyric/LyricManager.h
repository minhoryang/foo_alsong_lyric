#pragma once

#include "pugixml/pugixml.hpp"

struct LyricResult
{
	LyricResult() : nInfo(-1) {}
	operator int()
	{
		return nInfo != -1;
	}

	int nInfo;
	std::string Artist;
	std::string Album;
	std::string Title;
	std::string Registrant;
	std::string Lyric;
};

class LyricSearchResult
{
	friend class LyricManager;
private:
	std::vector<char> data;
	pugi::xml_document doc;
	pugi::xml_node node;
public:
	LyricResult Get();
};

class LyricManager : public play_callback
{
private:
	string m_Title;
	string m_Album;
	string m_Artist;
	string m_Registrant;
	struct lyricinfo
	{
		lyricinfo(DWORD t, const std::string &str) : time(t), lyric(str) {}
		lyricinfo() : time(0), lyric("") {}
		DWORD time;
		std::string lyric;
	};
	vector<lyricinfo> m_Lyric;
	int m_Lyricpos;
	int m_Seconds;
	int m_haslyric;

	boost::signals2::signal<void ()> RedrawHandler;
	boost::shared_ptr<boost::thread> m_fetchthread;
	boost::shared_ptr<boost::thread> m_countthread;
	boost::posix_time::ptime begin;
	boost::posix_time::ptime tick;

	static DWORD GetFileHash(metadb_handle_ptr track, CHAR *Hash);
	DWORD ParseLyric(const char *InputLyric, const char *Delimiter);
	void CountLyric();
	DWORD DownloadLyric(CHAR *Hash);
	DWORD FetchLyric(const metadb_handle_ptr &track);
	void Clear();
	
	static SOCKET InitateConnect(CHAR *Address, int port);
	static UINT CALLBACK LyricModifyDialogProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
public:
	LyricManager();
	~LyricManager();

	static DWORD UploadLyric(metadb_handle_ptr track, int PlayTime, int UploadType, const LyricResult &Lyric);
	static int SearchLyricGetCount(const std::string &Artist, const std::string &Title);
	static DWORD SearchLyric(const std::string &Artist, const std::string Title, int nPage, LyricSearchResult &data);
	
	boost::signals2::connection AddRedrawHandler(const boost::signals2::signal<void ()>::slot_type &Handler)
	{
		return RedrawHandler.connect(Handler);
	}

	std::vector<std::string> GetLyricBefore(int n); //이전 가사. n:줄수
	std::vector<std::string> GetLyric(); //현재 표시할 가사 보여주기
	std::vector<std::string> GetLyricAfter(int n); //다음가사. n:줄수

	void SaveToFile(WCHAR *SaveTo, CHAR *fmt);
	DWORD LoadFromFile(WCHAR *LoadFrom, CHAR *fmt);

	void OpenLyricModifyDialog(HWND hWndParent);

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

void md5( unsigned char *input, int ilen, unsigned char output[16] );
