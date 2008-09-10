inline static t_font_description get_def_font();

extern cfg_bool cfg_enabled;
extern cfg_bool cfg_topmost;
extern cfg_window_placement cfg_popup_window_placement;
extern cfg_bool cfg_save_to_lrc;
extern cfg_bool cfg_load_from_lrc;
extern cfg_string cfg_lrc_save_path;
extern cfg_int cfg_outer_transparency;

struct Alsong_Setting
{
	t_font_description font;
	COLORREF bkColor;
	COLORREF fgColor;
	WCHAR bgImage[MAX_PATH];
	BOOL bgType; //false: 색, true: 이미지

	DWORD nLine;

	DWORD LineMargin;//%단위

	BYTE bReserved[1024]; //구조체 크기가 변하면 설정이 초기화된다. 나중에 변수 추가할때 여기서 뺄것
};

extern cfg_struct_t<Alsong_Setting> cfg_outer;
extern cfg_struct_t<Alsong_Setting> cfg_panel;