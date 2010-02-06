#pragma once

#include "ConfigStore.h"

class AlsongUI
{
public:
	AlsongUI(Window_Setting &Setting, pfc::string8 &Script);
	~AlsongUI();

	LRESULT ProcessMessage(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
	void ShowConfig(HWND hWndParent);

private:
	bool on_keydown(WPARAM wParam);
	void on_contextmenu(HWND hWndparent);
	void Draw(HDC hdc);

	void UnInitializeScript(SquirrelVMSys *vm);
	SquirrelVMSys InitializeScript();

	HWND m_hWnd;

	Window_Setting &m_Setting;
	pfc::string8 &m_Script;

	Bitmap *m_BackImage;
	Bitmap *m_BackImageCache;
};

extern COLORREF acrCustClr[16];
