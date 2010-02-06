#include "stdafx.h"
#include "ConfigStore.h"
#include "AlsongUI.h"
#include "AlsongWnd.h"
#include "Preferences.h"

//TODO:UpdateLayeredWindow() 사용. http://msdn.microsoft.com/en-us/library/bb773289(VS.85).aspx 로 직접 창틀 그리기

AlsongWnd AlsongWndInstance; //singleton

HWND AlsongWnd::Create() 
{
	assert(m_hWnd == NULL);

	m_UI = new AlsongUI(cfg_outer.get_value(), cfg_outer_script);

	WNDCLASSEX wcex;
	memset(&wcex, 0, sizeof(wcex));
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_DBLCLKS;
	wcex.lpfnWndProc = &WindowProc;
	wcex.hInstance = NULL;
	wcex.hIconSm = NULL;
	wcex.hIcon = static_api_ptr_t<ui_control>()->get_main_icon();
	wcex.hCursor = LoadCursor(0, IDC_ARROW);
	wcex.hbrBackground = NULL;
	wcex.lpszClassName = TEXT("AlsongLyricWindow");
	wcex.cbWndExtra = sizeof(AlsongWnd *);
	RegisterClassEx(&wcex);
	
	m_hWnd = CreateWindowEx(
		(cfg_outer_topmost ? WS_EX_TOPMOST : 0) | 
		(cfg_outer_layered ? WS_EX_TRANSPARENT | WS_EX_TOPMOST : 0) | 
		WS_EX_LAYERED,
		TEXT("AlsongLyricWindow"),
		TEXT("Alsong Lyric"),
		(cfg_outer_border ? WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX : WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX),
		CW_USEDEFAULT, CW_USEDEFAULT,
		500, 200,
		NULL,
		0,
		NULL,
		this );
	SetLayeredWindowAttributes(m_hWnd, 0, (255 * cfg_outer_transparency) / 100, LWA_ALPHA);
	ShowWindow(m_hWnd, SW_HIDE);
	HMENU hMenu = GetSystemMenu(m_hWnd, FALSE);
	AppendMenu(hMenu, MF_STRING, 1000, TEXT("고급 설정..."));
	
	return m_hWnd;
}

void AlsongWnd::Destroy() 
{
	// Destroy the window.
	if (m_hWnd) 
	{
		DestroyWindow(m_hWnd);
	}
}

void AlsongWnd::Show()
{
	ShowWindow(m_hWnd, SW_SHOW);
	cfg_outer_shown = true;
}

void AlsongWnd::Hide()
{
	ShowWindow(m_hWnd, SW_HIDE);
	cfg_outer_shown = false;
}

LRESULT CALLBACK AlsongWnd::WindowProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	if(iMessage == WM_NCCREATE)
		SetWindowLongPtr(hWnd, GWL_USERDATA, (LONG)((CREATESTRUCT *)lParam)->lpCreateParams);
	AlsongWnd *_this = (AlsongWnd *)GetWindowLongPtr(hWnd, GWL_USERDATA);
	switch(iMessage)
	{
	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		cfg_outer_shown = false;
		return 0;
	case WM_SYSCOMMAND:
		if(wParam == 1000)
			_this->m_UI->ShowConfig(hWnd);

		break;
	case WM_NCHITTEST:
		{
			LRESULT ret = DefWindowProc(hWnd, iMessage, wParam, lParam);
			if(ret == HTCLIENT)
				return HTCAPTION;
			return ret;
		}
	case WM_DESTROY:
		if(!IsIconic(hWnd))
			cfg_outer_window_placement.on_window_destruction(hWnd);
		break;
	case WM_CREATE:
		cfg_outer_window_placement.on_window_creation(hWnd);
		_this->m_hWnd = hWnd;
		break;
	}
	if(_this)
		return _this->m_UI->ProcessMessage(hWnd, iMessage, wParam, lParam);
	else
		return DefWindowProc(hWnd, iMessage, wParam, lParam);
}