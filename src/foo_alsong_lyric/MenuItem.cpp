#include "stdafx.h"
#include "ConfigStore.h"
#include "AlsongWnd.h"
#include "LyricManager.h"

class menu_command_alsong : public mainmenu_commands 
{
	virtual t_uint32 get_command_count() 
	{
		return 1;
	}

	virtual GUID get_command(t_uint32 p_index) 
	{
		static const GUID guid_alsong_lyric_menu = { 0x66821da5, 0xfa15, 0x4002, { 0x99, 0xc5, 0x8c, 0x6a, 0x96, 0xf4, 0xe, 0xad } };

		if(p_index == 0)
			return guid_alsong_lyric_menu;
		return pfc::guid_null;
	}

	virtual void get_name(t_uint32 p_index, pfc::string_base & p_out) 
	{
		if(p_index == 0)
			p_out = pfc::stringcvt::string_utf8_from_wide(TEXT("알송 실시간 가사"), lstrlen(TEXT("알송 실시간 가사")));
	}

	virtual bool get_description(t_uint32 p_index, pfc::string_base & p_out) 
	{
		if(p_index == 0)
			p_out = pfc::stringcvt::string_utf8_from_wide(TEXT("알송 실시간 가사 창을 열거나 닫습니다."), lstrlen(TEXT("알송 실시간 가사 창을 열거나 닫습니다.")));
		else
			return false;
		return true;
	}

	virtual GUID get_parent()
	{
		return mainmenu_groups::file_etc_preferences;
	}

	virtual void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback) 
	{
		if(p_index == 0 && core_api::assert_main_thread()) 
		{
			if (cfg_outer_shown)
				AlsongWndInstance.Hide();
			else
				AlsongWndInstance.Show();
		}
	}

	virtual bool get_display(t_uint32 p_index, pfc::string_base & p_text, t_uint32 & p_flags)
	{
		p_flags = 0;
		if(is_checked(p_index))
			p_flags |= flag_checked;
		get_name(p_index, p_text);
		return true;
	}

	bool is_checked(t_uint32 p_index)
	{
		if(p_index == 0)
			return cfg_outer_shown;
		return false;
	}
};

static mainmenu_commands_factory_t<menu_command_alsong> alsong_main_menu;

class contextcommand_alsong : public contextmenu_item_simple 
{
public:
	virtual GUID get_parent()
	{
		return contextmenu_groups::root;
	}

	virtual unsigned int get_num_items() 
	{
		return 1;
	}

	virtual void get_item_name(unsigned int p_index, pfc::string_base & p_out) 
	{
		if(p_index == 0)
			p_out = pfc::stringcvt::string_utf8_from_wide(TEXT("알송 가사 추가/변경"), lstrlen(TEXT("알송 가사 추가/변경")));
	}

	virtual void context_command(unsigned int p_index, metadb_handle_list_cref p_data, const GUID& p_caller) 
	{
		if(p_index == 0 && p_data.get_count() == 1)
			LyricManager::OpenLyricModifyDialog(core_api::get_main_window(), p_data.get_item(0));
	}
	
	virtual bool context_get_display(unsigned int p_index, metadb_handle_list_cref p_data, pfc::string_base & p_out, unsigned & p_displayflags, const GUID & p_caller) 
	{
		if(p_index == 0)
		{
			p_out = pfc::stringcvt::string_utf8_from_wide(TEXT("알송 가사 추가/변경"), lstrlen(TEXT("알송 가사 추가/변경")));
			return p_data.get_count() == 1;
		}
		return false;
	}

	virtual GUID get_item_guid(unsigned int p_index) 
	{
		if(p_index == 0)
		{
			static const GUID item_guid = // {6C1CBA98-CB52-4ECA-92B6-7A8625DE1A6F}
				{ 0x6c1cba98, 0xcb52, 0x4eca, { 0x92, 0xb6, 0x7a, 0x86, 0x25, 0xde, 0x1a, 0x6f } };
			return item_guid;
		}
		static const GUID nullguid = {0,};
		return nullguid;
	}

	virtual bool get_item_description(unsigned int p_index, pfc::string_base & p_out) 
	{
		if(p_index == 0)
		{
			p_out = pfc::stringcvt::string_utf8_from_wide(TEXT("알송 가사 추가/변경"), lstrlen(TEXT("알송 가사 추가/변경")));
			return true;
		}
		return false;
	}
};

static contextmenu_item_factory_t<contextcommand_alsong> g_myitem_factory;
