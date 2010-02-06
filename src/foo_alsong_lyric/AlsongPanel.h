#pragma once

#include "AlsongUI.h"

class AlsongPanel : public uie::container_ui_extension
{
public:
	AlsongPanel();
	~AlsongPanel();

	virtual const GUID & get_extension_guid() const;
	virtual void get_name(pfc::string_base & out)const;
	virtual void get_category(pfc::string_base & out)const;
	unsigned get_type () const;
	virtual void set_config(stream_reader * p_reader, t_size p_size, abort_callback & p_abort);
	virtual void get_config(stream_writer * p_writer, abort_callback & p_abort) const;
	virtual bool have_config_popup() const;
	virtual bool show_config_popup(HWND wnd_parent);

private:
	LRESULT on_message(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

	virtual class_data & get_class_data() const; 
	virtual void get_menu_items (uie::menu_hook_t & p_hook) ;

	static const GUID g_extension_guid;
	static const GUID g_mimic_extension_guid;

	AlsongUI *m_UI;
	Window_Setting m_Setting;
	pfc::string8 m_Script;
};
