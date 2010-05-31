#pragma once

#include "UIPreference.h"

extern cfg_bool cfg_save_to_lrc;
extern cfg_bool cfg_load_from_lrc;
extern cfg_string cfg_lrc_save_path;

extern cfg_window_placement cfg_outer_window_placement;
extern cfg_bool cfg_outer_shown;
extern cfg_bool cfg_outer_topmost;
extern cfg_int cfg_outer_transparency;
extern cfg_bool cfg_outer_layered;
extern cfg_bool cfg_outer_border;
extern cfg_bool cfg_mimic_lyricshow;

extern cfg_string cfg_outer_script;
extern cfg_struct_t<UIPreference> cfg_outer;

extern cfg_bool cfg_outer_nolayered;
extern cfg_bool cfg_lyric_savetofile;
