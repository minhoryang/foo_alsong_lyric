#pragma once

class UIFont
{
private:
	COLORREF m_Color;
	HFONT m_Font;
	bool m_Generated;
	void Create(const TCHAR *fontfamily, int point, bool bold);
public:
	UIFont(HFONT font);
	UIFont(const TCHAR *fontfamily, int point);
	UIFont(const TCHAR *fontfamily, int point, COLORREF color);
	UIFont(const TCHAR *fontfamily, int point, COLORREF color, bool bold);
	~UIFont();

	HFONT GethFont() const;
	DWORD GetHeight(HDC hdc) const;
	COLORREF GetColor() const;
};

DECLARE_INSTANCE_TYPE(UIFont)

class UICanvas
{
private:
	HDC m_hDC;
	RECT m_LastPrint;
public:
	UICanvas(HDC hdc);
	~UICanvas();

	void DrawText(const UIFont &font, const SQChar *text);

	static void RegisterCanvas();
};

DECLARE_INSTANCE_TYPE(UICanvas)
