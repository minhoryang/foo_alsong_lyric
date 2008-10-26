#include "stdafx.h"
#include "Outer_Window_Plugin.h"
#include "Common_Settings.h"
#include "Common_UI.h"
#include "Common_Pref.h"
#include "Common_Lyric_Modify_Dialog.h"

Outer_Window_Plugin g_OuterWindow; //외부 윈도우는 하나다.

class menu_command_alsong : public mainmenu_commands 
{
	// Return the number of commands we provide.
	virtual t_uint32 get_command_count() 
	{
		return 1;
	}

	// All commands are identified by a GUID.
	virtual GUID get_command(t_uint32 p_index) 
	{
		static const GUID guid_alsong_lyric_menu = { 0x66821da5, 0xfa15, 0x4002, { 0x99, 0xc5, 0x8c, 0x6a, 0x96, 0xf4, 0xe, 0xad } };

		if (p_index == 0)
			return guid_alsong_lyric_menu;
		return pfc::guid_null;
	}

	// Set p_out to the name of the n-th command.
	// This name is used to identify the command and determines
	// the default position of the command in the menu.
	virtual void get_name(t_uint32 p_index, pfc::string_base & p_out) 
	{
		if (p_index == 0)
		{
			CHAR temp[255];
			pfc::stringcvt::convert_wide_to_utf8(temp, 255, TEXT("알송 실시간 가사"), lstrlen(TEXT("알송 실시간 가사")));
			p_out = temp;
		}
	}

	// Set p_out to the description for the n-th command.
	virtual bool get_description(t_uint32 p_index, pfc::string_base & p_out) 
	{
		if (p_index == 0)
		{
			CHAR temp[255];
			pfc::stringcvt::convert_wide_to_utf8(temp, 255, TEXT("알송 실시간 가사 창을 열거나 닫습니다."), lstrlen(TEXT("알송 실시간 가사 창을 열거나 닫습니다.")));
			p_out = temp;
		}
		else
			return false;
		return true;
	}

	// Every set of commands needs to declare which group it belongs to.
	virtual GUID get_parent()
	{
		return mainmenu_groups::file_etc_preferences;
	}

	// Execute n-th command.
	// p_callback is reserved for future use.
	virtual void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback) 
	{
		if (p_index == 0 && core_api::assert_main_thread()) 
		{
			if (cfg_outer_shown)
			{
				g_OuterWindow.Hide();
			}
			else
			{
				g_OuterWindow.Show();
			}
		}
	}

	// The standard version of this command does not support checked or disabled
	// commands, so we use our own version.
	virtual bool get_display(t_uint32 p_index, pfc::string_base & p_text, t_uint32 & p_flags)
	{
		p_flags = 0;
		if (is_checked(p_index))
			p_flags |= flag_checked;
		get_name(p_index,p_text);
		return true;
	}

	// Return whether the n-th command is checked.
	bool is_checked(t_uint32 p_index)
	{
		if (p_index == 0)
			return cfg_outer_shown;
		return false;
	}
};

// We need to create a service factory for our menu item class,
// otherwise the menu commands won't be known to the system.
static mainmenu_commands_factory_t< menu_command_alsong > foo_menu;

HWND Outer_Window_Plugin::Create() 
{
	assert(m_hWnd == NULL);

	WNDCLASS wc;
	memset(&wc, 0, sizeof(wc));
	wc.style = CS_DBLCLKS;
	wc.lpfnWndProc = &WindowProc;
	wc.hInstance = NULL;
	wc.hIcon = static_api_ptr_t<ui_control>()->get_main_icon();
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszClassName = TEXT("AlsongLyricWindow");
	wc.cbWndExtra = sizeof(Outer_Window_Plugin *);
	RegisterClass(&wc);
	
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
	SetLayeredWindowAttributes(m_hWnd, 0, (255 * cfg_outer_transparency) / 100, LWA_ALPHA);//TODO: 어디로 옮길까
	ShowWindow(m_hWnd, SW_HIDE);
	HMENU hMenu = GetSystemMenu(m_hWnd, FALSE);
	AppendMenu(hMenu, MF_STRING, 1000, TEXT("고급 설정..."));
	
	return m_hWnd;
}

void Outer_Window_Plugin::Destroy() 
{
	// Destroy the window.
	if (m_hWnd) 
	{
		DestroyWindow(m_hWnd);
	}
}

void Outer_Window_Plugin::Show()
{
	ShowWindow(m_hWnd, SW_SHOW);
	cfg_outer_shown = true;
}

void Outer_Window_Plugin::Hide()
{
	ShowWindow(m_hWnd, SW_HIDE);
	cfg_outer_shown = false;
}

LRESULT CALLBACK Outer_Window_Plugin::WindowProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	switch(iMessage)
	{
	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		cfg_outer_shown = false;
		return 0;
	case WM_CONTEXTMENU:
	case WM_NCRBUTTONUP:
		g_OuterWindow.OnContextMenu(hWnd);
		return 0;
	case WM_SYSCOMMAND:
		if(wParam == 1000)
			StartUIConfigDialog(&(cfg_outer.get_value()), hWnd, TRUE);

		break;
	case WM_NCHITTEST:
		{
			LRESULT ret = DefWindowProc(hWnd, iMessage, wParam, lParam);
			if(ret == HTCLIENT)
				return HTCAPTION;
			return ret;
		}
	case WM_NCPAINT:
		break;
	case WM_DESTROY:
		cfg_outer_window_placement.on_window_destruction(hWnd);
		break;
	case WM_CREATE:
		cfg_outer_window_placement.on_window_creation(hWnd);
		break;
	case WM_MOVE:
	case WM_SIZE:
		{
			if(IsMaximized(hWnd))
				break;
			RECT rt;
			GetWindowRect(hWnd, &rt);
			int ScrWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN); //화면크기 구하기
			int ScrHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
			int t = 0;//TODO:Test
			int x, y;
			x = rt.left;
			y = rt.top;

			if(rt.bottom < 0)
			{
				x = 0;
				t = 1;
			}
			if(rt.right < 0)
			{
				y = 0;
				t = 1;
			}
			if(rt.left > ScrWidth - 10)
			{
				x = ScrWidth - 10;
				t = 1;
			}
			if(rt.top > ScrHeight - 10)
			{
				y = ScrHeight - 10;
				t = 1;
			}
			if(t == 1)
			{
				SetWindowPos(hWnd, NULL, x, y, 500, 200, SWP_NOZORDER);
			}
		}
		break;
	}

	return Common_UI->Process_Message(hWnd, iMessage, wParam, lParam, &(cfg_outer.get_value()), TRUE);
}

void Outer_Window_Plugin::OnContextMenu(HWND hWndFrom) 
{ //TODO:메뉴 다 지우고 설정만
	// We need some IDs for the context menu.
	enum 
	{
		// ID for "Choose font..."
		ID_FONT = 1,
		ID_TOPMOST,
		ID_BKCOLOR,
		ID_FGCOLOR,
		ID_BGIMAGE,
		ID_ADVSET,
		ID_MODLRC,
		// The range ID_CONTEXT_FIRST through ID_CONTEXT_LAST is reserved
		// for menu entries from menu_manager.
		ID_CONTEXT_FIRST,
		ID_CONTEXT_LAST = ID_CONTEXT_FIRST + 1000,
	};

	// Create new popup menu.
	HMENU hMenu = CreatePopupMenu();

	// Add our "Choose font..." command.
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

	try {
		// Get the currently playing track.
		metadb_handle_list items;
		static_api_ptr_t<play_control> pc;
		metadb_handle_ptr handle;
		if (pc->get_now_playing(handle)) {
			// Insert it into a list.
			items.add_item(handle);
		}

		// Create a menu_manager that will build the context menu.
		service_ptr_t<contextmenu_manager> cmm;
		contextmenu_manager::g_create(cmm);
		// Query setting for showing keyboard shortcuts.
		const bool show_shortcuts = config_object::g_get_data_bool_simple(standard_config_objects::bool_show_keyboard_shortcuts_in_menus, false);
		// Set up flags for contextmenu_manager::init_context.
		unsigned flags = show_shortcuts ? contextmenu_manager::FLAG_SHOW_SHORTCUTS : 0;
		// Initialize menu_manager for using a context menu.
		cmm->init_context(items, flags);
		// If the menu_manager has found any applicable commands,
		// add them to our menu (after a separator).
		if (cmm->get_root()) {
			uAppendMenu(hMenu, MF_SEPARATOR, 0, 0);
			cmm->win32_build_menu(hMenu, ID_CONTEXT_FIRST, ID_CONTEXT_LAST);
		}

		// Use menu helper to gnereate mnemonics.
		menu_helpers::win32_auto_mnemonics(hMenu);

		// Get the location of the mouse pointer.
		// WM_CONTEXTMENU provides position of mouse pointer in argument lp,
		// but this isn't reliable (for example when the user pressed the
		// context menu key on the keyboard).
		POINT pt;
		GetCursorPos(&pt);
		// Show the context menu.
		int cmd = TrackPopupMenu(hMenu, TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON, 
			pt.x, pt.y, 0, hWndFrom, 0);

		// Check what command has been chosen. If cmd == 0, then no command
		// was chosen.
		if (cmd == ID_FONT) {
			// Show font configuration.
			t_font_description font = cfg_outer.get_value().font;
			if (font.popup_dialog(hWndFrom)) {
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
				cfg_outer.get_value().bgType = false;
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
				cfg_outer.get_value().bgType = true;
				InvalidateRect(hWndFrom, NULL, TRUE);
			}
		}
		else if(cmd == ID_ADVSET)
		{
			StartUIConfigDialog(&(cfg_outer.get_value()), hWndFrom, TRUE);
		}
		else if(cmd == ID_MODLRC)
		{
			if(handle == NULL)
				return;

			service_ptr_t<titleformat_object> to;
			pfc::string8 str;
			pfc::string8 str1;

			static_api_ptr_t<titleformat_compiler>()->compile_safe(to, "%artist%");
			handle->format_title(NULL, str, to, NULL);
			static_api_ptr_t<titleformat_compiler>()->compile_safe(to, "%title%");
			handle->format_title(NULL, str1, to, NULL);

			double len = handle->get_length();
			OpenLyricModifyDialog(hWndFrom, (CHAR *)str1.get_ptr(), (CHAR *)str.get_ptr(), (CHAR *)handle->get_path(), len);
		}
		else if (cmd >= ID_CONTEXT_FIRST && cmd <= ID_CONTEXT_LAST ) {
			// Let the menu_manager execute the chosen command.
			cmm->execute_by_id(cmd - ID_CONTEXT_FIRST);
		}

		// contextmenu_manager instance is released automatically, as is the metadb_handle we used.
	}
	catch (const exception_service_not_found &) {
	}

	// Finally, destroy the popup menu.
	DestroyMenu(hMenu);
}

Outer_Window_Plugin::Outer_Window_Plugin()
{

}