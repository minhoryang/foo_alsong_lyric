#pragma once

struct UISize
{
	int width;
	int height;

	UISize() : width(0), height(0) {};
	UISize(int w, int h) : width(w), height(h) {}
};

struct UIPoint
{
	int x;
	int y;

	UIPoint() : x(0), y(0) {};
	UIPoint(int _x, int _y) : x(_x), y(_y) {}
};

DECLARE_INSTANCE_TYPE(UISize)

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
	UIPoint m_TextPos;
	RECT m_DrawRect;
public:
	UICanvas(HDC hdc);
	~UICanvas();

	void DrawText(const UIFont &font, const SQChar *text);
	UISize EstimateText(const UIFont &font, const SQChar *text);
	void SetDrawTextOrigin(UIPoint pt);

	static void RegisterCanvas();
};

DECLARE_INSTANCE_TYPE(UICanvas)
