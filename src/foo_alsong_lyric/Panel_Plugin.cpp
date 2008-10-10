#include "stdafx.h"

#include "resource.h"
#include "Panel_Plugin.h"
#include "Common_Settings.h"
#include "Common_UI.h"
#include "Common_Pref.h"
#include "Common_Lyric_Modify_Dialog.h"

//TODO: 배경색 없음(기본) 추가

using namespace Gdiplus;

Alsong_Panel::class_data & Alsong_Panel::get_class_data() const 
{
	__implement_get_class_data_ex(_T("{E859B366-AF66-45f6-9BE1-234FD363825F}"), _T("Alsong Live Lyric"), true, 0, WS_CHILD | WS_CLIPCHILDREN, WS_EX_CONTROLPARENT, CS_DBLCLKS);
}

const GUID &Alsong_Panel::get_extension_guid() const
{
	return g_extension_guid;
}

void Alsong_Panel::get_name(pfc::string_base & out) const
{
	out.set_string("Alsong Lyric Panel");
}

void Alsong_Panel::get_category(pfc::string_base & out) const
{
	out.set_string("Panels");
}

unsigned Alsong_Panel::get_type() const
{
	return uie::type_panel;
}

void Alsong_Panel::OnContextMenu(HWND hParent)
{
	// We need some IDs for the context menu.
	enum 
	{
		ID_FONT = 1,
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
			pt.x, pt.y, 0, hParent, 0);

		// Check what command has been chosen. If cmd == 0, then no command
		// was chosen.
		if (cmd == ID_FONT) {
			// Show font configuration.
			t_font_description font = cfg_panel.get_value().font;
			if (font.popup_dialog(hParent)) {
				cfg_panel.get_value().font = font;
				InvalidateRect(hParent, NULL, TRUE);
			}
		}
		else if(cmd == ID_BKCOLOR)
		{			
			CHOOSECOLOR choosecolor;
			ZeroMemory(&choosecolor, sizeof(choosecolor));
			choosecolor.lStructSize = sizeof(CHOOSECOLOR);
			choosecolor.Flags = CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT ;
			choosecolor.hwndOwner = hParent;
			choosecolor.lpCustColors = (LPDWORD)acrCustClr;
			choosecolor.rgbResult = cfg_panel.get_value().bkColor;
			if(ChooseColor(&choosecolor))
			{
				cfg_panel.get_value().bgType = false;
				cfg_panel.get_value().bkColor = choosecolor.rgbResult;
				InvalidateRect(hParent, NULL, TRUE);
			}
		}
		else if(cmd == ID_FGCOLOR)
		{
			CHOOSECOLOR choosecolor;
			ZeroMemory(&choosecolor, sizeof(choosecolor));
			choosecolor.lStructSize = sizeof(CHOOSECOLOR);
			choosecolor.Flags = CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT ;
			choosecolor.hwndOwner = hParent;
			choosecolor.lpCustColors = (LPDWORD)acrCustClr;
			choosecolor.rgbResult = cfg_panel.get_value().fgColor;
			if(ChooseColor(&choosecolor))
			{
				cfg_panel.get_value().fgColor = choosecolor.rgbResult;
				InvalidateRect(hParent, NULL, TRUE);
			}
		}
		else if(cmd == ID_BGIMAGE)
		{
			OPENFILENAME ofn;
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.lpstrFile = cfg_panel.get_value().bgImage;
			ofn.lpstrFilter = TEXT("그림 파일(*.bmp;*.png;*.jpg;*.gif;*.jpeg)\0*.bmp;*.png;*.jpg;*.gif;*.jpeg\0\0");
			ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
			ofn.nMaxFile = 255;

			if(GetOpenFileName(&ofn))
			{
				cfg_panel.get_value().bgType = true;
				InvalidateRect(hParent, NULL, TRUE);
			}
		}
		else if(cmd == ID_ADVSET)
		{
			StartUIConfigDialog(&(cfg_panel.get_value()), hParent, FALSE);
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
			OpenLyricModifyDialog(hParent, (CHAR *)str1.get_ptr(), (CHAR *)str.get_ptr(), (CHAR *)handle->get_path(), len);
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

LRESULT Alsong_Panel::on_message(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	switch(iMessage)
	{
		case WM_CONTEXTMENU:
		case WM_NCRBUTTONUP:
		case WM_RBUTTONUP:
			OnContextMenu(hWnd);
			return 0;
	}
	return Common_UI->Process_Message(hWnd, iMessage, wParam, lParam, &(cfg_panel.get_value()), FALSE);
}

// {E859B366-AF66-45f6-9BE1-234FD363825F}

const GUID Alsong_Panel::g_extension_guid = { 0xe859b366, 0xaf66, 0x45f6, { 0x98, 0xe1, 0x23, 0x4f, 0xd3, 0x63, 0x82, 0x5f } };

uie::window_factory<Alsong_Panel> g_lyric_window_factory;

void Alsong_Panel::get_menu_items (uie::menu_hook_t & p_hook) 
{
	
}

void Alsong_Panel::set_config(stream_reader * p_reader, t_size p_size, abort_callback & p_abort)
{
	Alsong_Setting setting;
	if(p_size == sizeof(Alsong_Setting))
		p_reader->read(&setting, p_size, p_abort);
	else
		return;
	cfg_panel = setting;
}

void Alsong_Panel::get_config(stream_writer * p_writer, abort_callback & p_abort) const
{
	p_writer->write(&cfg_panel.get_value(), sizeof(Alsong_Setting), p_abort);
}

bool Alsong_Panel::have_config_popup() const
{
	return true;
}

bool Alsong_Panel::show_config_popup(HWND wnd_parent)
{
	StartUIConfigDialog(&cfg_panel.get_value(), wnd_parent, FALSE);
	return true;
}