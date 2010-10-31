/*
* foo_alsong_lyric														
* Copyright (C) 2007-2010 Inseok Lee <dlunch@gmail.com>
*
* This library is free software; you can redistribute it and/or modify it 
* under the terms of the GNU Lesser General Public License as published 
* by the Free Software Foundation; version 2.1 of the License.
*
* This library is distributed in the hope that it will be useful, 
* but WITHOUT ANY WARRANTY; without even the implied warranty of 
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
* See the GNU Lesser General Public License for more details.
*
* You can receive a copy of the GNU Lesser General Public License from 
* http://www.gnu.org/
*/

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

struct UIFontDescription
{
	TCHAR face[128];
	int size;
	COLORREF color;
	int italic;
	int bold;
	int underline;
	int strikeout;
};

class UIFont
{
	friend class UICanvas;
	friend class UIPreference;
private:
	boost::shared_ptr<Gdiplus::Font> m_Font;
	COLORREF m_Color;
public:
	UIFont();
	UIFont(const UIFontDescription & fontdesc);
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
	int *m_bits;
public:
	UICanvas::UICanvas(HWND hWnd, HDC hdc);
	~UICanvas();

	void DrawText(const UIFont &font, const SQChar *text, int align, float heightratio);
	UISize EstimateText(const UIFont &font, const SQChar *text);
	UISize GetCanvasSize();
	void SetDrawTextOrigin(const UIPoint &pt);
	void Fill(int x, int y, int width, int height, COLORREF color);
	void DrawImage(int x, int y, int width, int height, const SQChar *path);
	void SetTransparent();

	static void RegisterCanvas();
};

DECLARE_INSTANCE_TYPE(UICanvas)
