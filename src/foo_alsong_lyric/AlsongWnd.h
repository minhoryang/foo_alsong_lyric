#pragma once

#include "AlsongUI.h"

class AlsongWnd
{
private:
	HWND m_hWnd;
	AlsongUI *m_UI;
public:
	void Show();
	void Hide();
	HWND Create();
	void Destroy(); 

	HWND GetHWND()
	{
		return m_hWnd;
	}

	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
};

extern AlsongWnd AlsongWndInstance;