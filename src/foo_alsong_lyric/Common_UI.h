#pragma once

class AlsongUI : public play_callback
{
private:
	bool on_keydown(WPARAM wParam);
	void on_contextmenu(HWND hWndFrom);
	void GetLRCSavePath(WCHAR *path);
	void RenderScreen(HWND hWnd, HDC hdc);
	
	void UnInitializeScript(SquirrelVMSys *vm);
	SquirrelVMSys InitializeScript();
	void RunRenderScript(HWND hWnd, HDC hdc);
	void RunGlobalScript(HWND hWnd, HDC hdc);

	class Common_Lyric_Manipulation *Lyric;
	HWND m_hWnd;
	
	class save_lrc_callback : public main_thread_callback
	{
		AlsongUI *UI;
	public:
		save_lrc_callback(AlsongUI *UIPointer)
		{
			UI = UIPointer;
		}

		virtual void callback_run()
		{
			WCHAR wctemp[255];
			pfc::string8 str;
			str = UI->LRCSave_Path;

			UI->GetLRCSavePath(wctemp);
			if(wctemp[0] == 0)
				return;

			if(lstrcmpA(str.get_ptr(), UI->NowPlaying_Track->get_path()) != 0)
				return; //°î¹Ù²î¸é ¹«½Ã

			UI->Lyric->SaveToFile(wctemp, "lrc");
		}
	};


public:

	DWORD NowTime;
	DWORD NowLine;

	HANDLE hLyricThreadQuit;
	HANDLE hLyricThread;
	HANDLE hTime;
	
	Bitmap *BackImage;
	Bitmap *BackImageCache;
	SquirrelVMSys vm;

	AlsongUI_Base();
	~AlsongUI_Base();

	metadb_handle_ptr NowPlaying_Track;
	const char *LRCSave_Path;

	void InvalidateAllWindow();
	LRESULT Process_Message(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam, Window_Setting *NowSetting, BOOL isOuter);

	static UINT CALLBACK LyricFetchThread(LPVOID lpParameter);
	static UINT CALLBACK LyricCountThread(LPVOID lpParameter);

	// play_callback methods (the ones we're interested in)
	virtual void on_playback_seek(double p_time);
	virtual void on_playback_new_track(metadb_handle_ptr p_track);
	virtual void on_playback_stop(play_control::t_stop_reason reason);
	virtual void on_playback_time(double p_time);

	// play_callback methods (the rest)
	virtual void on_playback_dynamic_info_track(const file_info & p_info) {}
	virtual void on_playback_starting(play_control::t_track_command p_command, bool p_paused) {}
	virtual void on_playback_pause(bool p_state);
	virtual void on_playback_edited(metadb_handle_ptr p_track) {}
	virtual void on_playback_dynamic_info(const file_info & p_info) {}
	virtual void on_volume_change(float p_new_val) {}

	void ReloadLyric();

	//Script Functions & Variables

	static pair<HWND, WindowInfo *> ScriptingInfo;
	static void SetBackgroundImage(const TCHAR *Filename);
};

extern COLORREF acrCustClr[16];
