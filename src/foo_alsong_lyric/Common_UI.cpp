#include "stdafx.h"
#include "Panel_Plugin.h"
#include "Common_Settings.h"
#include "Common_Lyric_Manipulation.h"
#include "Common_UI.h"

//TODO: Titleformat 사용해서 출력 formatting.

COLORREF acrCustClr[16] = {RGB(255, 255, 255	), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), 
RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), 
RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), 
RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255), RGB(255, 255, 255)};

using namespace Gdiplus;

Common_UI_Base *Common_UI; 

void DeleteStub(std::pair<HWND, WindowData *> data);
void InvalidateStub(std::pair<HWND, WindowData *> data);

Common_UI_Base::Common_UI_Base()
{
	// Register play callback.
	try 
	{
		static_api_ptr_t<play_callback_manager> pcm;
		pcm->register_callback(this, flag_on_playback_all, false);
	}
	catch (const exception_service_not_found &) 
	{
		// play_callback_manager does not exist; something is very wrong.
	}

	Lyric = new Common_Lyric_Manipulation();

	NowPlaying_Path = NULL;
	hLyricThread = NULL;

	hSleep = CreateEvent(NULL, TRUE, FALSE, NULL);
	hTime = CreateEvent(NULL, TRUE, FALSE, NULL);
}

Common_UI_Base::~Common_UI_Base()
{
	try 
	{
		static_api_ptr_t<play_callback_manager> pcm;
		pcm->unregister_callback(this);
	}
	catch (const exception_service_not_found &) 
	{
		// play_callback_manager does not exist; something is very wrong.
	}

	std::for_each(WndInfo.begin(), WndInfo.end(), DeleteStub);
	
	CloseHandle(hSleep);
	CloseHandle(hTime);

	delete Lyric;
}

void DeleteStub(std::pair<HWND, WindowData *> data)
{
	delete data.second;
}

void InvalidateStub(std::pair<HWND, WindowData *> data)
{
	InvalidateRect(data.first, NULL, TRUE);
}

void Common_UI_Base::InvalidateAllWindow()
{
	std::for_each(WndInfo.begin(), WndInfo.end(), InvalidateStub);
}

class save_lrc_callback : public main_thread_callback
{
	Common_UI_Base *UI;
public:
	save_lrc_callback(Common_UI_Base *UIPointer)
	{
		UI = UIPointer;
	}

	virtual void callback_run()
	{
		WCHAR wctemp[255];
		pfc::string8 str;
		str = UI->LRCSave_Path;

		UI->GetLRCSavePath(wctemp);
		if(wctemp[0] == 0)
			return;

		if(lstrcmpA(str.get_ptr(), UI->NowPlaying_Path) != 0)
			return; //곡바뀌면 무시

		UI->Lyric->SaveToFile(wctemp, "lrc");
	}
};

UINT CALLBACK Common_UI_Base::LyricFetchThread(LPVOID lpParameter)
{
	Common_UI_Base *_this = (Common_UI_Base *)lpParameter;
	pfc::string8 str;
	unsigned char *data = NULL;
	str = _this->NowPlaying_Path;
	_this->LRCSave_Path = _this->NowPlaying_Path;
	service_ptr_t<file> file;
	abort_callback_impl abort_callback;

	try
	{
		archive_impl::g_open(file, _this->NowPlaying_Path, foobar2000_io::filesystem::open_mode_read, abort_callback);
		//TODO:cue일때 특별 처리(subsong_index가 있을 때)
		data = (unsigned char *)malloc(min(0x50000, (size_t)file->get_size(abort_callback)));
		//다 읽지 말고 처음 부분만 읽자
		file->read(data, min(0x50000, (size_t)file->get_size(abort_callback)), abort_callback);
	}
	catch(...)
	{
		if(data)
			free(data);
		return 0;
	}

	if(str.find_last('.') == 0)
	{
		free(data);
		return 0;
	}
	
	_this->Lyric->FetchLyric(data, min(0x50000, (size_t)file->get_size(abort_callback)), (char *)str.get_ptr() + str.find_last('.') + 1);

	if(cfg_save_to_lrc) //TODO:UI만들기
	{
		service_ptr_t<save_lrc_callback> p_callback = new service_impl_t<save_lrc_callback>(_this);
		static_api_ptr_t<main_thread_callback_manager> man;
		man->add_callback(p_callback);
	}
	
	if(lstrcmpA(str.get_ptr(), _this->NowPlaying_Path) != 0)
	{
		_this->Lyric->ClearLyric();
		free(data);
		return 0; //곡이 바뀌면 무시
	}

	free(data);

	_this->hLyricThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)LyricCountThread, (LPVOID)_this, NULL, NULL);

	_this->InvalidateAllWindow();
	return 0;
}

class uSetWindowTextStub
{
private:
	const char *m_arg;
public:

	uSetWindowTextStub(const char *arg)
	{
		m_arg = arg;
	}

	void operator() (std::pair<HWND, WindowData *> data) const
	{
		uSetWindowText(data.first, m_arg);
	}
};
//TODO: 가사등록 성공시 가사 가져오기
void Common_UI_Base::on_playback_new_track(metadb_handle_ptr p_track)
{
	NowPlaying_Path = p_track->get_path();

	NowLine = 0;
	NowTime = 0;
	Lyric->ClearLyric();
	InvalidateAllWindow();
	if(hLyricThread)
	{
		TerminateThread(hLyricThread, 0);//TODO: 쓰지 말란다.
		CloseHandle(hLyricThread);
		hLyricThread = 0;
	}
	
	if(cfg_load_from_lrc) //TODO: UI
	{
		WCHAR wctemp[255];

		GetLRCSavePath(wctemp);

		if(wctemp[0] != 0 && !Lyric->LoadFromFile(wctemp, "lrc")) //로드 실패시 정상 과정으로
			CloseHandle(CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)LyricFetchThread, (LPVOID)this, NULL, NULL));
		else
		{ //성공시 Count Thread 생성. Fetch Thread에서도 전송 성공시 생성된다.
			hLyricThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)LyricCountThread, (LPVOID)this, NULL, NULL);

			InvalidateAllWindow();
		}
	}
	else
		CloseHandle(CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)LyricFetchThread, (LPVOID)this, NULL, NULL));

	//제목 설정
	service_ptr_t<titleformat_object> to;
	pfc::string8 str;

	static_api_ptr_t<titleformat_compiler>()->compile_safe(to, "[%artist% - ]%title%");
	p_track->format_title(NULL, str, to, NULL);

	std::for_each(WndInfo.begin(), WndInfo.end(), uSetWindowTextStub(str.get_ptr()));
}

void Common_UI_Base::GetLRCSavePath(WCHAR *path)
{
	core_api::assert_main_thread();
	metadb_handle_ptr p_track;
	static_api_ptr_t<play_control> pc;
	pc->get_now_playing(p_track);
	if(p_track == NULL)
	{
		path[0] = 0;
		return;
	}

	service_ptr_t<titleformat_object> to;
	pfc::string8 str;

	static_api_ptr_t<titleformat_compiler>()->compile_safe(to, "[%artist% - ]%title%");
	p_track->format_title(NULL, str, to, NULL);
	const char *Nowpath = p_track->get_path();
	
	if(!cfg_lrc_save_path.get_ptr()[0] && (pfc::strcmp_ex(Nowpath, 6, "file://", 6) == 0))
	//if(pfc::strcmp_ex(Nowpath, 6, "file://", 6) == 0) //파일이면
	{
		Nowpath += 7;
		pfc::stringcvt::convert_utf8_to_wide(path, 255, Nowpath, lstrlenA(Nowpath));
		LPTSTR ext = StrRChr(path, NULL, '.');
		if(ext)
		{
			ext[0] = 0;
			StrCat(path, L".lrc");
		}
	}
	else if(cfg_lrc_save_path.get_ptr()[0])
	{
		TCHAR temp[255];
		pfc::stringcvt::convert_utf8_to_wide(path, 255, cfg_lrc_save_path, lstrlenA(cfg_lrc_save_path));
		pfc::stringcvt::convert_utf8_to_wide(temp, 255, str.get_ptr(), str.length());
		StrCat(path, L"\\");
		StrCat(path, temp);
		StrCat(path, L".lrc");
	}

	//else
	//{
	//	pfc::stringcvt::convert_utf8_to_wide(path, 255, cfg_lrc_save_path, lstrlenA(cfg_lrc_save_path));
	//	StrCat(path, L".lrc");
	//}
}

void Common_UI_Base::on_playback_pause(bool p_state)
{
	if(Lyric->GetLyric(0))
	{
		if(p_state == true)
		{
			TerminateThread(hLyricThread, 0);//TODO: 쓰지 말란다.
			CloseHandle(hLyricThread);
			hLyricThread = 0;
		}
		else 
		{
			hLyricThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)LyricCountThread, (LPVOID)this, NULL, NULL);
		}
	}
}

void Common_UI_Base::on_playback_stop(play_control::t_stop_reason reason)
{
	if(Lyric->GetLyric(0))
	{
		TerminateThread(hLyricThread, 0);//TODO: 쓰지 말란다.
		CloseHandle(hLyricThread);
		hLyricThread = 0;
	}

	Lyric->ClearLyric();

	InvalidateAllWindow();
	
	std::for_each(WndInfo.begin(), WndInfo.end(), uSetWindowTextStub("Alsong Lyric"));
}

UINT CALLBACK Common_UI_Base::LyricCountThread(LPVOID lpParameter)
{
	int i;
	int WaitTime;
	int NowTime_Local;
	Common_UI_Base *_this = (Common_UI_Base *)lpParameter;

	if(_this->NowTime != 0) //처음부터 시작한 것이 아닐 때
	{
		int temp;
		WaitTime = 2147483647;
		temp = 0;
		i = 0;
		for(i = 0; i < (int)_this->Lyric->GetNumberOfLine(); i ++) //가장 작은 차이 찾기
		{
			if(WaitTime > (int)_this->NowTime - (int)_this->Lyric->GetLyricTime(i) && (int)_this->NowTime - (int)_this->Lyric->GetLyricTime(i) > 0 && _this->Lyric->GetLyricTime(i) != 0)
			{
				WaitTime = _this->NowTime - (int)_this->Lyric->GetLyricTime(i);
				temp = i;
			}
			if(WaitTime > (int)_this->Lyric->GetLyricTime(i) - (int)_this->NowTime && _this->Lyric->GetLyricTime(i) - _this->NowTime < 100)
			{
				WaitTime = (int)_this->Lyric->GetLyricTime(i) - _this->NowTime;
				temp = i; //1초보다 작은 차이일때는 다음 가사로
			}
		}
		_this->NowLine = temp;
	}
	_this->InvalidateAllWindow();

	i = 0;
	for(i = 0; i < (int)_this->Lyric->GetNumberOfLine(); i ++)
	{
		int j;

		if(_this->Lyric->GetLyricTime(i) == 0)
			continue;

		NowTime_Local = _this->NowTime;

		for(j = i; j < (int)_this->Lyric->GetNumberOfLine(); j ++)
		{
			if(_this->Lyric->GetLyricTime(j) == _this->Lyric->GetLyricTime(j - 1))
				continue; //한번에 여러줄 있는 경우는 처음꺼부터 보이게

			WaitTime = _this->Lyric->GetLyricTime(j) - NowTime_Local;
			if(WaitTime < 0)
				continue;
			if(WaitTime >= 100) //1초 초과
				break;
			if(WaitTime != 0)
			{
				int id = timeSetEvent(WaitTime * 10, 0, (LPTIMECALLBACK)_this->hSleep, 0, TIME_CALLBACK_EVENT_SET | TIME_ONESHOT); //정확할까?
				WaitForSingleObject(_this->hSleep, INFINITE);
				timeKillEvent(id);
			}
			ResetEvent(_this->hSleep);
			_this->NowLine = j;
			_this->InvalidateAllWindow();
			NowTime_Local += WaitTime;
		}
		i = j - 1; //++되므로 -1;

		WaitForSingleObject(_this->hTime, INFINITE); //1초 간격 될때까지 대기
		ResetEvent(_this->hTime);
	}

	return 0;
}

void Common_UI_Base::on_playback_time(double p_time)
{
	if(Lyric->GetLyric(0) != NULL) // 가사 있음
	{
		NowTime = (DWORD)p_time * 100;
		SetEvent(hTime);
	}
}

void Common_UI_Base::on_playback_seek(double p_time)
{
	int i = 0;
	int MinTime = 2147483647;
	int temp;
	
	if(Lyric->GetLyric(0))
	{
		TerminateThread(hLyricThread, 0); //TODO: 쓰지 말란다
		CloseHandle(hLyricThread);
		hLyricThread = 0;

		NowTime = (DWORD)p_time * 100;

		for(i = 0; i < (int)Lyric->GetNumberOfLine(); i ++)
		{
			if(Lyric->GetLyricTime(i) == 0)
				continue;

			if(MinTime > (int)NowTime - (int)Lyric->GetLyricTime(i) && (int)NowTime - (int)Lyric->GetLyricTime(i) > 0 && Lyric->GetLyricTime(i) != 0)
			{
				MinTime = (int)NowTime - (int)Lyric->GetLyricTime(i);
				temp = i;
			}
		}
		if(MinTime == 2147483647)
			NowLine = 0;
		else
			NowLine = temp;

		InvalidateAllWindow();

		hLyricThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)LyricCountThread, (LPVOID)this, NULL, NULL); //스레드 재생성
	}
}
//TODO: 정비

HWND hToolTipWindow;

void CreateToolTipWindow(HWND hWnd)
{
	TOOLINFO ti;

	hToolTipWindow = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hWnd, NULL, NULL, NULL);
	SetWindowPos(hToolTipWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

	ti.cbSize = sizeof(TOOLINFO);
	ti.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
	ti.hwnd = hWnd;
	ti.hinst = NULL;
	ti.uId = (UINT)hToolTipWindow;
	ti.lpszText = LPSTR_TEXTCALLBACK;
	ti.rect.left = ti.rect.top = ti.rect.bottom = ti.rect.right = 0; 

	SendMessage(hToolTipWindow, TTM_ADDTOOL, 0, (LPARAM)&ti);
	SendMessage(hToolTipWindow, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&ti);

}

LRESULT Common_UI_Base::Process_Message(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam, Alsong_Setting *NowSetting, BOOL isOuter)
{
	if(core_api::is_shutting_down() || core_api::is_initializing())
	{
		if(iMessage == WM_CREATE)
		{
			CreateToolTipWindow(hWnd);
		}
		return DefWindowProc(hWnd, iMessage, wParam, lParam);
	}
	
	if(WndInfo.find(hWnd) == WndInfo.end())
	{
		if(!NowSetting->font.m_facename[0]) // 처음 실행시 값이 없다.
		{
			NowSetting->font.from_font((HFONT)GetStockObject(DEFAULT_GUI_FONT));
			NowSetting->nLine = 3; //기본값 3줄
			NowSetting->bgType = false;
			NowSetting->bkColor = RGB(255, 255, 255);
		}
		WindowData *Data = new WindowData(NowSetting);

		WndInfo.insert(std::make_pair(hWnd, Data));
		InvalidateRect(hWnd, NULL, TRUE);
	}

	if(WndInfo[hWnd]->UpdateData(NowSetting) == TRUE)
	{
		RECT rt;
		GetClientRect(hWnd, &rt);

		WndInfo[hWnd]->ForceUpdateCache(rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top);

		InvalidateRect(hWnd, NULL, TRUE);
	}
	
	switch(iMessage)
	{
	case WM_NOTIFY:
		if(((NMHDR *)lParam)->code == TTN_GETDISPINFO)
		{
			SendMessage(((NMHDR *)lParam)->hwndFrom, TTM_SETMAXTIPWIDTH, 0, 300);
			((NMTTDISPINFO *)lParam)->lpszText = L"Test";
		}

		return 0;
	case WM_NCMOUSEMOVE:
		SendMessage(hToolTipWindow, TTM_TRACKPOSITION, 0, MAKELPARAM(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));

		return 0;
	case WM_KEYDOWN:
		on_keydown(wParam);
		return 0;

	case WM_SYSKEYDOWN:
		on_keydown(wParam);
		return 0;

	case WM_DESTROY:
		return 0;

	case WM_SIZE:
		KillTimer(hWnd, 0);
		SetTimer(hWnd, 0, 100, NULL); //0.1초간 기다렸다가 안움직이면 업데이트
		DeleteObject(WndInfo[hWnd]->GetBackBuffer());
		WndInfo[hWnd]->SetBackBuffer(NULL);
		return 0;

	case WM_TIMER:
		{
			RECT rt;
			GetClientRect(hWnd, &rt);

			WndInfo[hWnd]->ForceUpdateCache(rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top);

			InvalidateRect(hWnd, NULL, TRUE);

			KillTimer(hWnd, 0);
			return 0;
		}

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			if(BeginPaint(hWnd, &ps) != NULL) 
			{
				RECT ClientRect;
				GetClientRect(hWnd, &ClientRect);
				//TODO:Gdiplus로
				HDC hMemDC = CreateCompatibleDC(ps.hdc); //따블 버퍼링
				if(WndInfo[hWnd]->GetBackBuffer() == NULL)
					WndInfo[hWnd]->SetBackBuffer(CreateCompatibleBitmap(ps.hdc, ClientRect.right, ClientRect.bottom));

				HBITMAP OldBitMap = (HBITMAP)SelectObject(hMemDC, WndInfo[hWnd]->GetBackBuffer());

				if(NowSetting->bgType && NowSetting->bgImage[0])
				{
					if(WndInfo[hWnd]->DrawImage(hMemDC, ClientRect.left, ClientRect.top, ClientRect.right, ClientRect.bottom) != Gdiplus::Ok)
					{
						//실패시
						HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
						FillRect(hMemDC, &ClientRect, hBrush);
						DeleteObject(hBrush);
					}
				}
				else
				{ //TODO:브러시 없음도 될까? 투명 배경
					HBRUSH hBrush = CreateSolidBrush(NowSetting->bkColor);
					FillRect(hMemDC, &ClientRect, hBrush);
					DeleteObject(hBrush);
				}

				HFONT OldFont;
				HFONT hFont = NowSetting->font.create();
				OldFont = (HFONT)SelectObject(hMemDC, hFont);
				SetBkMode(hMemDC, TRANSPARENT);
				SetTextColor(hMemDC, NowSetting->fgColor);
			
				if(Lyric->GetLyric(0) != NULL && static_api_ptr_t<play_control>()->is_playing()) // 가사 있음
				{
					TEXTMETRIC tm;
					GetTextMetrics(hMemDC, &tm);
					int i, emptycnt;
					int start;
					int end;
					if(Lyric->GetLyricTime(NowLine) == Lyric->GetLyricTime(NowLine + 1)) //동시에 표시되는게 두 줄 이상일 때
					{
						int nSameLine;
						//줄 수 세기
						for(i = NowLine; ; i ++)
							if(Lyric->GetLyricTime(NowLine) != Lyric->GetLyricTime(i))
								break;

						nSameLine = i - NowLine;
						start = NowLine - (int)(NowSetting->nLine - nSameLine) / 2;
						end = min(start + NowSetting->nLine, (int)Lyric->GetNumberOfLine());
					}
					else //한줄일때는 가운데에 출력
					{
						start = (int)NowLine - (int)floor((double)NowSetting->nLine / 2);
						end = min(start + NowSetting->nLine, (int)Lyric->GetNumberOfLine());
					}
					if(start < 0) //곡의 첫부분에서는 시작이 음수부분이다
					{
						emptycnt = -start;
						end = min(start + NowSetting->nLine, (int)Lyric->GetNumberOfLine());
						start = 0; //처음 위치부터 시작한다
					}
					else
						emptycnt = 0;

					WCHAR OutLine[1024]; //크게
					RECT rt;
					GetClientRect(hWnd, &rt);
#ifdef _DEBUG
					rt.left = 50;
#endif
					rt.top = emptycnt * tm.tmHeight;
					int LineMargin = (int)(tm.tmHeight * 0.2);
					for(i = start; i < end; i ++)
					{
						RECT trt = {0,};
						memcpy(&trt, &rt, sizeof rt);
#ifdef _DEBUG
						TCHAR temp[10];
						wsprintf(temp, L"%d", Lyric->GetLyricTime(i));
						TextOut(hMemDC, 0, rt.top, temp, lstrlen(temp)); //시간
#endif

						pfc::stringcvt::convert_utf8_to_wide(OutLine, 1024, Lyric->GetLyric(i), lstrlenA(Lyric->GetLyric(i)));

						if(Lyric->GetLyricTime(i) == Lyric->GetLyricTime(NowLine)) //현재줄은 굵게
						{
							t_font_description font = NowSetting->font;
							font.m_weight = FW_BOLD;
							HFONT hBoldFont = font.create();
							HFONT hOldFont = (HFONT)SelectObject(hMemDC, hBoldFont);

							//DrawText로 Clipping
							//Bold일 때는 크기가 달라지기 때문에 같은 코드를 두 번 쓴다.
							DrawText(hMemDC, OutLine, lstrlenW(OutLine), &rt, DT_TOP | DT_LEFT | DT_WORDBREAK | DT_EDITCONTROL | DT_NOPREFIX);
							DrawText(hMemDC, OutLine, lstrlenW(OutLine), &trt, DT_TOP | DT_LEFT | DT_CALCRECT | DT_WORDBREAK | DT_EDITCONTROL | DT_NOPREFIX); //크기재기

							DeleteObject(SelectObject(hMemDC, hOldFont));

						}
						else
						{
							//DrawText로 Clipping
							DrawText(hMemDC, OutLine, lstrlenW(OutLine), &rt, DT_TOP | DT_LEFT | DT_WORDBREAK | DT_EDITCONTROL | DT_NOPREFIX);
							DrawText(hMemDC, OutLine, lstrlenW(OutLine), &trt, DT_TOP | DT_LEFT | DT_CALCRECT | DT_WORDBREAK | DT_EDITCONTROL | DT_NOPREFIX); //크기재기
						}
						if(trt.bottom - trt.top == 0)
							rt.top += tm.tmHeight + LineMargin;
						else
							rt.top += (trt.bottom - trt.top) + LineMargin;
					}
					/*
					album_art_manager_instance_ptr aami =
						static_api_ptr_t<album_art_manager>()->instantiate();
					try
					{
						foobar2000_io::abort_callback_impl abort;
						aami->open(NowPlaying_Path, abort);
						album_art_data_ptr pdata =    
							aami->query(album_art_ids::cover_front, abort);    
						
					}
					catch (...) {}*/
				}
				else
				{
					metadb_handle_ptr p_track;
					static_api_ptr_t<play_control> pc;
					pc->get_now_playing(p_track);

					if(p_track != NULL)
					{/*
						file_info_impl file;
						p_track->get_info(file);
						const char *temp = file.meta_get("TITLE", 0);
						*/
						
						service_ptr_t<titleformat_object> to;
						pfc::string8 str;
						
						static_api_ptr_t<titleformat_compiler>()->compile_safe(to, "[%artist% - ]%title%");
						p_track->format_title(NULL, str, to, NULL);

						uExtTextOut(hMemDC, 0, 0, NULL, NULL, str, str.length(), NULL);

						ExtTextOut(hMemDC, 0, 30, NULL, NULL, Lyric->GetStatus(), lstrlen(Lyric->GetStatus()), NULL);
					}
				}

				BitBlt(ps.hdc, 0, 0, ClientRect.right, ClientRect.bottom, hMemDC, 0, 0, SRCCOPY);

				SelectObject(hMemDC, OldBitMap);
				DeleteDC(hMemDC);
				DeleteObject(hFont);
				EndPaint(hWnd, &ps);

				return 0;
			}
			return 0;
		}		

	}
	return DefWindowProc(hWnd, iMessage, wParam, lParam);
}

bool Common_UI_Base::on_keydown(WPARAM wParam) 
{
	// Return value indicates whether the key has been translated.
	bool rv = false;

	try 
	{
		// List for passing metadb_handle(s) to keyboard_shortcut_manager.
		metadb_handle_list items;
		// Get the currently playing item
		static_api_ptr_t<play_control> pc;
		metadb_handle_ptr handle;
		if (pc->get_now_playing(handle)) 
		{
			// If there is a currently playing item, add it to the list.
			items.add_item(handle);
		}

		// Pass metadb_handles and  key code to keyboard_shortcut_manager.
		static_api_ptr_t<keyboard_shortcut_manager> ksm;
		rv = ksm->on_keydown_auto_context(items, wParam, contextmenu_item::caller_undefined);

		// metadb_handles in items are automatically released.
	}
	catch (const exception_service_not_found &) 
	{

	}

	return rv;
}
