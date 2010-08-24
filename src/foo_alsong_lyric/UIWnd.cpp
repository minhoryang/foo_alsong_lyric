/*
* foo_alsong_lyric														
* Copyright (C) 2007-2010 Inseok Lee <dlunch@gmail.com>
*
* This library is free software; you can redistribute it and/or modify it 
* under the terms of the GNU Lesser General Public License as published 
* by the Free Software Foundation; version 2.1 of the License.
*
* This library is distributed in the hope that it will be useful, 
* but WITHOUT ANY WARRANTY; without even the implied warranty of 
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
* See the GNU Lesser General Public License for more details.
*
* You can receive a copy of the GNU Lesser General Public License from 
* http://www.gnu.org/
*/

#include "stdafx.h"
#include "ConfigStore.h"
#include "UIManager.h"
#include "UIWnd.h"
#include "UIPreference.h"
#include <propkey.h>
#include <propvarutil.h>

//TODO:UpdateLayeredWindow() 사용. http://msdn.microsoft.com/en-us/library/bb773289(VS.85).aspx 로 직접 창틀 그리기

UIWnd WndInstance; //singleton

//SHGetPropertyStoreForWindow prototype
typedef DECLSPEC_IMPORT HRESULT (STDAPICALLTYPE *SHGPSFW)(HWND hwnd,
														  REFIID riid,
														  void** ppv);


/* interface ICustomDestinationList */
/* [unique][object][uuid] */ 

typedef /* [v1_enum] */ 
enum KNOWNDESTCATEGORY
{	KDC_FREQUENT	= 1,
KDC_RECENT	= ( KDC_FREQUENT + 1 ) 
} 	KNOWNDESTCATEGORY;

//icustomdestinationlist prototype
EXTERN_C const IID IID_ICustomDestinationList;
MIDL_INTERFACE("6332debf-87b5-4670-90c0-5e57b408a49e")
ICustomDestinationList : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE SetAppID( 
		/* [string][in] */ __RPC__in_string LPCWSTR pszAppID) = 0;

	virtual HRESULT STDMETHODCALLTYPE BeginList( 
		/* [out] */ __RPC__out UINT *pcMinSlots,
		/* [in] */ __RPC__in REFIID riid,
		/* [iid_is][out] */ __RPC__deref_out_opt void **ppv) = 0;

	virtual HRESULT STDMETHODCALLTYPE AppendCategory( 
		/* [string][in] */ __RPC__in_string LPCWSTR pszCategory,
		/* [in] */ __RPC__in_opt IObjectArray *poa) = 0;

	virtual HRESULT STDMETHODCALLTYPE AppendKnownCategory( 
		/* [in] */ KNOWNDESTCATEGORY category) = 0;

	virtual HRESULT STDMETHODCALLTYPE AddUserTasks( 
		/* [in] */ __RPC__in_opt IObjectArray *poa) = 0;

	virtual HRESULT STDMETHODCALLTYPE CommitList( void) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetRemovedDestinations( 
		/* [in] */ __RPC__in REFIID riid,
		/* [iid_is][out] */ __RPC__deref_out_opt void **ppv) = 0;

	virtual HRESULT STDMETHODCALLTYPE DeleteList( 
		/* [string][unique][in] */ __RPC__in_opt_string LPCWSTR pszAppID) = 0;

	virtual HRESULT STDMETHODCALLTYPE AbortList( void) = 0;

};

UIWnd::UIWnd() : m_isResizing(false)
{

}

HWND UIWnd::Create() 
{
	assert(m_hWnd == NULL);

	if(cfg_outer.get_value().GetnLine() == 0)
		cfg_outer.get_value().SetDefault();
	m_UI = new UIManager(&cfg_outer.get_value(), &cfg_outer_script);

	WNDCLASSEX wcex;
	memset(&wcex, 0, sizeof(wcex));
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = &WindowProc;
	wcex.hInstance = NULL;
	wcex.hIconSm = NULL;
	wcex.hIcon = static_api_ptr_t<ui_control>()->get_main_icon();
	wcex.hCursor = LoadCursor(0, IDC_ARROW);
	wcex.hbrBackground = NULL;
	wcex.lpszClassName = TEXT("UILyricWindow");
	wcex.cbWndExtra = sizeof(UIWnd *);
	RegisterClassEx(&wcex);
	
	m_hWnd = CreateWindowEx(
		(cfg_outer_topmost ? WS_EX_TOPMOST : 0) | 
		(cfg_outer_layered ? WS_EX_TRANSPARENT : 0) | 
		(cfg_outer_nolayered ? 0 : WS_EX_LAYERED) | 
		(cfg_outer_taskbar ? WS_EX_TOOLWINDOW : 0),
		TEXT("UILyricWindow"),
		TEXT("가사 창"),
		WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT,
		500, 200,
		NULL,
		0,
		NULL,
		this );
	//windows 7 taskbar

	HMODULE shell32 = LoadLibrary(L"shell32.dll");
	SHGPSFW SHGetPropertyStoreForWindow = reinterpret_cast<SHGPSFW>(GetProcAddress(shell32, "SHGetPropertyStoreForWindow"));
	m_Propstore = NULL;
	if(SHGetPropertyStoreForWindow)
	{
		SHGetPropertyStoreForWindow(m_hWnd, IID_IPropertyStore, (void **)&m_Propstore);
		wchar_t appid[] = L"dlunch.foo_alsong_lyric";
		if(m_Propstore)
		{
			PROPVARIANT propvar;
			InitPropVariantFromString(appid, &propvar);
			m_Propstore->SetValue(PKEY_AppUserModel_ID, propvar); //use separate window
			InitPropVariantFromBoolean(true, &propvar);
			m_Propstore->SetValue(PKEY_AppUserModel_PreventPinning, propvar); //not to pin
		}
		AddTaskList(L"알송 실시간 가사", L"알송 실시간 가사 창", L"");
		AddTaskList(L"Alsong Lyric Window Config", L"알송 실시간 가사 창 설정", appid);
	}
	ShowWindow(m_hWnd, SW_HIDE);
	HMENU hMenu = GetSystemMenu(m_hWnd, FALSE);
	AppendMenu(hMenu, MF_STRING, 1000, TEXT("고급 설정..."));
	
	return m_hWnd;
}

int UIWnd::AddTaskList(std::wstring command, std::wstring display, std::wstring appid)
{
	ICustomDestinationList *destlist = NULL;
	CoCreateInstance(CLSID_DestinationList, NULL, CLSCTX_INPROC_SERVER, IID_ICustomDestinationList, (void **)&destlist);
	if(destlist) //main window list
	{
		UINT MinSlot;
		IObjectArray *removed;
		if(appid.size())
			destlist->SetAppID(appid.c_str());
		destlist->BeginList(&MinSlot, IID_IObjectArray, (void **)&removed);
		IObjectCollection *tasks;
		CoCreateInstance(CLSID_EnumerableObjectCollection, NULL, CLSCTX_INPROC_SERVER, IID_IObjectCollection, (void **)&tasks);

		IShellLink *link;
		CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void **)&link);
		wchar_t name[255];
		GetModuleFileName(GetModuleHandle(L"foobar2000.exe"), name, 255);
		link->SetPath(name);
		link->SetArguments((std::wstring(L"/command:\"") + command + L"\"").c_str());

		IPropertyStore *propstore;
		link->QueryInterface(IID_IPropertyStore, (void **)&propstore);
		PROPVARIANT pv;
		InitPropVariantFromString(display.c_str(), &pv);
		propstore->SetValue(PKEY_Title, pv);
		propstore->Commit();
		propstore->Release();

		tasks->AddObject(link);
		IObjectArray *arr;
		tasks->QueryInterface(IID_IObjectArray, (void **)&arr);
		destlist->AddUserTasks(arr);
		destlist->CommitList();

		destlist->Release();
		tasks->Release();
		arr->Release();
		link->Release();
		removed->Release();
		
		return 1;
	}

	return 0;
}

void UIWnd::Destroy() 
{
	// Destroy the window.
	if(m_hWnd)
	{
		if(m_Propstore)
		{
			PROPVARIANT propvar;
			propvar.vt = VT_EMPTY;
			m_Propstore->SetValue(PKEY_AppUserModel_ID, propvar);
			m_Propstore->SetValue(PKEY_AppUserModel_PreventPinning, propvar); //reset
			m_Propstore->Release();
		}
		DestroyWindow(m_hWnd);
		delete m_UI;
	}
}

void UIWnd::Show()
{
	ShowWindow(m_hWnd, SW_SHOW);
	cfg_outer_shown = true;
}

void UIWnd::Hide()
{
	ShowWindow(m_hWnd, SW_HIDE);
	cfg_outer_shown = false;
}

void UIWnd::StyleUpdated()
{
	SetWindowLong(m_hWnd, GWL_STYLE, WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX);

	SetWindowLong(m_hWnd, GWL_EXSTYLE, (cfg_outer_topmost ? WS_EX_TOPMOST : 0) | (cfg_outer_layered ? WS_EX_TRANSPARENT : 0) | (cfg_outer_nolayered ? 0 : WS_EX_LAYERED) | (cfg_outer_taskbar ? WS_EX_TOOLWINDOW : 0));
	if(cfg_outer_shown)
		SetWindowPos(m_hWnd, (cfg_outer_topmost ? HWND_TOPMOST : HWND_NOTOPMOST), 0, 0, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOMOVE | SWP_FRAMECHANGED);
	else
		SetWindowPos(m_hWnd, (cfg_outer_topmost ? HWND_TOPMOST : HWND_NOTOPMOST), 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_FRAMECHANGED);

	InvalidateRect(m_hWnd, NULL, TRUE);
}

LRESULT CALLBACK UIWnd::WindowProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	if(iMessage == WM_NCCREATE)
		SetWindowLongPtr(hWnd, GWL_USERDATA, (LONG)((CREATESTRUCT *)lParam)->lpCreateParams);
	UIWnd *_this = (UIWnd *)GetWindowLongPtr(hWnd, GWL_USERDATA);
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