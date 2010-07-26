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
		(cfg_outer_layered ? WS_EX_TRANSPARENT | WS_EX_TOPMOST : 0) | 
		WS_EX_LAYERED,
		TEXT("UILyricWindow"),
		TEXT("가사 창"),
		(cfg_outer_border ? WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX : WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX),
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
		ICustomDestinationList *destlist = NULL;
		CoCreateInstance(CLSID_DestinationList, NULL, CLSCTX_INPROC_SERVER, IID_ICustomDestinationList, (void **)&destlist);
		if(destlist) //main window list
		{
			UINT MinSlot;
			IObjectArray *removed;
			destlist->BeginList(&MinSlot, IID_IObjectArray, (void **)&removed);
			IObjectCollection *tasks;
			CoCreateInstance(CLSID_EnumerableObjectCollection, NULL, CLSCTX_INPROC_SERVER, IID_IObjectCollection, (void **)&tasks);
			
			//"c:\Program Files (x86)\foobar2000\foobar2000.exe" /command:"알송 실시간 가사"
			IShellLink *link;
			CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void **)&link);
			wchar_t name[255];
			GetModuleFileName(GetModuleHandle(L"foobar2000.exe"), name, 255);
			link->SetPath(name);
			link->SetArguments(L"/command:\"알송 실시간 가사\"");
			
			IPropertyStore *propstore;
			link->QueryInterface(IID_IPropertyStore, (void **)&propstore);
			PROPVARIANT pv;
			InitPropVariantFromString(L"알송 실시간 가사", &pv);
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
		}
		destlist = NULL;
		CoCreateInstance(CLSID_DestinationList, NULL, CLSCTX_INPROC_SERVER, IID_ICustomDestinationList, (void **)&destlist);
		if(destlist) //alsong window list
		{
			UINT MinSlot;
			IObjectArray *removed;
			destlist->SetAppID(appid);
			destlist->BeginList(&MinSlot, IID_IObjectArray, (void **)&removed);
			IObjectCollection *tasks;
			CoCreateInstance(CLSID_EnumerableObjectCollection, NULL, CLSCTX_INPROC_SERVER, IID_IObjectCollection, (void **)&tasks);

			//"c:\Program Files (x86)\foobar2000\foobar2000.exe" /command:"알송 실시간 가사"
			IShellLink *link;
			CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void **)&link);
			wchar_t name[255];
			GetModuleFileName(GetModuleHandle(L"foobar2000.exe"), name, 255);
			link->SetPath(name);
			link->SetArguments(L"/command:\"Alsong Lyric Window Config\"");

			IPropertyStore *propstore;
			link->QueryInterface(IID_IPropertyStore, (void **)&propstore);
			PROPVARIANT pv;
			InitPropVariantFromString(L"알송 실시간 가사 설정", &pv);
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
		}
	}
	SetLayeredWindowAttributes(m_hWnd, 0, (255 * cfg_outer_transparency) / 100, LWA_ALPHA);
	ShowWindow(m_hWnd, SW_HIDE);
	HMENU hMenu = GetSystemMenu(m_hWnd, FALSE);
	AppendMenu(hMenu, MF_STRING, 1000, TEXT("고급 설정..."));
	
	return m_hWnd;
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