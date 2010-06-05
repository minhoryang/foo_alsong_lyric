#include "stdafx.h"
#include "UICanvas.h"

UICanvas::UICanvas(HDC hdc) : m_hDC(hdc)
{
	SetRect(&m_LastPrint, -1, -1, -1, -1);
}

UICanvas::~UICanvas()
{

}

void UICanvas::RegisterCanvas()
{
	SqPlus::SQClassDefNoConstructor<UICanvas>(TEXT("UICanvas")).
		func(&UICanvas::DrawText, TEXT("DrawText"));

	SqPlus::SQClassDefNoConstructor<UIFont>(TEXT("UIFont")).
		overloadConstructor<UIFont(*)(const TCHAR *, int)>().
		overloadConstructor<UIFont(*)(const TCHAR *, int, COLORREF)>().
		overloadConstructor<UIFont(*)(const TCHAR *, int, COLORREF, bool)>();
}

void UICanvas::DrawText(const UIFont &font, const SQChar *text)
{
	if(!m_hDC)
		return;
	if(m_LastPrint.bottom == -1)
		GetClientRect(WindowFromDC(m_hDC), &m_LastPrint);

	COLORREF oldColor = SetTextColor(m_hDC, font.GetColor() & 0x00FFFFFF);
	int oldMode = SetBkMode(m_hDC, TRANSPARENT);
	HFONT oldFont = (HFONT)SelectObject(m_hDC, font.GethFont());

	TextOut(m_hDC, m_LastPrint.left, m_LastPrint.top, text, lstrlen(text));
	m_LastPrint.top += font.GetHeight(m_hDC);

	SetBkMode(m_hDC, oldMode);
	SetTextColor(m_hDC, oldColor);
	SelectObject(m_hDC, oldFont);
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

