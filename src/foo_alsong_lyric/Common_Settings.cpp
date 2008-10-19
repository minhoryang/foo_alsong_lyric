#include "stdafx.h"
#include "Common_Settings.h"

const GUID guid_cfg_save_to_lrc = // {6F31D8AA-5B2D-484a-9C73-D729BC98459F}
{ 0x6f31d8aa, 0x5b2d, 0x484a, { 0x9c, 0x73, 0xd7, 0x29, 0xbc, 0x98, 0x45, 0x9f } };
cfg_bool cfg_save_to_lrc(guid_cfg_save_to_lrc, false);

const GUID guid_cfg_lrc_save_path = // {8D462857-FEE7-4626-A03E-B5B2F072C442}
{ 0x8d462857, 0xfee7, 0x4626, { 0xa0, 0x3e, 0xb5, 0xb2, 0xf0, 0x72, 0xc4, 0x42 } };
cfg_string cfg_lrc_save_path(guid_cfg_lrc_save_path, "");

const GUID guid_cfg_load_from_lrc = // {F9E8ECB2-B91D-4c6e-8328-BF1068052682}
{ 0xf9e8ecb2, 0xb91d, 0x4c6e, { 0x83, 0x28, 0xbf, 0x10, 0x68, 0x5, 0x26, 0x82 } };
cfg_bool cfg_load_from_lrc(guid_cfg_load_from_lrc, false);


const GUID guid_cfg_outer_shown = 
{ 0x44eefa62, 0x996e, 0x4480, { 0xaa, 0x72, 0x98, 0xef, 0xb6, 0xf0, 0xcd, 0xd2 } };
cfg_bool cfg_outer_shown(guid_cfg_outer_shown, false);

const GUID guid_cfg_outer_topmost = // {337F0BB2-5A5B-4bd5-A845-6C146F887838}
{ 0x337f0bb2, 0x5a5b, 0x4bd5, { 0xa8, 0x45, 0x6c, 0x14, 0x6f, 0x88, 0x78, 0x38 } };
cfg_bool cfg_outer_topmost(guid_cfg_outer_topmost, false);

const GUID guid_cfg_outer_window_placement = 
{ 0xcd3ac2f2, 0x1951, 0x4821, { 0x9f, 0xfa, 0x7, 0x23, 0x3b, 0x57, 0x1d, 0x17 } };
cfg_window_placement cfg_outer_window_placement(guid_cfg_outer_window_placement);

const GUID guid_cfg_outer_transparency = // {05F4DB24-8352-4f22-9901-78AC304FAD8A}
{ 0x5f4db24, 0x8352, 0x4f22, { 0x99, 0x1, 0x78, 0xac, 0x30, 0x4f, 0xad, 0x8a } };
cfg_int cfg_outer_transparency(guid_cfg_outer_transparency, 100);

const GUID guid_cfg_outer_layered = // {F2CC0F3A-735F-46b9-8FA2-6B8313A69B2C}
{ 0xf2cc0f3a, 0x735f, 0x46b9, { 0x8f, 0xa2, 0x6b, 0x83, 0x13, 0xa6, 0x9b, 0x2c } };
cfg_bool cfg_outer_layered(guid_cfg_outer_layered, false);

const GUID guid_cfg_outer_border = // {425D9FC1-06DD-4ba8-9366-D47D142A066E}
{ 0x425d9fc1, 0x6dd, 0x4ba8, { 0x93, 0x66, 0xd4, 0x7d, 0x14, 0x2a, 0x6, 0x6e } };
cfg_bool cfg_outer_border(guid_cfg_outer_border, false);

const GUID guid_cfg_outer = // {77FC7D79-8B1F-4611-BBB9-CB6A57D2A98A}
{ 0x77fc7d79, 0x8b1f, 0x4611, { 0xbb, 0xb9, 0xcb, 0x6a, 0x57, 0xd2, 0xa9, 0x8a } };
cfg_struct_t<Window_Setting> cfg_outer(guid_cfg_outer, NULL); //NULL로 채운다

const GUID guid_cfg_panel = // {EDB8E8E8-42F7-4d1e-8DE5-CC2CBA3769CC}
{ 0xedb8e8e8, 0x42f7, 0x4d1e, { 0x8d, 0xe5, 0xcc, 0x2c, 0xba, 0x37, 0x69, 0xcc } };
cfg_struct_t<Window_Setting> cfg_panel(guid_cfg_panel, NULL); //NULL로 채운다

inline t_font_description get_def_font()
{
	return t_font_description::g_from_font((HFONT)GetStockObject(DEFAULT_GUI_FONT));
}
