#include "stdafx.h"

#include "resource.h"
#include "ConfigStore.h"
#include "UIPanel.h"
#include "UIManager.h"
#include "UIPreference.h"

UIPanel::UIPanel() : m_UI(NULL)
{
	memset(&m_Setting, 0, sizeof(m_Setting));
}

UIPanel::~UIPanel()
{
	if(m_UI)
		delete m_UI;
}

UIPanel::class_data & UIPanel::get_class_data() const 
{
	__implement_get_class_data_ex(_T("{E859B366-AF66-45f6-9BE1-234FD363825F}"), _T("Alsong Live Lyric"), true, 0, WS_CHILD | WS_CLIPCHILDREN, WS_EX_CONTROLPARENT, CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW);
}

const GUID &UIPanel::get_extension_guid() const
{
	return g_extension_guid;
}

void UIPanel::get_name(pfc::string_base & out) const
{
	out.set_string("Alsong Lyric Panel");
}

void UIPanel::get_category(pfc::string_base & out) const
{
	out.set_string("Panels");
}

unsigned UIPanel::get_type() const
{
	return uie::type_panel;
}

LRESULT UIPanel::on_message(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	return m_UI->ProcessMessage(hWnd, iMessage, wParam, lParam);
}

// {E859B366-AF66-45f6-9BE1-234FD363825F}

const GUID UIPanel::g_extension_guid = { 0xe859b366, 0xaf66, 0x45f6, { 0x98, 0xe1, 0x23, 0x4f, 0xd3, 0x63, 0x82, 0x5f } };

uie::window_factory<UIPanel> g_lyric_window_factory;

void UIPanel::get_menu_items (uie::menu_hook_t & p_hook) 
{
	
}

void UIPanel::set_config(stream_reader * p_reader, t_size p_size, abort_callback & p_abort)
{
	UIPreference Setting_temp;
	int t;
	p_reader->read(&t, sizeof(int), p_abort);
	if(t == ('A' << 24 | 'L' << 16 | 'S' << 8 | 'O')) //signature
	{
		if(p_size - 4>= sizeof(UIPreference))
		{
			p_reader->read(&Setting_temp, sizeof(UIPreference), p_abort);
			if(p_size - 4 > sizeof(UIPreference))
				p_reader->read_string(m_Script, p_abort);
			
			memcpy(&m_Setting, &Setting_temp, sizeof(UIPreference));
		}
	}

	m_UI = new UIManager(&m_Setting, &m_Script);
}

void UIPanel::get_config(stream_writer * p_writer, abort_callback & p_abort) const
{
	int t;
	t = ('A' << 24 | 'L' << 16 | 'S' << 8 | 'O');
	p_writer->write(&t, sizeof(int), p_abort);
	p_writer->write(&m_Setting, sizeof(UIPreference), p_abort);
	if(m_Script)
		p_writer->write_string(m_Script.get_ptr(), p_abort);
}

bool UIPanel::have_config_popup() const
{
	return true;
}

bool UIPanel::show_config_popup(HWND wnd_parent)
{
	m_Setting.OpenConfigPopup(wnd_parent);
	return true;
}