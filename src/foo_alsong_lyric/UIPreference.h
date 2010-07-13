#pragma once

class UIPreference
{
public:
	UIPreference() {}
	~UIPreference() {}

	enum AlignPosition
	{
		ALIGN_LEFT = 1,
		ALIGN_TOP = 1,
		ALIGN_MODDLE = 2,
		ALIGN_BOTTOM = 3,
		ALIGN_RIGHT = 3,
	};
	
	enum BgType
	{
		BG_SOLIDCOLOR = 0,
		BG_IMAGE = 1,
		BG_TRANSPARENT = 2,
	};

	HFONT GetFont()
	{
		return font.create();
	}
	COLORREF GetBkColor()
	{
		return bkColor;
	}
	COLORREF GetFgColor()
	{
		return fgColor;
	}
	WCHAR *GetBgImagePath()
	{
		return bgImage;
	}
	int GetBgType()
	{
		return (int)bgType;
	}
	unsigned int GetnLine()
	{
		return nLine;
	}
	unsigned int GetLineMargin()
	{
		return LineMargin;
	}
	int GetVerticalAlign()
	{
		return VerticalAlign;
	}
	int GetHorizentalAlign()
	{
		return HorizentalAlign;
	}
	int OpenFontPopup(HWND hWndFrom)
	{
		return font.popup_dialog(hWndFrom);
	}
	int OpenFgColorPopup(HWND hWndFrom)
	{
		COLORREF color;
		if((color = OpenColorPopup(hWndFrom, fgColor)) != -1)
		{
			fgColor = color;
			return color;
		}
		return -1;
	}
	int OpenBkColorPopup(HWND hWndFrom)
	{
		COLORREF color;
		if((color = OpenColorPopup(hWndFrom, bkColor)) != -1)
		{
			bgType = BG_SOLIDCOLOR;
			bkColor = color;
			return color;
		}
		return -1;
	}
	int OpenBgImagePopup(HWND hWndFrom)
	{
		OPENFILENAME ofn;
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.lpstrFile = bgImage;
		ofn.lpstrFilter = TEXT("그림 파일(*.bmp;*.png;*.jpg;*.gif;*.jpeg)\0*.bmp;*.png;*.jpg;*.gif;*.jpeg\0\0");
		ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
		ofn.nMaxFile = 255;

		if(GetOpenFileName(&ofn))
		{
			bgType = BG_IMAGE;
			return true;
		}
		return false;
	}
	void SetDefault()
	{
		font = t_font_description::g_from_font((HFONT)GetStockObject(DEFAULT_GUI_FONT));
		bkColor = RGB(255, 255, 255);
		fgColor = RGB(0, 0, 0);
		bgImage[0] = 0;
		bgType = BG_SOLIDCOLOR;
		nLine = 3;
		LineMargin = 100;
		VerticalAlign = ALIGN_MODDLE;
		HorizentalAlign = ALIGN_MODDLE;
	}

	void OpenConfigPopup(HWND hParent);
	static BOOL CALLBACK ConfigProcDispatcher(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
private:
	COLORREF OpenColorPopup(HWND hWndFrom, COLORREF color)
	{
		static COLORREF acrCustClr[16] = {RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), 
			RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), 
			RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), 
			RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255)};

		CHOOSECOLOR choosecolor;
		ZeroMemory(&choosecolor, sizeof(choosecolor));
		choosecolor.lStructSize = sizeof(CHOOSECOLOR);
		choosecolor.Flags = CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT ;
		choosecolor.hwndOwner = hWndFrom;
		choosecolor.lpCustColors = (LPDWORD)acrCustClr;
		choosecolor.rgbResult = color;
		if(ChooseColor(&choosecolor))
			return choosecolor.rgbResult;
		return -1;
	}
	BOOL UIConfigProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam, HWND hParent);

	t_font_description font;
	COLORREF bkColor;
	COLORREF fgColor;
	WCHAR bgImage[MAX_PATH];
	BgType bgType; //0: 색, 1: 이미지, 2:투명한 배경

	DWORD nLine;
	DWORD LineMargin;//%단위

	BYTE VerticalAlign; //상하정렬. 1:위 2:가운데 3:아래
	BYTE HorizentalAlign; //좌우정렬. 1:왼쪽 2:가운데 3:오른쪽

	BYTE bReserved[1022]; //구조체 크기가 변하면 설정이 초기화된다. 나중에 변수 추가할때 여기서 뺄것
};

DECLARE_INSTANCE_TYPE(UIPreference)