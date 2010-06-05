#include "stdafx.h"
#include "UICanvas.h"

UICanvas::UICanvas(HDC hdc) : m_hDC(hdc), m_TextPos(0, 0)
{
	GetClientRect(WindowFromDC(hdc), &m_DrawRect);
}

UICanvas::~UICanvas()
{

}

void UICanvas::RegisterCanvas()
{
	SqPlus::SQClassDefNoConstructor<UICanvas>(TEXT("UICanvas")).
		func(&UICanvas::DrawText, TEXT("DrawText")).
		func(&UICanvas::EstimateText, TEXT("EstimateText")).
		func(&UICanvas::SetDrawTextOrigin, TEXT("SetDrawTextOrigin")).
		func(&UICanvas::GetCanvasSize, TEXT("GetCanvasSize"));

	SqPlus::SQClassDefNoConstructor<UIFont>(TEXT("UIFont")).
		overloadConstructor<UIFont(*)(const TCHAR *, int)>().
		overloadConstructor<UIFont(*)(const TCHAR *, int, COLORREF)>().
		overloadConstructor<UIFont(*)(const TCHAR *, int, COLORREF, bool)>();

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

void UICanvas::DrawText(const UIFont &font, const SQChar *text)
{
	if(!m_hDC)
		return;

	COLORREF oldColor = SetTextColor(m_hDC, font.GetColor() & 0x00FFFFFF);
	int oldMode = SetBkMode(m_hDC, TRANSPARENT);
	HFONT oldFont = (HFONT)SelectObject(m_hDC, font.GethFont());

	RECT DrawRect;
	SetRect(&DrawRect, m_TextPos.x, m_TextPos.y, m_DrawRect.right, m_DrawRect.bottom);
	int height = ::DrawText(m_hDC, text, -1, &DrawRect, DT_NOCLIP | DT_WORDBREAK);
	m_TextPos.y += height;

	SetBkMode(m_hDC, oldMode);
	SetTextColor(m_hDC, oldColor);
	SelectObject(m_hDC, oldFont);
}

void UICanvas::SetDrawTextOrigin(const UIPoint &pt)
{
	m_TextPos.x = pt.x;
	m_TextPos.y = pt.y;
}

UISize UICanvas::GetCanvasSize()
{
	RECT rt;
	GetClientRect(WindowFromDC(m_hDC), &rt);
	return UISize(rt.right, rt.bottom);
}

UISize UICanvas::EstimateText(const UIFont &font, const SQChar *text)
{
	if(!m_hDC)
		return UISize(0, 0);

	HFONT oldFont = (HFONT)SelectObject(m_hDC, font.GethFont());

	RECT DrawRect;
	SetRect(&DrawRect, m_TextPos.x, m_TextPos.y, m_DrawRect.right, m_DrawRect.bottom);
	::DrawText(m_hDC, text, -1, &DrawRect, DT_NOCLIP | DT_WORDBREAK | DT_CALCRECT);

	SelectObject(m_hDC, oldFont);
	UISize sz;
	sz.width = DrawRect.right - DrawRect.left;
	sz.height = DrawRect.bottom - DrawRect.top;

	return sz;
}

UIFont::UIFont(const TCHAR *fontfamily, int point, COLORREF color, bool bold) : m_Color(color)
{
	Create(fontfamily, point, bold);
}

UIFont::UIFont(const TCHAR *fontfamily, int point) : m_Color(0xFF000000)
{
	Create(fontfamily, point, false);
}

UIFont::UIFont(const TCHAR *fontfamily, int point, COLORREF color) : m_Color(color)
{
	Create(fontfamily, point, false);
}

void UIFont::Create(const TCHAR *fontfamily, int point, bool bold)
{
	LOGFONT lf;
	HDC hdc = GetDC(NULL);
	lf.lfHeight = -MulDiv(point, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	lf.lfWidth = 0;
	lf.lfEscapement = 0;
	lf.lfOrientation = 0;
	lf.lfWeight = (bold ? FW_BOLD : FW_NORMAL);
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
	m_Generated = true;

	ReleaseDC(NULL, hdc);
}

UIFont::UIFont(HFONT font) : m_Font(font), m_Color(0xFF000000)
{
	m_Generated = false;
}

UIFont::~UIFont()
{
	if(m_Generated)
		DeleteObject(m_Font);
}

COLORREF UIFont::GetColor() const
{
	return m_Color;
}

HFONT UIFont::GethFont() const
{
	return m_Font;
}

DWORD UIFont::GetHeight(HDC hdc) const
{
	TEXTMETRIC tm;
	HFONT hOldFont = (HFONT)SelectObject(hdc, m_Font);
	GetTextMetrics(hdc, &tm);
	SelectObject(hdc, hOldFont);

	return tm.tmHeight;
}

