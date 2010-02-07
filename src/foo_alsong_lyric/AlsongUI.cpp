#include "stdafx.h"
#include "ConfigStore.h"
#include "LyricManager.h"
#include "AlsongUI.h"

//TODO: DropSource

COLORREF acrCustClr[16] = {RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), 
RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), 
RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), 
RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255)};

using namespace Gdiplus;

AlsongUI::AlsongUI(Window_Setting &Setting, pfc::string8 &Script) : m_Setting(Setting), m_Script(Script)
{
}

AlsongUI::~AlsongUI()
{
}

LRESULT AlsongUI::ProcessMessage(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	switch(iMessage)
	{
	case WM_CREATE:
		LyricManagerInstance->AddRedrawHandler(boost::bind(InvalidateRect, hWnd, (const RECT *)NULL, TRUE));
		break;
	case WM_CONTEXTMENU:
	case WM_NCRBUTTONUP:
		on_contextmenu(hWnd);
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		on_keydown(wParam);
		return 0;

	case WM_DESTROY:
		return 0;

	case WM_PAINT:
		PAINTSTRUCT ps;
		if(BeginPaint(hWnd, &ps) != NULL) 
		{
			Draw(ps.hdc);
			EndPaint(hWnd, &ps);
		}
		return 0;
	}
	return DefWindowProc(hWnd, iMessage, wParam, lParam);
}

void AlsongUI::Draw(HDC hdc)
{
	const char *nowlrc = LyricManagerInstance->GetLyric();
	if(!nowlrc)
		return;
	std::wstring nowlrcw = pfc::stringcvt::string_wide_from_utf8_fast(nowlrc);

	TextOut(hdc, 0, 0, nowlrcw.c_str(), nowlrcw.length());
	//DrawText(hdc, nowlrcw.c_str(), nowlrcw.length(), NULL, NULL);
}

void AlsongUI::ShowConfig(HWND hWndParent)
{

}

SquirrelVMSys AlsongUI::InitializeScript()
{
	SquirrelVMSys v;
	SquirrelVM::Init();
	SquirrelVM::GetVMSys(v);

	return v;
}

void AlsongUI::UnInitializeScript(SquirrelVMSys *vm)
{
	SquirrelVM::SetVMSys(*vm);
	//SquirrelVM::Shutdown();
}

bool AlsongUI::on_keydown(WPARAM wParam) 
{
	bool rv = false;

	try 
	{
		metadb_handle_list items;
		static_api_ptr_t<play_control> pc;
		metadb_handle_ptr handle;
		if (pc->get_now_playing(handle)) 
			items.add_item(handle);

		static_api_ptr_t<keyboard_shortcut_manager> ksm;
		rv = ksm->on_keydown_auto_context(items, wParam, contextmenu_item::caller_undefined);
	}
	catch (const exception_service_not_found &) 
	{

	}

	return rv;
}

void AlsongUI::on_contextmenu(HWND hWndFrom)
{//TODO:메뉴 다 지우고 설정만
	enum 
	{
		ID_FONT = 1,
		ID_TOPMOST,
		ID_BKCOLOR,
		ID_FGCOLOR,
		ID_BGIMAGE,
		ID_ADVSET,
		ID_MODLRC,
		ID_CONTEXT_FIRST,
		ID_CONTEXT_LAST = ID_CONTEXT_FIRST + 1000,
	};

	HMENU hMenu = CreatePopupMenu();
	AppendMenu(hMenu, MF_STRING, ID_FONT, TEXT("폰트 선택..."));
	if(cfg_outer_topmost)
		AppendMenu(hMenu, MF_STRING | MF_CHECKED, ID_TOPMOST, TEXT("항상 위에 보이기"));
	else
		AppendMenu(hMenu, MF_STRING, ID_TOPMOST, TEXT("항상 위에 보이기"));
	AppendMenu(hMenu, MF_STRING, ID_BKCOLOR, TEXT("배경색 선택..."));
	AppendMenu(hMenu, MF_STRING, ID_FGCOLOR, TEXT("글자색 선택..."));
	AppendMenu(hMenu, MF_STRING, ID_BGIMAGE, TEXT("배경그림 선택..."));
	AppendMenu(hMenu, MF_STRING, ID_ADVSET, TEXT("고급 설정..."));
	AppendMenu(hMenu, MF_STRING, ID_MODLRC, TEXT("가사 추가/변경..."));

	try 
	{
		metadb_handle_list items;
		static_api_ptr_t<play_control> pc;
		metadb_handle_ptr handle;
		if (pc->get_now_playing(handle))
			items.add_item(handle);
		service_ptr_t<contextmenu_manager> cmm;
		contextmenu_manager::g_create(cmm);
		const bool show_shortcuts = config_object::g_get_data_bool_simple(standard_config_objects::bool_show_keyboard_shortcuts_in_menus, false);
		unsigned flags = show_shortcuts ? contextmenu_manager::FLAG_SHOW_SHORTCUTS : 0;
		cmm->init_context(items, flags);
		if (cmm->get_root()) 
		{
			uAppendMenu(hMenu, MF_SEPARATOR, 0, 0);
			cmm->win32_build_menu(hMenu, ID_CONTEXT_FIRST, ID_CONTEXT_LAST);
		}

		menu_helpers::win32_auto_mnemonics(hMenu);

		POINT pt;
		GetCursorPos(&pt);
		int cmd = TrackPopupMenu(hMenu, TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON, 
			pt.x, pt.y, 0, hWndFrom, 0);

		if (cmd == ID_FONT) {
			t_font_description font = cfg_outer.get_value().font;
			if (font.popup_dialog(hWndFrom)) 
			{
				cfg_outer.get_value().font = font;
				InvalidateRect(hWndFrom, NULL, TRUE);
			}
		} 
		else if(cmd == ID_TOPMOST)
		{
			cfg_outer_topmost = !cfg_outer_topmost;

			SetWindowLong(hWndFrom, GWL_EXSTYLE, (cfg_outer_topmost ? GetWindowLong(hWndFrom, GWL_EXSTYLE) | WS_EX_TOPMOST : GetWindowLong(hWndFrom, GWL_EXSTYLE) & ~WS_EX_TOPMOST));
			SetWindowPos(hWndFrom, (cfg_outer_topmost ? HWND_TOPMOST : HWND_NOTOPMOST), 0, 0, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOMOVE | SWP_FRAMECHANGED);
		}
		else if(cmd == ID_BKCOLOR)
		{			
			CHOOSECOLOR choosecolor;
			ZeroMemory(&choosecolor, sizeof(choosecolor));
			choosecolor.lStructSize = sizeof(CHOOSECOLOR);
			choosecolor.Flags = CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT ;
			choosecolor.hwndOwner = hWndFrom;
			choosecolor.lpCustColors = (LPDWORD)acrCustClr;
			choosecolor.rgbResult = cfg_outer.get_value().bkColor;
			if(ChooseColor(&choosecolor))
			{
				cfg_outer.get_value().bgType = 0;
				cfg_outer.get_value().bkColor = choosecolor.rgbResult;
				InvalidateRect(hWndFrom, NULL, TRUE);
			}
		}
		else if(cmd == ID_FGCOLOR)
		{
			CHOOSECOLOR choosecolor;
			ZeroMemory(&choosecolor, sizeof(choosecolor));
			choosecolor.lStructSize = sizeof(CHOOSECOLOR);
			choosecolor.Flags = CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT ;
			choosecolor.hwndOwner = hWndFrom;
			choosecolor.lpCustColors = (LPDWORD)acrCustClr;
			choosecolor.rgbResult = cfg_outer.get_value().fgColor;
			if(ChooseColor(&choosecolor))
			{
				cfg_outer.get_value().fgColor = choosecolor.rgbResult;
				InvalidateRect(hWndFrom, NULL, TRUE);
			}
		}
		else if(cmd == ID_BGIMAGE)
		{
			OPENFILENAME ofn;
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.lpstrFile = cfg_outer.get_value().bgImage;
			ofn.lpstrFilter = TEXT("그림 파일(*.bmp;*.png;*.jpg;*.gif;*.jpeg)\0*.bmp;*.png;*.jpg;*.gif;*.jpeg\0\0");
			ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
			ofn.nMaxFile = 255;

			if(GetOpenFileName(&ofn))
			{
				cfg_outer.get_value().bgType = 1;
				InvalidateRect(hWndFrom, NULL, TRUE);
			}
		}
		else if(cmd == ID_ADVSET)
		{
			//TODO
		}
		else if(cmd == ID_MODLRC)
		{
			if(handle == NULL)
				return;
			//TODO
		}
		else if (cmd >= ID_CONTEXT_FIRST && cmd <= ID_CONTEXT_LAST ) 
			cmm->execute_by_id(cmd - ID_CONTEXT_FIRST);

	}
	catch (const exception_service_not_found &) {
	}
	DestroyMenu(hMenu);
}

