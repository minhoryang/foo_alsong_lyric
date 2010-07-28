#pragma once

struct UISize
{
	int width;
	int height;

	UISize() : width(0), height(0) {};
	UISize(int w, int h) : width(w), height(h) {}
};

DECLARE_INSTANCE_TYPE(UISize)

struct UIPoint
{
	int x;
	int y;

	UIPoint() : x(0), y(0) {};
	UIPoint(int _x, int _y) : x(_x), y(_y) {}
};

DECLARE_INSTANCE_TYPE(UIPoint)

class UIFont
{
private:
	COLORREF m_Color;
	HFONT m_Font;
	HFONT m_BoldFont;
	bool m_Generated;
	void Create(const TCHAR *fontfamily, int point);
public:
	UIFont();
	UIFont(const UIFont &font);
	UIFont(HFONT font, COLORREF color);
	UIFont(const TCHAR *fontfamily, int point);
	UIFont(const TCHAR *fontfamily, int point, COLORREF color);
	~UIFont();

	UIFont &operator =(const UIFont &font);

	HFONT GethFont() const;
	HFONT GetBoldFont() const;
	DWORD GetHeight(HDC hdc) const;
	COLORREF GetColor() const;
};

DECLARE_INSTANCE_TYPE(UIFont)

class UICanvas
{
private:
	HDC m_hDC;
	HDC m_destDC;
	UIPoint m_TextPos;
	RECT m_DrawRect;
	HBITMAP m_hOldBitmap;
	HWND m_hWnd;
	bool m_transparent;
	int *m_bits;
public:
	UICanvas::UICanvas(HWND hWnd, HDC hdc);
	~UICanvas();

	void DrawText(const UIFont &font, const SQChar *text, int align);
	UISize EstimateText(const UIFont &font, const SQChar *text);
	UISize GetCanvasSize();
	void SetDrawTextOrigin(const UIPoint &pt);
	void Fill(int x, int y, int width, int height, COLORREF color);
	void DrawImage(int x, int y, int width, int height, const SQChar *path);
	void SetTransparent();

	static void RegisterCanvas();
};

DECLARE_INSTANCE_TYPE(UICanvas)
