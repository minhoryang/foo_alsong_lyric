#pragma once

#include "AlsongUI.h"

class AlsongElement : public ui_element_instance, public ui_element
{
public:
	AlsongElement();
	~AlsongElement();

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
	AlsongUI *m_UI;
	Window_Setting m_Setting;
	pfc::string8 m_Script;

	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
protected:
	ui_element_instance_callback_ptr m_callback;
};

static service_factory_single_t<AlsongElement> g_ui_element_alsong_factory;