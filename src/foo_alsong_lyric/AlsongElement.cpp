#include "stdafx.h"

#include "AlsongElement.h"
#include "AlsongUI.h"

AlsongElement::AlsongElement()
{

}

AlsongElement::~AlsongElement()
{

}

ui_element_instance_ptr AlsongElement::instantiate(HWND p_parent,ui_element_config::ptr cfg,ui_element_instance_callback_ptr p_callback)
{
	m_config = cfg;
	m_callback = p_callback;

	initialize_window(p_parent);

	return this;
}

void AlsongElement::initialize_window(HWND parent)
{
	Window_Setting nullset;
	m_UI = new AlsongUI(nullset, pfc::string8());

	WNDCLASSEX wcex;
	memset(&wcex, 0, sizeof(wcex));
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_DBLCLKS;
	wcex.lpfnWndProc = &WindowProc;
	wcex.hInstance = NULL;
	wcex.hIconSm = NULL;
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(0, IDC_ARROW);
	wcex.hbrBackground = NULL;
	wcex.lpszClassName = TEXT("AlsongLyricElement");
	wcex.cbWndExtra = sizeof(AlsongElement *);
	RegisterClassEx(&wcex);

	m_hWnd = CreateWindowEx(NULL, TEXT("AlsongLyricElement"), TEXT("Alsong Lyric"), WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, parent, NULL, NULL, this);
}

LRESULT CALLBACK AlsongElement::WindowProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	if(iMessage == WM_NCCREATE)
		SetWindowLongPtr(hWnd, GWL_USERDATA, (LONG)((CREATESTRUCT *)lParam)->lpCreateParams);
	AlsongElement *_this = (AlsongElement *)GetWindowLongPtr(hWnd, GWL_USERDATA);
	if(_this)
		return _this->m_UI->ProcessMessage(hWnd, iMessage, wParam, lParam);
	else
		return DefWindowProc(hWnd, iMessage, wParam, lParam);
}

HWND AlsongElement::get_wnd()
{
	return m_hWnd;
}

void AlsongElement::set_configuration(ui_element_config::ptr config)
{
	m_config = config;
}

ui_element_config::ptr AlsongElement::get_configuration()
{
	return m_config;
}

GUID AlsongElement::get_guid()
{
	static const GUID AlsongElementGUID = 
		{ 0x59c2103b, 0x1d53, 0x48ed, { 0xa8, 0x11, 0xff, 0x32, 0x2b, 0x63, 0x1d, 0x46 } };
	return AlsongElementGUID;
}

GUID AlsongElement::get_subclass()
{
	return ui_element_subclass_playback_information;
}

void AlsongElement::get_name(pfc::string_base & out)
{
	out.set_string("Alsong Lyric Element");
}

ui_element_config::ptr AlsongElement::get_default_configuration()
{
	return ui_element_config::g_create_empty(get_guid());
}

const char *AlsongElement::get_description()
{
	return pfc::stringcvt::string_utf8_from_wide(L"알송 실시간 가사");
}

void AlsongElement::notify(const GUID & p_what, t_size p_param1, const void * p_param2, t_size p_param2size)
{
	if (p_what == ui_element_notify_colors_changed || p_what == ui_element_notify_font_changed) 
		InvalidateRect(m_hWnd, NULL, TRUE);
}