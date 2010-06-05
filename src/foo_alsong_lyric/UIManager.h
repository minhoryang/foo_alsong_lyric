#pragma once

#include "ConfigStore.h"

class UIManager
{
public:
	UIManager() {}
	UIManager(UIPreference *Setting, pfc::string8 *Script);
	~UIManager();

	LRESULT ProcessMessage(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
	void ShowConfig(HWND hWndParent);

private:
	bool on_keydown(WPARAM wParam);
	void on_contextmenu(HWND hWndparent);
	void Draw(HWND hWnd, HDC hdc);

	static void ScriptDebugLog(HSQUIRRELVM v,const SQChar* s,...);

	SquirrelVMSys m_vmSys;
	SquirrelObject m_RootTable;
	UIPreference *m_Setting;
	pfc::string8 *m_Script;

	Bitmap *m_BackImage;
	Bitmap *m_BackImageCache;
};

class UIFont
{
private:
	HFONT m_Font;
	bool m_Generated;
	void Create(const TCHAR *fontfamily, int point, bool bold);
public:
	UIFont(HFONT font);
	UIFont(const TCHAR *fontfamily, int point, bool bold);
	UIFont(const TCHAR *fontfamily, int point);
	~UIFont();

	HFONT GethFont() const;
	DWORD GetHeight(HDC hdc) const;

	static int ConstructWithBold(const TCHAR *fontfamily, int point, bool bold, HSQUIRRELVM v);
	static int Construct(const TCHAR *fontfamily, int point, HSQUIRRELVM v);
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
};

DECLARE_INSTANCE_TYPE(UICanvas)
