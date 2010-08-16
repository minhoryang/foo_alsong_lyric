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

#pragma once

#include "UIManager.h"

class UIPanel : public uie::container_ui_extension
{
public:
	UIPanel();
	~UIPanel();

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

	UIManager *m_UI;
	UIPreference m_Setting;
	pfc::string8 m_Script;
};
