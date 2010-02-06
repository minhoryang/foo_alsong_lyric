#include "stdafx.h"
#include "ConfigStore.h"
#include "AlsongWnd.h"
#include "resource.h"
#include "Preferences.h"

static BOOL CALLBACK UICommonConfigProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK ConfigProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

int CALLBACK PropCallback(HWND hWnd, UINT message, LPARAM lParam)
{
	switch (message)
	{
	case PSCB_PRECREATE:
		{
			LPDLGTEMPLATE lpTemplate = (LPDLGTEMPLATE)lParam;
			DWORD dwOldProtect;
			VirtualProtect(lpTemplate, sizeof(DLGTEMPLATE), PAGE_READWRITE, &dwOldProtect);

			lpTemplate->style = DS_SETFONT | DS_FIXEDSYS | WS_CHILD | WS_SYSMENU | WS_VISIBLE | DS_3DLOOK;

			lpTemplate->dwExtendedStyle = 0;
			return TRUE;
		}
	case PSCB_INITIALIZED:
		{
			ShowWindow(GetDlgItem(hWnd, 0x00000001), SW_HIDE);
			ShowWindow(GetDlgItem(hWnd, 0x00000002), SW_HIDE);
			ShowWindow(GetDlgItem(hWnd, 0x00003021), SW_HIDE);

			//Property sheet bug. See http://support.microsoft.com/kb/149501
			SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_CONTROLPARENT);
			SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
		}
	}

	return 0;
}

static BOOL CALLBACK PrefConfigProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	static HWND hProp;
	switch(iMessage)
	{
	case WM_INITDIALOG:
		{
			PROPSHEETPAGE pages[2];
			HPROPSHEETPAGE hpages[2];
			pages[0].dwSize = sizeof(PROPSHEETPAGE);
			pages[0].dwFlags = PSP_DEFAULT | PSP_USETITLE;
			pages[0].hInstance = core_api::get_my_instance();
			pages[0].pszTemplate = MAKEINTRESOURCE(IDD_COMMON_PREF);
			pages[0].pfnDlgProc = ConfigProc;
			pages[0].pszTitle = TEXT("공통 설정");
			pages[0].lParam = NULL;

			pages[1].dwSize = sizeof(PROPSHEETPAGE);
			pages[1].dwFlags = PSP_DEFAULT | PSP_USETITLE;
			pages[1].hInstance = core_api::get_my_instance();
			pages[1].pszTemplate = MAKEINTRESOURCE(IDD_UI_PREF_COMMON);
			pages[1].pfnDlgProc = UICommonConfigProc;
			pages[1].pszTitle = TEXT("외부 창 설정");
			std::pair<Window_Setting, HWND> temp = std::make_pair(cfg_outer.get_value(), AlsongWndInstance.GetHWND());
			pages[1].lParam = (LPARAM)&temp;

			hpages[0] = CreatePropertySheetPage(&pages[0]);
			hpages[1] = CreatePropertySheetPage(&pages[1]);

			PROPSHEETHEADER psh;
			psh.dwSize = sizeof(psh);
			psh.dwFlags = PSH_DEFAULT | PSH_NOCONTEXTHELP | PSH_USEHICON | PSH_USECALLBACK | PSH_MODELESS;
			psh.hwndParent = hWnd;
			psh.hInstance = core_api::get_my_instance();
			psh.hIcon = static_api_ptr_t<ui_control>()->get_main_icon();
			psh.pszCaption = TEXT("알송 가사 설정");
			psh.nPages = 2;
			psh.nStartPage = 0;
			psh.pfnCallback = PropCallback;
			psh.phpage = hpages;

			hProp = (HWND)PropertySheet(&psh);
		}
		break;
	case WM_SIZE:
		{
			RECT rt;
			GetWindowRect(hWnd, &rt);
			HWND hTab = (HWND)SendMessage(hProp, PSM_GETTABCONTROL, 0, 0);
			MoveWindow(hProp, 0, 0, rt.right - rt.left, rt.bottom - rt.top, TRUE);
			MoveWindow(hTab, 10, 3, rt.right - rt.left - 15, rt.bottom - rt.top - 5, TRUE);
		}
	case WM_DESTROY:
		SendMessage(hProp, PSM_APPLY, NULL, NULL);//마지막 위치 저장 -> 열때 탭복구
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

static BOOL CALLBACK ConfigProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	switch (iMessage)
	{
	case WM_INITDIALOG:
		if(cfg_save_to_lrc == true)
			CheckDlgButton(hWnd, IDC_SAVELRC, TRUE);
		if(cfg_load_from_lrc == true)
			CheckDlgButton(hWnd, IDC_LOADFROMLRC, TRUE);
		if(cfg_lrc_save_path)
			uSetDlgItemText(hWnd, IDC_LRCPATH, cfg_lrc_save_path);
		if(cfg_mimic_lyricshow == true)
			CheckDlgButton(hWnd, IDC_MIMIC, TRUE);
		break;

	case WM_NOTIFY:
		if(((LPNMHDR)lParam)->code == PSN_APPLY)
		{
			cfg_load_from_lrc = (IsDlgButtonChecked(hWnd, IDC_LOADFROMLRC) ? true : false);
			cfg_save_to_lrc = (IsDlgButtonChecked(hWnd, IDC_SAVELRC) ? true : false);
			cfg_lrc_save_path = uGetDlgItemText(hWnd, IDC_LRCPATH).get_ptr();
			cfg_mimic_lyricshow = (IsDlgButtonChecked(hWnd, IDC_MIMIC) ? true : false);

			SetWindowLong(hWnd, DWL_MSGRESULT, PSNRET_NOERROR);
		}
		else if(((LPNMHDR)lParam)->code == PSN_KILLACTIVE)
		{
			SetWindowLong(hWnd, DWL_MSGRESULT, FALSE);
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

// {A58D6A8E-5932-4def-AD63-44185988B105}
static const GUID guid_prefs_alsong_lyric = { 0xa58d6a8e, 0x5932, 0x4def, { 0xad, 0x63, 0x44, 0x18, 0x59, 0x88, 0xb1, 0x5 } };

class preferences_page_alsong_lyric : public preferences_page
{
private:
	HWND hWnd;
public:
	virtual const char * get_name()
	{
		return "Alsong Live Lyric";
	}

	virtual GUID get_guid()
	{
		return guid_prefs_alsong_lyric;
	}

	virtual GUID get_parent_guid()
	{
		return preferences_page::guid_tools;
	}

	virtual bool reset_query()
	{
		return false;
	}

	virtual void reset()
	{
		return;
	}

	virtual HWND create(HWND parent)
	{
		hWnd = uCreateDialog(IDD_PREF, parent, PrefConfigProc, 0);
		return hWnd;
	}
};

static preferences_page_factory_t<preferences_page_alsong_lyric> foo_preferences_page_alsong_lyric;

void UpdateOuterWindowStyle(HWND hWnd)
{
	SetWindowLong(hWnd, GWL_STYLE, (cfg_outer_border ? WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX : WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX));
	//TODO: 작업 표시줄, Alt+Tab에서 없애기

	//100%투명도 아닐경우에만 적용. 항상위 강제
	if(cfg_outer_layered == true && cfg_outer_transparency != 100)
	{
		SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT | WS_EX_TOPMOST);
		if(cfg_outer_shown)
			SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOMOVE | SWP_FRAMECHANGED);
		else
			SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_FRAMECHANGED);
	}
	else
	{
		SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) & ~WS_EX_TRANSPARENT & (cfg_outer_topmost ? 0xFFFFFFFF : ~WS_EX_TOPMOST));
		if(cfg_outer_shown)
			SetWindowPos(hWnd, (cfg_outer_topmost ? HWND_TOPMOST : HWND_NOTOPMOST), 0, 0, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOMOVE | SWP_FRAMECHANGED);
		else
			SetWindowPos(hWnd, (cfg_outer_topmost ? HWND_TOPMOST : HWND_NOTOPMOST), 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_FRAMECHANGED);
	}
	SetLayeredWindowAttributes(hWnd, NULL, (255 * cfg_outer_transparency) / 100, LWA_ALPHA);

	InvalidateRect(hWnd, NULL, TRUE);
}

//TODO: 줄간격 설정
static BOOL CALLBACK UICommonConfigProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	static Window_Setting *Setting;
	static Window_Setting OldSetting;
	static BOOL bOuter = FALSE;
	static HWND hParent;

	static int old_transparency;
	static bool old_layered, old_border;

	switch (iMessage)
	{
	case WM_INITDIALOG:
		{
			Setting = ((pair<Window_Setting *, pair<BOOL, HWND> *> *)(((PROPSHEETPAGE *)lParam)->lParam))->first;
			bOuter = ((pair<Window_Setting *, pair<BOOL, HWND> *> *)(((PROPSHEETPAGE *)lParam)->lParam))->second->first;
			hParent = ((pair<Window_Setting *, pair<BOOL, HWND>* > *)(((PROPSHEETPAGE *)lParam)->lParam))->second->second;
			memcpy(&OldSetting, Setting, sizeof(Window_Setting));
			old_transparency = cfg_outer_transparency;
			old_layered = cfg_outer_layered;
			old_border = cfg_outer_border;

			delete ((pair<Window_Setting *, pair<BOOL, HWND> *> *)((PROPSHEETPAGE *)lParam)->lParam)->second;
			delete ((pair<Window_Setting *, pair<BOOL, HWND> *> *)((PROPSHEETPAGE *)lParam)->lParam);

			SendMessage(GetDlgItem(hWnd, IDC_NLINESPIN), UDM_SETRANGE32, 1, 20);
			SendMessage(GetDlgItem(hWnd, IDC_NLINESPIN), UDM_SETBUDDY, (WPARAM)GetDlgItem(hWnd, IDC_NLINE), 0);
			SendMessage(GetDlgItem(hWnd, IDC_NLINESPIN), UDM_SETPOS32, 0, Setting->nLine);

			SendMessage(GetDlgItem(hWnd, IDC_MARGINSPIN), UDM_SETRANGE32, 0, 100);
			SendMessage(GetDlgItem(hWnd, IDC_MARGINSPIN), UDM_SETBUDDY, (WPARAM)GetDlgItem(hWnd, IDC_LINEMARGIN), 0);
			SendMessage(GetDlgItem(hWnd, IDC_MARGINSPIN), UDM_SETPOS32, 0, Setting->LineMargin);

//			if(Setting->Script)
//				uSetDlgItemText(hWnd, IDC_UISCRIPT, Setting->Script->get_ptr());
			if(bOuter)
			{
				SendMessage(GetDlgItem(hWnd, IDC_TRANSPARENCY), TBM_SETRANGE, TRUE, MAKELONG(0, 100));
				SendMessage(GetDlgItem(hWnd, IDC_TRANSPARENCY), TBM_SETPOS, TRUE, cfg_outer_transparency);

				int pos;
				pos = SendMessage(GetDlgItem(hWnd, IDC_TRANSPARENCY), TBM_GETPOS, 0, 0);
				TCHAR temp[255];
				wsprintf(temp, TEXT("%d%%"), pos);
				SetWindowText(GetDlgItem(hWnd, IDC_TRANSPARENCY_LABEL), temp);

				CheckDlgButton(hWnd, IDC_LAYERED, cfg_outer_layered);
				CheckDlgButton(hWnd, IDC_BORDER, cfg_outer_border);
			}
			else
			{
				SendMessage(GetDlgItem(hWnd, IDC_TRANSPARENCY), WM_CLOSE, 0, 0);
				SendMessage(GetDlgItem(hWnd, IDC_TRANSPARENCY_STATIC), WM_CLOSE, 0, 0);
				SendMessage(GetDlgItem(hWnd, IDC_TRANSPARENCY_LABEL), WM_CLOSE, 0, 0);
				SendMessage(GetDlgItem(hWnd, IDC_LAYERED), WM_CLOSE, 0, 0);
				SendMessage(GetDlgItem(hWnd, IDC_BORDER), WM_CLOSE, 0, 0);
			}

			SendMessage(GetDlgItem(hWnd, IDC_VERTICALALIGN), CB_ADDSTRING, NULL, (LPARAM)TEXT("위"));
			SendMessage(GetDlgItem(hWnd, IDC_VERTICALALIGN), CB_ADDSTRING, NULL, (LPARAM)TEXT("가운데"));
			SendMessage(GetDlgItem(hWnd, IDC_VERTICALALIGN), CB_ADDSTRING, NULL, (LPARAM)TEXT("아래"));
			SendMessage(GetDlgItem(hWnd, IDC_HORIZENTALALIGN), CB_ADDSTRING, NULL, (LPARAM)TEXT("왼쪽"));
			SendMessage(GetDlgItem(hWnd, IDC_HORIZENTALALIGN), CB_ADDSTRING, NULL, (LPARAM)TEXT("가운데"));
			SendMessage(GetDlgItem(hWnd, IDC_HORIZENTALALIGN), CB_ADDSTRING, NULL, (LPARAM)TEXT("오른쪽"));

			SendMessage(GetDlgItem(hWnd, IDC_VERTICALALIGN), CB_SETCURSEL, Setting->VerticalAlign, NULL);
			SendMessage(GetDlgItem(hWnd, IDC_HORIZENTALALIGN), CB_SETCURSEL, Setting->HorizentalAlign, NULL);

			SendMessage(GetDlgItem(hWnd, IDC_MANUALSCRIPT), WM_CLOSE, 0, 0);
			SendMessage(GetDlgItem(hWnd, IDC_UISCRIPT), WM_CLOSE, 0, 0);
		}

		break;
	case WM_HSCROLL:
		if(bOuter)
		{
			int pos;
			pos = SendMessage(GetDlgItem(hWnd, IDC_TRANSPARENCY), TBM_GETPOS, 0, 0);
			TCHAR temp[255];
			wsprintf(temp, TEXT("%d%%"), pos);
			SetWindowText(GetDlgItem(hWnd, IDC_TRANSPARENCY_LABEL), temp);
			SendMessage(GetParent(hWnd), PSM_CHANGED, (WPARAM)hWnd, 0);
		}
		break;
	case WM_COMMAND:
		if(HIWORD(wParam) == EN_CHANGE || HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == CBN_SELCHANGE)
			SendMessage(GetParent(hWnd), PSM_CHANGED, (WPARAM)hWnd, 0);
		break;
	case WM_NOTIFY:
		if(((LPNMHDR)lParam)->code == PSN_APPLY)
		{
			Setting->nLine = SendMessage(GetDlgItem(hWnd, IDC_NLINESPIN), UDM_GETPOS32, NULL, NULL);
			Setting->LineMargin = SendMessage(GetDlgItem(hWnd, IDC_MARGINSPIN), UDM_GETPOS32, NULL, NULL);
			if(bOuter)
			{
				cfg_outer_transparency = SendMessage(GetDlgItem(hWnd, IDC_TRANSPARENCY), TBM_GETPOS, 0, 0);
				cfg_outer_layered = (IsDlgButtonChecked(hWnd, IDC_LAYERED) ? true : false);
				cfg_outer_border = (IsDlgButtonChecked(hWnd, IDC_BORDER) ? true : false);
				UpdateOuterWindowStyle(hParent);
			}

			Setting->VerticalAlign = (BYTE)SendMessage(GetDlgItem(hWnd, IDC_VERTICALALIGN), CB_GETCURSEL, NULL, NULL);
			Setting->HorizentalAlign = (BYTE)SendMessage(GetDlgItem(hWnd, IDC_HORIZENTALALIGN), CB_GETCURSEL, NULL, NULL);
//			if(Setting->Script)
//				uGetDlgItemText(hWnd, IDC_UISCRIPT, *(Setting->Script));

			SetWindowLong(hWnd, DWL_MSGRESULT, PSNRET_NOERROR);
		}
		else if(((LPNMHDR)lParam)->code == PSN_KILLACTIVE)
		{
			SetWindowLong(hWnd, DWL_MSGRESULT, FALSE);
		}
		else if(((LPNMHDR)lParam)->code == PSN_RESET)
		{
			memcpy(Setting, &OldSetting, sizeof(Window_Setting));
			if(bOuter)
			{
				cfg_outer_transparency = old_transparency;
				cfg_outer_layered = old_layered;
				cfg_outer_border = old_border;

				UpdateOuterWindowStyle(hParent);
			}
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

void StartUIConfigDialog(Window_Setting *Setting, HWND hParent, BOOL bOuter)
{
	HPROPSHEETPAGE pages[1];
	PROPSHEETPAGE page;
	page.dwSize = sizeof(PROPSHEETPAGE);
	page.dwFlags = PSP_DEFAULT;
	page.hInstance = core_api::get_my_instance();
	page.pszTemplate = MAKEINTRESOURCE(IDD_UI_PREF_COMMON);
	page.pfnDlgProc = UICommonConfigProc;
	page.lParam = (LPARAM)
				(new pair<Window_Setting *, pair<BOOL, HWND> *>
				(Setting, 
				new pair<BOOL, HWND>(bOuter, hParent)));
	pages[0] = CreatePropertySheetPage(&page);

	PROPSHEETHEADER psh;
	psh.dwSize = sizeof(psh);
	psh.dwFlags = PSH_DEFAULT | PSH_NOCONTEXTHELP | PSH_USEHICON;
	psh.hwndParent = hParent;
	psh.hInstance = core_api::get_my_instance();
	psh.hIcon = static_api_ptr_t<ui_control>()->get_main_icon();
	psh.pszCaption = TEXT("고급 설정");
	psh.nPages = 1;
	psh.nStartPage = 0;
	psh.phpage = pages;

	int nRet = PropertySheet(&psh);
}
