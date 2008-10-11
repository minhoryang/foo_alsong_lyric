#include "stdafx.h"
#include "Panel_Plugin.h"
#include "Outer_Window_Plugin.h"
#include "Common_Settings.h"
#include "Common_UI.h"

DECLARE_COMPONENT_VERSION(
"Alsong lyric plugin",
"0.1.0",
"Alsong lyric plugin for foobar2000\n"
"Developed by dlunch(dlunch@gmail.com)\n"
"compiled: " __DATE__ "\n"
"with Panel API version: " UI_EXTENSION_VERSION)

static ULONG_PTR gdiplus_token;

class initquit_alsong : public initquit 
{
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

		Common_UI = new Common_UI_Base();

		g_OuterWindow.Create();
		if (cfg_outer_shown)
			g_OuterWindow.Show();
	}

	virtual void on_quit() 
	{
		g_OuterWindow.Destroy();
		delete Common_UI;
		TIMECAPS ptc;
		timeGetDevCaps(&ptc, sizeof(TIMECAPS));
		timeEndPeriod(ptc.wPeriodMin);
		WSACleanup();
		GdiplusShutdown(gdiplus_token);
	}
};

static initquit_factory_t<initquit_alsong> foo_initquit;
