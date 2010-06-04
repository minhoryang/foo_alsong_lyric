#pragma once

#include "ConfigStore.h"

class UIManager
{
public:
	UIManager() {}
	UIManager(UIPreference *Setting, pfc::string8 *Script);
	~UIManager();

	LRESULT ProcessMessage(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
	void ShowConfig(HWND hWndParent);

private:
	bool on_keydown(WPARAM wParam);
	void on_contextmenu(HWND hWndparent);
	void Draw(HWND hWnd, HDC hdc);

	void UnInitializeScript(SquirrelVMSys vm);
	SquirrelVMSys InitializeScript();
	void SetLyricArea(int x, int y, int width, int height);
	static void ScriptDebugLog(HSQUIRRELVM v,const SQChar* s,...);

	RECT m_LyricArea;
	UIPreference *m_Setting;
	pfc::string8 *m_Script;

	Bitmap *m_BackImage;
	Bitmap *m_BackImageCache;
};

DECLARE_INSTANCE_TYPE_NAME(UIManager, UIManager)

