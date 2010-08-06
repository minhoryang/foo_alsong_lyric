#pragma once

#include "UIPreference.h"

extern cfg_window_placement cfg_outer_window_placement;
extern cfg_bool cfg_outer_shown;
extern cfg_bool cfg_outer_topmost;
extern cfg_int cfg_outer_transparency;
extern cfg_bool cfg_outer_layered;
extern cfg_bool cfg_outer_border;

extern cfg_string cfg_outer_script;
extern cfg_struct_t<UIPreference> cfg_outer;

extern cfg_bool cfg_outer_nolayered;

class cfg_lyricsource_var : public cfg_var
{
private:
	std::vector<GUID> m_reallist;
public:
	cfg_lyricsource_var(const GUID &guid) : cfg_var(guid) {}

	void get_data_raw(stream_writer * p_stream,abort_callback & p_abort)
	{
		for(std::vector<GUID>::iterator it = m_reallist.begin(); it != m_reallist.end(); it ++)
			p_stream->write(&*it, sizeof(GUID), p_abort);
	}

	void set_data_raw(stream_reader * p_stream,t_size p_sizehint,abort_callback & p_abort)
	{
		int cnt = p_sizehint / sizeof(GUID);

		for(int i = 0; i < cnt; i ++)
		{
			GUID tmp;
			p_stream->read(&tmp, sizeof(GUID), p_abort);
			m_reallist.push_back(tmp);
		}
	}
};

extern cfg_lyricsource_var cfg_enabledlyricsource;