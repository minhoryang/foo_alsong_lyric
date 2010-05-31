#pragma once

#include "ConfigStore.h"

class UIManager
{
public:
	UIManager(UIPreference &Setting, pfc::string8 &Script);
	~UIManager();

	LRESULT ProcessMessage(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
	void ShowConfig(HWND hWndParent);

private:
	bool on_keydown(WPARAM wParam);
	void on_contextmenu(HWND hWndparent);
	void Draw(HWND hWnd, HDC hdc);

	void UnInitializeScript(SquirrelVMSys *vm);
	SquirrelVMSys InitializeScript();

	UIPreference &m_Setting;
	pfc::string8 &m_Script;

	Bitmap *m_BackImage;
	Bitmap *m_BackImageCache;
};

extern COLORREF acrCustClr[16];
