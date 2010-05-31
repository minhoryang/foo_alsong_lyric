#include "stdafx.h"
#include "UIWnd.h"
#include "ConfigStore.h"
#include "UIManager.h"
#include "LyricManager.h"

DECLARE_COMPONENT_VERSION(
"Alsong lyric plugin",
"0.2.0",
"Alsong lyric plugin for foobar2000\n"
"Developed by dlunch(dlunch@gmail.com)\n"
"compiled: " __DATE__ "\n"
"with Panel API version: " UI_EXTENSION_VERSION)

class initquit_plugin : public initquit 
{
private:
	ULONG_PTR gdiplus_token;
public:
	virtual void on_init() 
	{
		WSADATA wd;
		WSAStartup(MAKEWORD(2, 2), &wd);

		TIMECAPS ptc;
		timeGetDevCaps(&ptc, sizeof(TIMECAPS));
		timeBeginPeriod(ptc.wPeriodMin);

		GdiplusStartupInput gsi;
		gsi.GdiplusVersion = 1;
		gsi.DebugEventCallback = NULL;
		gsi.SuppressBackgroundThread = FALSE;
		gsi.SuppressExternalCodecs = TRUE;

		GdiplusStartup(&gdiplus_token, &gsi, NULL);

		if(!LyricManagerInstance)
			LyricManagerInstance = new LyricManager();

		WndInstance.Create();
		if (cfg_outer_shown)
			WndInstance.Show();
	}

	virtual void on_quit() 
	{
		WndInstance.Destroy();
		TIMECAPS ptc;
		timeGetDevCaps(&ptc, sizeof(TIMECAPS));
		timeEndPeriod(ptc.wPeriodMin);
		WSACleanup();
		GdiplusShutdown(gdiplus_token);

		SquirrelVM::AppFinalShutdown();
		if(LyricManagerInstance)
			delete LyricManagerInstance;
	}
};

static initquit_factory_t<initquit_plugin> plugin_initquit;
