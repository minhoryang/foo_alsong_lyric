#include "stdafx.h"
#include "ConfigStore.h"
#include "AlsongWnd.h"

class menu_command_alsong : public mainmenu_commands 
{
	virtual t_uint32 get_command_count() 
	{
		return 1;
	}

	virtual GUID get_command(t_uint32 p_index) 
	{
		static const GUID guid_alsong_lyric_menu = { 0x66821da5, 0xfa15, 0x4002, { 0x99, 0xc5, 0x8c, 0x6a, 0x96, 0xf4, 0xe, 0xad } };

		if (p_index == 0)
			return guid_alsong_lyric_menu;
		return pfc::guid_null;
	}

	virtual void get_name(t_uint32 p_index, pfc::string_base & p_out) 
	{
		if (p_index == 0)
		{
			CHAR temp[255];
			pfc::stringcvt::convert_wide_to_utf8(temp, 255, TEXT("알송 실시간 가사"), lstrlen(TEXT("알송 실시간 가사")));
			p_out = temp;
		}
	}

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

	virtual GUID get_parent()
	{
		return mainmenu_groups::file_etc_preferences;
	}

	virtual void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback) 
	{
		if (p_index == 0 && core_api::assert_main_thread()) 
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
		if (is_checked(p_index))
			p_flags |= flag_checked;
		get_name(p_index,p_text);
		return true;
	}

	bool is_checked(t_uint32 p_index)
	{
		if (p_index == 0)
			return cfg_outer_shown;
		return false;
	}
};

static mainmenu_commands_factory_t< menu_command_alsong > foo_menu;
