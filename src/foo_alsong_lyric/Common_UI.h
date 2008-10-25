
struct WindowInfo
{
	Bitmap *BackImage;
	Bitmap *BackImageCache;
	SquirrelVMSys vm;
};

class Common_UI_Base : public play_callback
{
	friend class save_lrc_callback;
private:
	bool on_keydown(WPARAM wParam);
	void on_contextmenu(HWND hWndFrom);
	void GetLRCSavePath(WCHAR *path);
	void RenderScreen(HWND hWnd, HDC hdc, Window_Setting *NowSetting);
	
	void UnInitializeScript(SquirrelVMSys *vm);
	SquirrelVMSys InitializeScript();
	void RunRenderScript(HWND hWnd, HDC hdc, Window_Setting *Setting);
	void RunGlobalScript(HWND hWnd, HDC hdc, Window_Setting *Setting);

	class Common_Lyric_Manipulation *Lyric;

	map<HWND, WindowInfo *> WndInfo;

public:

	DWORD NowTime;
	DWORD NowLine;

	HANDLE hLyricThreadQuit;
	HANDLE hLyricThread;
	HANDLE hTime;

	Common_UI_Base();
	~Common_UI_Base();

	const char *NowPlaying_Path;
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
};

extern Common_UI_Base *Common_UI; 

extern COLORREF acrCustClr[16];
