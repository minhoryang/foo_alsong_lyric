#include "stdafx.h"
#include "Common_Lyric_Manipulation.h"
#include "resource.h"

//TODO:멀티스레드

struct SLyricInfo
{
	string Title;
	string Album;
	string Registrant;
	string Lyric;
	string Artist;

	int nInfo;
};

void FillListView(SLyricInfo *LyricInfo, int nLyric, HWND hLyricList)
{
	ListView_DeleteAllItems(hLyricList);
	int i;
	LVITEM lv;
	lv.mask = LVIF_TEXT;
	TCHAR temp[255];
	for(i = 0; i < nLyric; i ++)
	{
		lv.iSubItem = 0;
		lv.iItem = i;
		temp[MultiByteToWideChar(CP_UTF8, NULL, LyricInfo[i].Artist.c_str(), lstrlenA(LyricInfo[i].Artist.c_str()), temp, 255)] = 0;
		lv.pszText = temp;
		ListView_InsertItem(hLyricList, &lv);
		lv.iSubItem = 1;
		temp[MultiByteToWideChar(CP_UTF8, NULL, LyricInfo[i].Title.c_str(), lstrlenA(LyricInfo[i].Title.c_str()), temp, 255)] = 0;
		ListView_SetItem(hLyricList, &lv);
	}
}

void FillListViewColumn(HWND hLyricList)
{
	LVCOLUMN lv;
	lv.mask = LVCF_WIDTH | LVCF_TEXT;
	lv.cx = 150;
	lv.pszText = TEXT("아티스트");
	ListView_InsertColumn(hLyricList, 0, &lv);
	lv.pszText = TEXT("제목");
	ListView_InsertColumn(hLyricList, 1, &lv);
}

WNDPROC OldProc;
WNDPROC OldProc1;

LRESULT CALLBACK HookProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	switch(iMessage)
	{
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_CHAR:
		return 0;
	}
	return CallWindowProc(OldProc, hWnd, iMessage, wParam, lParam);
}

LRESULT CALLBACK HookProc1(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	if(iMessage == WM_KEYDOWN)
	{
		if(wParam == VK_RETURN)
		{
			SendMessage(GetParent(hWnd), WM_COMMAND, MAKEWPARAM(IDC_SEARCH, 0), 0);
			return 0;
		}
	}
	return CallWindowProc(OldProc1, hWnd, iMessage, wParam, lParam);
}

void FillLyric(HWND hDialog, int NowPage, SLyricInfo *LyricInfo, int nLyric)
{
	WCHAR ArtistW[255];
	WCHAR TitleW[255];
	CHAR TitleU[255];
	CHAR ArtistU[255];

	CHAR *NowData = NULL;

	GetDlgItemText(hDialog, IDC_ARTIST, ArtistW, 255);
	GetDlgItemText(hDialog, IDC_TITLE, TitleW, 255);
	WideCharToMultiByte(CP_UTF8, NULL, ArtistW, 255, ArtistU, 255, NULL, NULL);
	WideCharToMultiByte(CP_UTF8, NULL, TitleW, 255, TitleU, 255, NULL, NULL);

	Common_Lyric_Manipulation::SearchLyric(&string(ArtistU), &string(TitleU), NowPage, &NowData);
	CHAR *otemp;
	otemp = NowData;
	int i;
	for(i = 0; i < min(nLyric, 100); i ++)
	{
		Common_Lyric_Manipulation::SearchLyricGetNext(&NowData, &LyricInfo[i].nInfo, &LyricInfo[i].Title, &LyricInfo[i].Artist, &LyricInfo[i].Album, &LyricInfo[i].Lyric, &LyricInfo[i].Registrant);
		while(LyricInfo[i].Lyric.find("&lt;br&gt;") != string::npos)
			LyricInfo[i].Lyric.replace(LyricInfo[i].Lyric.find("&lt;br&gt;"), 10, "\r\n");
		while(LyricInfo[i].Lyric.find("&amp;") != string::npos)
			LyricInfo[i].Lyric.replace(LyricInfo[i].Lyric.find("&amp;"), 5, "&");
		while(LyricInfo[i].Lyric.find("&lt;") != string::npos)
			LyricInfo[i].Lyric.replace(LyricInfo[i].Lyric.find("&lt;"), 4, "<");
		while(LyricInfo[i].Lyric.find("&gt;") != string::npos)
			LyricInfo[i].Lyric.replace(LyricInfo[i].Lyric.find("&gt;"), 4, ">");
	}
	free(otemp);
	NowData = NULL;

	FillListView(LyricInfo, min(nLyric, 100), GetDlgItem(hDialog, IDC_LYRICLIST));
	TCHAR temp[255];
	if(NowPage == nLyric / 100)
		wsprintf(temp, TEXT("%d~%d/%d"), NowPage * 100 + 1, nLyric, nLyric);
	else if(nLyric < 100)
		wsprintf(temp, TEXT("1~%d/%d"), nLyric, nLyric);
	else
		wsprintf(temp, TEXT("%d~%d/%d"), NowPage * 100 + 1, min(nLyric, (NowPage + 1) * 100), nLyric);

	SetDlgItemText(hDialog, IDC_STATUS, temp);
}

int GetNumberOfLyric(HWND hDialog)
{
	WCHAR ArtistW[255];
	WCHAR TitleW[255];
	CHAR TitleU[255];
	CHAR ArtistU[255];

	GetDlgItemText(hDialog, IDC_ARTIST, ArtistW, 255);
	GetDlgItemText(hDialog, IDC_TITLE, TitleW, 255);
	WideCharToMultiByte(CP_UTF8, NULL, ArtistW, 255, ArtistU, 255, NULL, NULL);
	WideCharToMultiByte(CP_UTF8, NULL, TitleW, 255, TitleU, 255, NULL, NULL);
	
	return Common_Lyric_Manipulation::SearchLyricGetCount(&string(ArtistU), &string(TitleU));
}

struct FileInfo
{
	CHAR *Title;
	CHAR *Artist;
	CHAR *FileName;
	double length;
};

BOOL CALLBACK LyricModifyDialogProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	static int nLyric;
	static int NowPage = 0;
	static struct SLyricInfo LyricInfo[100];
	static CHAR FileName[255];
	static double length;
	switch (iMessage)
	{
	case WM_INITDIALOG:
		uSetDlgItemText(hWnd, IDC_ARTIST, ((FileInfo *)lParam)->Artist);
		uSetDlgItemText(hWnd, IDC_TITLE, ((FileInfo *)lParam)->Title);
		lstrcpyA(FileName, ((FileInfo *)lParam)->FileName);
		length = ((FileInfo *)lParam)->length;

		FillListViewColumn(GetDlgItem(hWnd, IDC_LYRICLIST));
		OldProc = (WNDPROC)SetWindowLong(GetDlgItem(hWnd, IDC_LYRIC), GWL_WNDPROC, (LONG)HookProc);
		OldProc1 = (WNDPROC)SetWindowLong(GetDlgItem(hWnd, IDC_ARTIST), GWL_WNDPROC, (LONG)HookProc1);
		OldProc1 = (WNDPROC)SetWindowLong(GetDlgItem(hWnd, IDC_TITLE), GWL_WNDPROC, (LONG)HookProc1);


		TCHAR *temp;
		temp = L"가사 등록 - %s";
		CHAR tempc[255];
		CHAR tempp[255];
		pfc::stringcvt::convert_wide_to_utf8(tempc, 255, temp, lstrlen(temp));
		wsprintfA(tempp, tempc, FileName);
		uSetWindowText(hWnd, tempp);
		break;
	case WM_CLOSE:
		EndDialog(hWnd, 1);
		break;
	case WM_NOTIFY:
		switch(((NMHDR *)lParam)->code)
		{
		case LVN_ITEMCHANGED:
			int nSel;
			nSel = SendMessage(GetDlgItem(hWnd, IDC_LYRICLIST), LVM_GETSELECTIONMARK, 0, 0);
			uSetWindowText(GetDlgItem(hWnd, IDC_LYRIC), LyricInfo[nSel].Lyric.c_str());
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_SEARCH:
			NowPage = 0;
			nLyric = GetNumberOfLyric(hWnd);
			if(nLyric <= 0)
			{
				//TODO: 에러처리
				MessageBox(hWnd, L"검색 실패", L"검색 실패", MB_OK);
				break;
			}
			FillLyric(hWnd, NowPage, LyricInfo, nLyric);

			break;
		case IDC_RESET:
			SetDlgItemText(hWnd, IDC_ARTIST, TEXT(""));
			SetDlgItemText(hWnd, IDC_TITLE, TEXT(""));
			SetDlgItemText(hWnd, IDC_STATUS, TEXT(""));
			ListView_DeleteAllItems(GetDlgItem(hWnd, IDC_LYRICLIST));
			SetDlgItemText(hWnd, IDC_LYRIC, TEXT(""));
			SetFocus(GetDlgItem(hWnd, IDC_ARTIST));
			break;
		case IDC_NEXT:
			if(nLyric < 100)
				break;

			if(NowPage == nLyric / 100)
				break;
			NowPage ++;
			ListView_DeleteAllItems(GetDlgItem(hWnd, IDC_LYRICLIST));
			FillLyric(hWnd, NowPage, LyricInfo, nLyric);

			break;
		case IDC_PREV:
			if(nLyric < 100)
				break;
			if(NowPage == 0)
				break;

			NowPage --;
			ListView_DeleteAllItems(GetDlgItem(hWnd, IDC_LYRICLIST));
			FillLyric(hWnd, NowPage, LyricInfo, nLyric);
			break;
		case IDC_REGISTER:
			{
				int nSel;
				nSel = SendMessage(GetDlgItem(hWnd, IDC_LYRICLIST), LVM_GETSELECTIONMARK, 0, 0);
				int nRet;
				service_ptr_t<file> file;
				abort_callback_impl abort_callback;
				archive_impl::g_open(file, FileName, foobar2000_io::filesystem::open_mode_read, abort_callback);
				nRet = Common_Lyric_Manipulation::UploadLyric(&file, &string(FileName), (int)(length * 1000), LyricInfo[nSel].nInfo, 1, &LyricInfo[nSel].Lyric, &LyricInfo[nSel].Title, &LyricInfo[nSel].Artist, &LyricInfo[nSel].Album, &LyricInfo[nSel].Registrant);
				if(nRet == S_OK)
				{
					MessageBox(hWnd, L"등록 성공", L"안내", MB_OK);
					EndDialog(hWnd, 1);
				}
				else
				{
					MessageBox(hWnd, L"등록 실패", L"안내", MB_OK);
					EndDialog(hWnd, 0);
				}
				break;
			}
		case IDC_CANCEL:
			EndDialog(hWnd, 0);
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

int OpenLyricModifyDialog(HWND hParent, CHAR *Title, CHAR *Artist, CHAR *FileName, double length)
{
	FileInfo File;
	File.Artist = Artist;
	File.Title = Title;
	File.FileName = FileName;
	File.length = length;
	return DialogBoxParam(core_api::get_my_instance(), MAKEINTRESOURCE(IDD_LYRIC_MODIFY), hParent, LyricModifyDialogProc, (LPARAM)&File);
}
