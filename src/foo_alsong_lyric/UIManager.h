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
};
