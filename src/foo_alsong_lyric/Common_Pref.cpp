#include "stdafx.h"
#include "Common_Settings.h"
#include "resource.h"
#include "Common_Pref.h"

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

		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_LOADFROMLRC:
			if(HIWORD(wParam) == BN_CLICKED)
			{
				cfg_load_from_lrc = (IsDlgButtonChecked(hWnd, IDC_LOADFROMLRC) ? true : false);
				//CheckDlgButton(hWnd, IDC_LOADFROMLRC, !IsDlgButtonChecked(hWnd, IDC_LOADFROMLRC));
			}
			break;
		case IDC_SAVELRC:
			if(HIWORD(wParam) == BN_CLICKED)
			{
				cfg_save_to_lrc = (IsDlgButtonChecked(hWnd, IDC_SAVELRC) ? true : false);
				//CheckDlgButton(hWnd, IDC_SAVELRC, !IsDlgButtonChecked(hWnd, IDC_SAVELRC));
			}
			break;
		case IDC_LRCPATH:
			if(HIWORD(wParam) == EN_CHANGE)
			{
				cfg_lrc_save_path = uGetDlgItemText(hWnd, IDC_LRCPATH).get_ptr();
			}
			break;
		case IDC_PANELSET:
			if(HIWORD(wParam) == BN_CLICKED)
			{
				StartUIConfigDialog(&cfg_panel.get_value(), hWnd, FALSE);
			}
			break;
		case IDC_OUTERSET:
			if(HIWORD(wParam) == BN_CLICKED)
			{
				StartUIConfigDialog(&cfg_outer.get_value(), hWnd, TRUE);
			}
			break;
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
		//TODO: Implement
	}

	virtual HWND create(HWND parent)
	{
		return uCreateDialog(IDD_COMMON_PREF, parent, ConfigProc, 0);
	}
};

static preferences_page_factory_t<preferences_page_alsong_lyric> foo_preferences_page_alsong_lyric;

//TODO: 줄간격 설정
static BOOL CALLBACK UIConfigProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	static Alsong_Setting *Setting;
	static BOOL bOuter = FALSE;
	switch (iMessage)
	{
	case WM_INITDIALOG:
		{
			Setting = ((pair<Alsong_Setting *, BOOL> *)lParam)->first;
			bOuter = ((pair<Alsong_Setting *, BOOL> *)lParam)->second;
			SendMessage(GetDlgItem(hWnd, IDC_NLINESPIN), UDM_SETRANGE32, 1, 20);
			SendMessage(GetDlgItem(hWnd, IDC_NLINESPIN), UDM_SETBUDDY, (WPARAM)GetDlgItem(hWnd, IDC_NLINE), 0);
			SendMessage(GetDlgItem(hWnd, IDC_NLINESPIN), UDM_SETPOS32, 0, Setting->nLine);
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
		}
		break;
	case WM_CLOSE:
		EndDialog(hWnd, 1);
		break;
	case WM_NOTIFY:
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
			Setting->nLine = SendMessage(GetDlgItem(hWnd, IDC_NLINESPIN), UDM_GETPOS32, NULL, NULL);
			if(bOuter)
			{
				cfg_outer_transparency = SendMessage(GetDlgItem(hWnd, IDC_TRANSPARENCY), TBM_GETPOS, 0, 0);
				cfg_outer_layered = (IsDlgButtonChecked(hWnd, IDC_LAYERED) ? true : false);
				cfg_outer_border = (IsDlgButtonChecked(hWnd, IDC_BORDER) ? true : false);
			}
			EndDialog(hWnd, 0);
			break;
		case IDCANCEL:
			EndDialog(hWnd, 1);
			break;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

void StartUIConfigDialog(Alsong_Setting *Setting, HWND hParent, BOOL bOuter)
{
	Alsong_Setting SettingTemp;
	memcpy(&SettingTemp, Setting, sizeof(Alsong_Setting));
	int nRet = DialogBoxParam(core_api::get_my_instance(), MAKEINTRESOURCE(IDD_UI_PREF), hParent, UIConfigProc, (LPARAM)&(make_pair(&SettingTemp, bOuter)));
	if(nRet == 0)
	{
		//성공
		if(bOuter == TRUE)
		{
			SetWindowLong(hParent, GWL_STYLE, (cfg_outer_border ? WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX : WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX));
			//TODO: 작업 표시줄, Alt+Tab에서 없애기

			//100%투명도 아닐경우에만 적용. 항상위 강제
			if(cfg_outer_layered == true && cfg_outer_transparency != 100)
			{
				SetWindowLong(hParent, GWL_EXSTYLE, GetWindowLong(hParent, GWL_EXSTYLE) | WS_EX_TRANSPARENT | WS_EX_TOPMOST);
				SetWindowPos(hParent, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOMOVE | SWP_FRAMECHANGED);
			}
			else
			{
				SetWindowLong(hParent, GWL_EXSTYLE, GetWindowLong(hParent, GWL_EXSTYLE) & ~WS_EX_TRANSPARENT & (cfg_outer_topmost ? 0xFFFFFFFF : ~WS_EX_TOPMOST));
				SetWindowPos(hParent, (cfg_outer_topmost ? HWND_TOPMOST : HWND_NOTOPMOST), 0, 0, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOMOVE | SWP_FRAMECHANGED);
			}
			SetLayeredWindowAttributes(hParent, NULL, (255 * cfg_outer_transparency) / 100, LWA_ALPHA);
		}
		memcpy(Setting, &SettingTemp, sizeof(Alsong_Setting));
		InvalidateRect(hParent, NULL, TRUE);
	}
}
