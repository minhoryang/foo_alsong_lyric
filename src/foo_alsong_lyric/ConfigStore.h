#pragma once;

t_font_description get_def_font();

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

struct Window_Setting
{
	t_font_description font;
	COLORREF bkColor;
	COLORREF fgColor;
	WCHAR bgImage[MAX_PATH];
	int bgType; //0: 색, 1: 이미지, 2:투명한 배경

	DWORD nLine;
	DWORD LineMargin;//%단위

	BYTE VerticalAlign; //상하정렬. 1:위 2:가운데 3:아래
	BYTE HorizentalAlign; //좌우정렬. 1:왼쪽 2:가운데 3:오른쪽

	BYTE bReserved[1022]; //구조체 크기가 변하면 설정이 초기화된다. 나중에 변수 추가할때 여기서 뺄것
};

extern cfg_string cfg_outer_script;
extern cfg_struct_t<Window_Setting> cfg_outer;