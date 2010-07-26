#include "stdafx.h"
#include "UICanvas.h"

UICanvas::UICanvas(HWND hWnd, HDC hdc) : m_hWnd(hWnd), m_destDC(hdc), m_transparent(false)
{
	GetClientRect(hWnd, &m_DrawRect);

	//double buffer
	m_hDC = CreateCompatibleDC(hdc);//TODO: move to canvas.
	HBITMAP hBitmap = CreateCompatibleBitmap(hdc, m_DrawRect.right, m_DrawRect.bottom);
	m_hOldBitmap = (HBITMAP)SelectObject(m_hDC, hBitmap);

	FillRect(m_hDC, &m_DrawRect, (HBRUSH)(COLOR_WINDOW + 1));
}

UICanvas::~UICanvas()
{
	RECT rt;
	GetClientRect(m_hWnd, &rt);

	if(m_transparent && GetParent(m_hWnd) == NULL && (GetWindowLong(m_hWnd, GWL_EXSTYLE) & WS_EX_LAYERED))
	{
		BLENDFUNCTION blend = {0};
		blend.BlendOp = AC_SRC_OVER;
		blend.SourceConstantAlpha = 255;
		blend.AlphaFormat = AC_SRC_ALPHA;
		POINT ptPos = {0, 0};
		SIZE sizeWnd = {rt.right, rt.bottom};
		POINT ptSrc = {0, 0};
		UpdateLayeredWindow(m_hWnd, m_destDC, &ptPos, &sizeWnd, m_hDC, &ptSrc, RGB(255, 255, 255), &blend, ULW_COLORKEY);
	}
	else
		BitBlt(m_destDC, 0, 0, rt.right, rt.bottom, m_hDC, 0, 0, SRCCOPY);

	DeleteObject(SelectObject(m_hDC, m_hOldBitmap));
	DeleteDC(m_hDC);
}

void UICanvas::RegisterCanvas()
{
	SqPlus::SQClassDefNoConstructor<UICanvas>(TEXT("UICanvas")).
		func(&UICanvas::DrawText, TEXT("DrawText")).
		func(&UICanvas::EstimateText, TEXT("EstimateText")).
		func(&UICanvas::SetDrawTextOrigin, TEXT("SetDrawTextOrigin")).
		func(&UICanvas::GetCanvasSize, TEXT("GetCanvasSize")).
		func(&UICanvas::Fill, TEXT("Fill")).
		func(&UICanvas::DrawImage, TEXT("DrawImage")).
		func(&UICanvas::SetTransparent, TEXT("SetTransparent"));

	SqPlus::SQClassDefNoConstructor<UIFont>(TEXT("UIFont")).
		overloadConstructor<UIFont(*)(const TCHAR *, int)>().
		overloadConstructor<UIFont(*)(const TCHAR *, int, COLORREF)>().
		overloadConstructor<UIFont(*)(const UIFont &)>().
		overloadConstructor<UIFont(*)()>();

	SqPlus::SQClassDefNoConstructor<UISize>(TEXT("UISize")).
		overloadConstructor<UISize(*)()>().
		overloadConstructor<UISize(*)(int, int)>().
		var(&UISize::width, TEXT("width")).
		var(&UISize::height, TEXT("height"));

	SqPlus::SQClassDefNoConstructor<UIPoint>(TEXT("UIPoint")).
		overloadConstructor<UIPoint(*)()>().
		overloadConstructor<UIPoint(*)(int, int)>().
		var(&UIPoint::x, TEXT("x")).
		var(&UIPoint::y, TEXT("y"));
}

void UICanvas::DrawImage(int x, int y, int width, int height, const SQChar *path)
{
	Gdiplus::Bitmap bitmap(path);
	Gdiplus::Graphics g(m_hDC);
	g.DrawImage(&bitmap, x, y, width, height);
}

void UICanvas::Fill(int x, int y, int width, int height, COLORREF color)
{
	if(!m_hDC)
		return;

	RECT rt;
	SetRect(&rt, x, y, width + x, height + y);
	HBRUSH brush = CreateSolidBrush(color);
	FillRect(m_hDC, &rt, brush);
	DeleteObject(brush);
}

void UICanvas::SetTransparent()
{
	HWND wnd_parent = GetParent(m_hWnd);
	if(wnd_parent)
	{
		POINT pt = {0, 0}, pt_old = {0,0};
		MapWindowPoints(m_hWnd, wnd_parent, &pt, 1);
		OffsetWindowOrgEx(m_hDC, pt.x, pt.y, &pt_old);
		BOOL b_ret = SendMessage(wnd_parent, WM_ERASEBKGND,(WPARAM)m_hDC, 0);
		SetWindowOrgEx(m_hDC, pt_old.x, pt_old.y, 0); //notify parent to redraw background
	}
	m_transparent = true;
}

void UICanvas::DrawText(const UIFont &font, const SQChar *text, int align)
{
	if(!m_hDC)
		return;

	COLORREF oldColor = SetTextColor(m_hDC, font.GetColor() & 0x00FFFFFF);
	int oldMode = SetBkMode(m_hDC, TRANSPARENT);

	RECT DrawRect;
	SetRect(&DrawRect, m_TextPos.x, m_TextPos.y, m_DrawRect.right, m_DrawRect.bottom);
	int height;
	int alignopt = (align == 1 ? DT_LEFT : align == 2 ? DT_CENTER : DT_RIGHT);
	if(text[0] == 1)
	{//bold
		HFONT oldFont = (HFONT)SelectObject(m_hDC, font.GetBoldFont());
		height = ::DrawText(m_hDC, text + 1, -1, &DrawRect, DT_NOCLIP | DT_WORDBREAK | DT_NOPREFIX | alignopt);
		SelectObject(m_hDC, oldFont);
	}
	else
	{
		HFONT oldFont = (HFONT)SelectObject(m_hDC, font.GethFont());
		height = ::DrawText(m_hDC, text, -1, &DrawRect, DT_NOCLIP | DT_WORDBREAK | DT_NOPREFIX | alignopt);
		SelectObject(m_hDC, oldFont);
	}
	m_TextPos.y += height;

	SetBkMode(m_hDC, oldMode);
	SetTextColor(m_hDC, oldColor);
}

void UICanvas::SetDrawTextOrigin(const UIPoint &pt)
{
	m_TextPos.x = pt.x;
	m_TextPos.y = pt.y;
}

UISize UICanvas::GetCanvasSize()
{
	RECT rt;
	GetClientRect(m_hWnd, &rt);
	return UISize(rt.right, rt.bottom);
}

UISize UICanvas::EstimateText(const UIFont &font, const SQChar *text)
{
	if(!m_hDC)
		return UISize(0, 0);

	RECT DrawRect;
	SetRect(&DrawRect, m_TextPos.x, m_TextPos.y, m_DrawRect.right, m_DrawRect.bottom);

	if(text[0] == 1)
	{//bold
		HFONT oldFont = (HFONT)SelectObject(m_hDC, font.GetBoldFont());
		::DrawText(m_hDC, text + 1, -1, &DrawRect, DT_NOCLIP | DT_WORDBREAK | DT_CALCRECT | DT_NOPREFIX);
		SelectObject(m_hDC, oldFont);
	}
	else
	{
		HFONT oldFont = (HFONT)SelectObject(m_hDC, font.GethFont());
		::DrawText(m_hDC, text, -1, &DrawRect, DT_NOCLIP | DT_WORDBREAK | DT_CALCRECT | DT_NOPREFIX);
		SelectObject(m_hDC, oldFont);
	}

	UISize sz;
	sz.width = DrawRect.right - DrawRect.left;
	sz.height = DrawRect.bottom - DrawRect.top;

	return sz;
}

UIFont::UIFont() : m_Color(0x00000000), m_BoldFont(NULL), m_Font(NULL), m_Generated(FALSE)
{
}

UIFont::UIFont(const TCHAR *fontfamily, int point) : m_Color(0xFF000000)
{
	Create(fontfamily, point);
}

UIFont::UIFont(const TCHAR *fontfamily, int point, COLORREF color) : m_Color(color)
{
	Create(fontfamily, point);
}

void UIFont::Create(const TCHAR *fontfamily, int point)
{
	LOGFONT lf;
	HDC hdc = GetDC(NULL);
	lf.lfHeight = -MulDiv(point, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	lf.lfWidth = 0;
	lf.lfEscapement = 0;
	lf.lfOrientation = 0;
	lf.lfWeight = FW_NORMAL;
	lf.lfItalic = FALSE;
	lf.lfUnderline = FALSE;
	lf.lfStrikeOut = FALSE;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
	lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lf.lfQuality = CLEARTYPE_QUALITY;
	lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
	lstrcpy(lf.lfFaceName, fontfamily);
	m_Font = CreateFontIndirect(&lf);
	lf.lfWeight = FW_BOLD;
	m_BoldFont = CreateFontIndirect(&lf);
	m_Generated = true;

	ReleaseDC(NULL, hdc);
}

UIFont::UIFont(HFONT font, COLORREF color) : m_Font(font), m_Color(color), m_Generated(true)
{
	LOGFONT lf;
	int ret = GetObject(font, sizeof(lf), &lf);
	lf.lfWeight = FW_BOLD;
	m_BoldFont = CreateFontIndirect(&lf);
}

UIFont::UIFont(const UIFont &font)
{
	m_Generated = font.m_Generated;
	m_Font = font.m_Font;
	m_Color = font.m_Color;
	m_BoldFont = font.m_BoldFont;
	const_cast<UIFont &>(font).m_Generated = false;
}

UIFont::~UIFont()
{
	if(m_Generated)
	{
		DeleteObject(m_Font);
		DeleteObject(m_BoldFont);
	}
}

UIFont &UIFont::operator =(const UIFont &font)
{
	m_Generated = font.m_Generated;
	m_Font = font.m_Font;
	m_Color = font.m_Color;
	m_BoldFont = font.m_BoldFont;
	const_cast<UIFont &>(font).m_Generated = false;

	return *this;
}

COLORREF UIFont::GetColor() const
{
	return m_Color;
}

HFONT UIFont::GethFont() const
{
	return m_Font;
}

HFONT UIFont::GetBoldFont() const
{
	return m_BoldFont;
}

DWORD UIFont::GetHeight(HDC hdc) const
{
	TEXTMETRIC tm;
	HFONT hOldFont = (HFONT)SelectObject(hdc, m_Font);
	GetTextMetrics(hdc, &tm);
	SelectObject(hdc, hOldFont);

	return tm.tmHeight;
}

