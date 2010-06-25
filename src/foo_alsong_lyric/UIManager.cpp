#include "stdafx.h"
#include "ConfigStore.h"
#include "LyricManager.h"
#include "UIManager.h"
#include "UICanvas.h"

//TODO: DropSource

UIManager::UIManager(UIPreference *Setting, pfc::string8 *Script) : m_Setting(Setting), m_Script(Script)
{
	SquirrelVM::Init();
	SquirrelVM::GetVMSys(m_vmSys);

	sq_setprintfunc(m_vmSys, &UIManager::ScriptDebugLog, &UIManager::ScriptDebugLog);

	UICanvas::RegisterCanvas();

	SquirrelObject InitScript = SquirrelVM::CompileBuffer(TEXT("function Init() { }"));
	SquirrelVM::RunScript(InitScript);

	SquirrelObject DrawScript = SquirrelVM::CompileBuffer(TEXT("function Draw(canvas, lines) { local font = UIFont(\"맑은 고딕\", 10, WndSetting.GetFontColor()); local h = 0; foreach(i,v in lines) {local sz = canvas.EstimateText(font, v); h += sz.height} local sz = canvas.GetCanvasSize(); canvas.SetDrawTextOrigin(UIPoint(0, (sz.height - h) / 2)); foreach(i, v in lines){ canvas.DrawText(font, v);}}"));
	SquirrelVM::RunScript(DrawScript);

	SqPlus::SQClassDefNoConstructor<UIPreference>(TEXT("UIPreference")).
		func(&UIPreference::GetBkColor, TEXT("GetBackColor")).
		func(&UIPreference::GetFgColor, TEXT("GetFontColor"));

	SqPlus::BindVariable(m_Setting, TEXT("WndSetting"));

	m_RootTable = SquirrelVM::GetRootTable();
	SqPlus::SquirrelFunction<void>(m_RootTable, TEXT("Init"))();
}

UIManager::~UIManager()
{
	SquirrelVM::SetVMSys(m_vmSys);
	SquirrelVM::Shutdown();
	m_vmSys.Reset();
}

LRESULT UIManager::ProcessMessage(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	switch(iMessage)
	{
	case WM_CREATE:
		if(!LyricManagerInstance)
			LyricManagerInstance = new LyricManager();
		LyricManagerInstance->AddRedrawHandler(boost::bind(InvalidateRect, hWnd, (const RECT *)NULL, TRUE));
		break;
	case WM_CONTEXTMENU:
	case WM_NCRBUTTONUP:
		on_contextmenu(hWnd);
		break;
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		on_keydown(wParam);
		return 0;

	case WM_DESTROY:
		return 0;

	case WM_PAINT:
		PAINTSTRUCT ps;
		if(BeginPaint(hWnd, &ps) != NULL) 
		{
			Draw(hWnd, ps.hdc);
			EndPaint(hWnd, &ps);
		}
		return 0;
	case WM_ERASEBKGND:
		return TRUE;
	}
	return DefWindowProc(hWnd, iMessage, wParam, lParam);
}

void UIManager::Draw(HWND hWnd, HDC hdc)
{
	int before, after;
	unsigned int i, cnt = 0;
	std::vector<LyricLine> lyric = LyricManagerInstance->GetLyric();
	after = before = m_Setting->GetnLine() / 2 - lyric.size() / 2;
	std::vector<LyricLine> lyricbefore = LyricManagerInstance->GetLyricBefore(before);
	std::vector<LyricLine> lyricafter = LyricManagerInstance->GetLyricAfter(after + lyric.size() - 1 - (1 - m_Setting->GetnLine() % 2));
	//현재 가사가 1줄 이상인 경우에는 두번째 줄부터
	if(!lyric.size())
		return;

	RECT rt;
	GetClientRect(hWnd, &rt);
	FillRect(hdc, &rt, (HBRUSH)(COLOR_WINDOW + 1));

	HWND wnd_parent = GetParent(hWnd);
	POINT pt = {0, 0}, pt_old = {0,0};
	MapWindowPoints(hWnd, wnd_parent, &pt, 1);
	OffsetWindowOrgEx(hdc, pt.x, pt.y, &pt_old);
	BOOL b_ret = SendMessage(wnd_parent, WM_ERASEBKGND,(WPARAM)hdc, 0);
	SetWindowOrgEx(hdc, pt_old.x, pt_old.y, 0); //notify parent to redraw background

	SquirrelVM::SetVMSys(m_vmSys);
	SquirrelObject lyrics = SquirrelVM::CreateArray(0);

	for(i = 0; i < before - lyricbefore.size(); i ++)
		lyrics.ArrayAppend(TEXT(" "));
	if(lyric.size() < m_Setting->GetnLine())
	{
		for(i = 0; i < lyricbefore.size(); i ++)
		{
			std::wstring nowlrcw = pfc::stringcvt::string_wide_from_utf8_fast(lyricbefore[i].lyric.c_str());
			lyrics.ArrayAppend(nowlrcw.c_str());
		}
	}
	for(i = 0; i < lyric.size(); i ++)
	{
		std::wstring nowlrcw = std::wstring(1, (wchar_t)1) + pfc::stringcvt::string_wide_from_utf8_fast(lyric[i].lyric.c_str()).get_ptr();
		lyrics.ArrayAppend(nowlrcw.c_str());
	}
	for(i = max(lyric.size() - 1, 0); i < lyricafter.size(); i ++)
	{
		std::wstring nowlrcw = pfc::stringcvt::string_wide_from_utf8_fast(lyricafter[i].lyric.c_str());
		lyrics.ArrayAppend(nowlrcw.c_str());
	}

	UICanvas canvas(hdc);
	SqPlus::SquirrelFunction<void>(m_RootTable, TEXT("Draw"))(&canvas, lyrics);
}

void UIManager::ScriptDebugLog(HSQUIRRELVM v,const SQChar* s,...)
{
	static SQChar temp[2048];
	va_list vl;
	va_start(vl, s);
	scvsprintf(temp, s, vl);
	console::formatter() << "foo_alsong_lyric: Squirrel print:" << pfc::stringcvt::string_utf8_from_wide(temp);
	va_end(vl);
}

void UIManager::ShowConfig(HWND hWndParent)
{

}

bool UIManager::on_keydown(WPARAM wParam) 
{
	bool rv = false;

	try 
	{
		metadb_handle_list items;
		static_api_ptr_t<play_control> pc;
		metadb_handle_ptr handle;
		if (pc->get_now_playing(handle)) 
			items.add_item(handle);

		static_api_ptr_t<keyboard_shortcut_manager> ksm;
		rv = ksm->on_keydown_auto_context(items, wParam, contextmenu_item::caller_undefined);
	}
	catch (const exception_service_not_found &) 
	{

	}

	return rv;
}

void UIManager::on_contextmenu(HWND hWndFrom)
{//TODO:메뉴 다 지우고 설정만
	enum 
	{
		ID_FONT = 1,
		ID_TOPMOST,
		ID_BKCOLOR,
		ID_FGCOLOR,
		ID_BGIMAGE,
		ID_ADVSET,
		ID_CONTEXT_FIRST,
		ID_CONTEXT_LAST = ID_CONTEXT_FIRST + 1000,
	};

	HMENU hMenu = CreatePopupMenu();
	AppendMenu(hMenu, MF_STRING, ID_FONT, TEXT("폰트 선택..."));
	if(GetParent(hWndFrom) == NULL)//if top level
	{
		if(cfg_outer_topmost)
			AppendMenu(hMenu, MF_STRING | MF_CHECKED, ID_TOPMOST, TEXT("항상 위에 보이기"));
		else
			AppendMenu(hMenu, MF_STRING, ID_TOPMOST, TEXT("항상 위에 보이기"));
	}
	AppendMenu(hMenu, MF_STRING, ID_BKCOLOR, TEXT("배경색 선택..."));
	AppendMenu(hMenu, MF_STRING, ID_FGCOLOR, TEXT("글자색 선택..."));
	AppendMenu(hMenu, MF_STRING, ID_BGIMAGE, TEXT("배경그림 선택..."));
	AppendMenu(hMenu, MF_STRING, ID_ADVSET, TEXT("고급 설정..."));

	try 
	{
		metadb_handle_list items;
		static_api_ptr_t<play_control> pc;
		metadb_handle_ptr handle;
		if (pc->get_now_playing(handle))
			items.add_item(handle);
		service_ptr_t<contextmenu_manager> cmm;
		contextmenu_manager::g_create(cmm);
		const bool show_shortcuts = config_object::g_get_data_bool_simple(standard_config_objects::bool_show_keyboard_shortcuts_in_menus, false);
		unsigned flags = show_shortcuts ? contextmenu_manager::FLAG_SHOW_SHORTCUTS : 0;
		cmm->init_context(items, flags);
		if (cmm->get_root()) 
		{
			uAppendMenu(hMenu, MF_SEPARATOR, 0, 0);
			cmm->win32_build_menu(hMenu, ID_CONTEXT_FIRST, ID_CONTEXT_LAST);
		}

		menu_helpers::win32_auto_mnemonics(hMenu);

		POINT pt;
		GetCursorPos(&pt);
		int cmd = TrackPopupMenu(hMenu, TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTBUTTON, 
			pt.x, pt.y, 0, hWndFrom, 0);

		if (cmd == ID_FONT) 
		{
			if(m_Setting->OpenFontPopup(hWndFrom))
				InvalidateRect(hWndFrom, NULL, TRUE);
		} 
		else if(cmd == ID_TOPMOST)
		{
			cfg_outer_topmost = !cfg_outer_topmost;

			SetWindowLong(hWndFrom, GWL_EXSTYLE, (cfg_outer_topmost ? GetWindowLong(hWndFrom, GWL_EXSTYLE) | WS_EX_TOPMOST : GetWindowLong(hWndFrom, GWL_EXSTYLE) & ~WS_EX_TOPMOST));
			SetWindowPos(hWndFrom, (cfg_outer_topmost ? HWND_TOPMOST : HWND_NOTOPMOST), 0, 0, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOMOVE | SWP_FRAMECHANGED);
		}
		else if(cmd == ID_BKCOLOR)
		{			
			if(m_Setting->OpenBkColorPopup(hWndFrom) != -1)
				InvalidateRect(hWndFrom, NULL, TRUE);
		}
		else if(cmd == ID_FGCOLOR)
		{
			if(m_Setting->OpenFgColorPopup(hWndFrom) != -1)
				InvalidateRect(hWndFrom, NULL, TRUE);
		}
		else if(cmd == ID_BGIMAGE)
		{
			if(m_Setting->OpenBgImagePopup(hWndFrom))
				InvalidateRect(hWndFrom, NULL, TRUE);
		}
		else if(cmd == ID_ADVSET)
		{
			m_Setting->OpenConfigPopup(hWndFrom);
		}
		else if (cmd >= ID_CONTEXT_FIRST && cmd <= ID_CONTEXT_LAST ) 
			cmm->execute_by_id(cmd - ID_CONTEXT_FIRST);

	}
	catch (const exception_service_not_found &) {
	}
	DestroyMenu(hMenu);
}
