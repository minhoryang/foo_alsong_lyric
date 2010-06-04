#include "stdafx.h"

#include "UIElement.h"
#include "UIManager.h"
#include "ConfigStore.h"

UIElement::UIElement() : m_UI(NULL)
{

}

UIElement::~UIElement()
{
	if(m_UI)
		delete m_UI;
}

ui_element_instance_ptr UIElement::instantiate(HWND p_parent,ui_element_config::ptr cfg,ui_element_instance_callback_ptr p_callback)
{
	set_configuration(cfg);
	m_callback = p_callback;

	initialize_window(p_parent);

	return this;
}

void UIElement::initialize_window(HWND parent)
{
	m_UI = new UIManager(&m_Setting, &m_Script);

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
	wcex.lpszClassName = TEXT("UILyricElement");
	wcex.cbWndExtra = sizeof(UIElement *);
	RegisterClassEx(&wcex);

	m_hWnd = CreateWindowEx(NULL, TEXT("UILyricElement"), TEXT("UILyricElement"), WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, parent, NULL, NULL, this);
}

LRESULT CALLBACK UIElement::WindowProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	if(iMessage == WM_NCCREATE)
		SetWindowLongPtr(hWnd, GWL_USERDATA, (LONG)((CREATESTRUCT *)lParam)->lpCreateParams);
	UIElement *_this = (UIElement *)GetWindowLongPtr(hWnd, GWL_USERDATA);
	if(iMessage == WM_NCDESTROY)
	{
		if(_this)
			_this->m_UI->ProcessMessage(hWnd, iMessage, wParam, lParam);
		delete _this->m_UI;
		_this->m_UI = NULL;
	}
	if(_this)
		return _this->m_UI->ProcessMessage(hWnd, iMessage, wParam, lParam);
	else
		return DefWindowProc(hWnd, iMessage, wParam, lParam);
}

HWND UIElement::get_wnd()
{
	return m_hWnd;
}

GUID UIElement::get_guid()
{
	static const GUID UIElementGUID = 
		{ 0x59c2103b, 0x1d53, 0x48ed, { 0xa8, 0x11, 0xff, 0x32, 0x2b, 0x63, 0x1d, 0x46 } };
	return UIElementGUID;
}

GUID UIElement::get_subclass()
{
	return ui_element_subclass_playback_information;
}

void UIElement::get_name(pfc::string_base & out)
{
	out.set_string("Alsong Lyric Element");
}

void UIElement::set_configuration(ui_element_config::ptr config)
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

ui_element_config::ptr UIElement::get_default_configuration()
{
	BYTE temp[sizeof(UIPreference) + 8];
	UIPreference *settemp = (UIPreference *)&temp[4];
	memset(temp, 0, sizeof(temp));
	*((DWORD *)&temp[0]) = ('A' << 24 | 'L' << 16 | 'S' << 8 | 'O');
	settemp->SetDefault();

	return ui_element_config::g_create(get_guid(), temp, sizeof(temp));
}

ui_element_config::ptr UIElement::get_configuration()
{
	if(m_config->get_data_size() == sizeof(m_Setting) + m_Script.get_length() + 8)
	{
		memcpy(((BYTE *)m_config->get_data() + 4), &m_Setting, sizeof(m_Setting));
	}
	return m_config;
}

const char *UIElement::get_description()
{
	return pfc::stringcvt::string_utf8_from_wide(L"알송 실시간 가사");
}

void UIElement::notify(const GUID & p_what, t_size p_param1, const void * p_param2, t_size p_param2size)
{
	if (p_what == ui_element_notify_colors_changed || p_what == ui_element_notify_font_changed) 
		InvalidateRect(m_hWnd, NULL, TRUE);
}