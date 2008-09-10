class WindowData //각 윈도우마다 저장될 데이터
{
private:
	Gdiplus::Bitmap *CachedBitmap;
	Gdiplus::Bitmap *bmp;
	Alsong_Setting Setting;
	HBITMAP BackBuffer;

	void UpdateCache(int x, int y, int width, int height)
	{
		if(CachedBitmap)
			delete CachedBitmap;
		CachedBitmap = new Gdiplus::Bitmap(width, height);

		Gdiplus::Graphics g(CachedBitmap);
		if(height == 0)
			height = 5; 
		g.DrawImage(bmp, x, y, width, height);

	}
public:
	void SetBackBuffer(HBITMAP hBuffer)
	{
		BackBuffer = hBuffer;
	}

	HBITMAP GetBackBuffer()
	{
		return BackBuffer;
	}

	void ForceUpdateCache(int x, int y, int width, int height)
	{
		if(!bmp)
			return;
		if(CachedBitmap)
			delete CachedBitmap;
		CachedBitmap = NULL;

		UpdateCache(x, y, width, height);
	}

	WindowData(Alsong_Setting *NowSetting) //Setting으로부터 내용을 채운다.
	{
		if(NowSetting->bgType && NowSetting->bgImage[0])
			bmp = new Gdiplus::Bitmap(NowSetting->bgImage);
		else
			bmp = NULL;

		CopyMemory(&Setting, NowSetting, sizeof(Alsong_Setting));

		CachedBitmap = NULL;
		BackBuffer = NULL;
	}

	~WindowData()
	{
		DeleteObject(CachedBitmap);
		CachedBitmap = NULL;
	}

	DWORD UpdateData(Alsong_Setting *NowSetting) //바뀌면 TRUE
	{
		DWORD bRet = FALSE;
		if(NowSetting->bgType != Setting.bgType) //다르면
			bRet = TRUE;
		if(((NowSetting->bgType != Setting.bgType) && NowSetting->bgImage[0]) || lstrcmp(NowSetting->bgImage, Setting.bgImage))
			//바뀌면 업데이트
		{
			if(bmp)
				delete bmp;
			bmp = new Gdiplus::Bitmap(NowSetting->bgImage);
		}
		CopyMemory(&Setting, NowSetting, sizeof(Alsong_Setting));

		return bRet;
	}

	Gdiplus::Status DrawImage(HDC hdc, int x, int y, int width, int height) 
	{
		try
		{
			if(!CachedBitmap)
				UpdateCache(x, y, width, height);

			Gdiplus::Graphics g(hdc);
			return g.DrawImage(CachedBitmap, x, y, width, height);
		}
		catch(...)
		{
			return Gdiplus::GenericError;
		}
	}
};

class Common_UI_Base : public play_callback
{
	friend class save_lrc_callback;
private:
	bool on_keydown(WPARAM wParam);
	void on_contextmenu(HWND hWndFrom);
	void GetLRCSavePath(WCHAR *path);

	class Common_Lyric_Manipulation *Lyric;
	std::map<HWND, WindowData *> WndInfo;

public:

	DWORD NowTime;
	DWORD NowLine;

	HANDLE hLyricThread;
	HANDLE hTime;
	HANDLE hSleep;

	Common_UI_Base();
	~Common_UI_Base();

	const char *NowPlaying_Path;
	const char *LRCSave_Path;

	void InvalidateAllWindow();
	LRESULT Process_Message(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam, Alsong_Setting *NowSetting, BOOL isOuter);

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