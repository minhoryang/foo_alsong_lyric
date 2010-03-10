#include "stdafx.h"

#include "AlsongElement.h"
#include "AlsongUI.h"
#include "ConfigStore.h"

AlsongElement::AlsongElement()
{

}

AlsongElement::~AlsongElement()
{

}

ui_element_instance_ptr AlsongElement::instantiate(HWND p_parent,ui_element_config::ptr cfg,ui_element_instance_callback_ptr p_callback)
{
	set_configuration(cfg);
	m_callback = p_callback;

	initialize_window(p_parent);

	return this;
}

void AlsongElement::initialize_window(HWND parent)
{
	m_UI = new AlsongUI(m_Setting, m_Script);

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

void AlsongElement::set_configuration(ui_element_config::ptr config)
{
	DWORD *dataptr = (DWORD *)config->get_data();
	if(dataptr && config->get_data_size() > 4 && dataptr[0] == ('A' << 24 | 'L' << 16 | 'S' << 8 | 'O'))
		m_config = config; //right signature
	else
		m_config = get_default_configuration();
	dataptr = (DWORD *)m_config->get_data();
	memcpy(&m_Setting, &dataptr[1], sizeof(m_Setting));
	BYTE *datatmp = (BYTE *)m_config->get_data();
	DWORD scriptlen = *((DWORD *)&datatmp[sizeof(m_Setting) + 4]);
	m_Script.add_string((const char *)datatmp[sizeof(m_Setting) + 8], scriptlen);
}

ui_element_config::ptr AlsongElement::get_default_configuration()
{
	BYTE temp[sizeof(Window_Setting) + 8];
	Window_Setting *settemp = (Window_Setting *)&temp[4];
	memset(temp, 0, sizeof(temp));
	*((DWORD *)&temp[0]) = ('A' << 24 | 'L' << 16 | 'S' << 8 | 'O');
	settemp->SetDefault();

	return ui_element_config::g_create(get_guid(), temp, sizeof(temp));
}

ui_element_config::ptr AlsongElement::get_configuration()
{
	if(m_config->get_data_size() == sizeof(m_Setting) + m_Script.get_length() + 8)
	{
		memcpy(((BYTE *)m_config->get_data() + 4), &m_Setting, sizeof(m_Setting));
	}
	return m_config;
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