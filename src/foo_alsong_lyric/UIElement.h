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

class UIElement : public ui_element_instance, public ui_element
{
public:
	UIElement();
	~UIElement();

	ui_element_instance_ptr instantiate(HWND p_parent,ui_element_config::ptr cfg,ui_element_instance_callback_ptr p_callback);
	ui_element_children_enumerator_ptr enumerate_children(ui_element_config::ptr cfg) {return NULL;}
	void initialize_window(HWND parent);
	HWND get_wnd();
	void set_configuration(ui_element_config::ptr config);
	GUID get_guid();
	GUID get_subclass();
	void get_name(pfc::string_base & out);
	ui_element_config::ptr get_default_configuration();
	ui_element_config::ptr get_configuration();
	const char *get_description();
	void notify(const GUID & p_what, t_size p_param1, const void * p_param2, t_size p_param2size);
private:
	ui_element_config::ptr m_config;
	HWND m_hWnd;
	UIManager *m_UI;
	UIPreference m_Setting;
	pfc::string8 m_Script;

	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
protected:
	ui_element_instance_callback_ptr m_callback;
};

static service_factory_single_t<UIElement> g_ui_element_alsong_factory;