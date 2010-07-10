#pragma once

#include "LyricSourceAlsong.h"
#include "LyricSearchResult.h"

class AlsongLyricLinkDialog
{
private:
	HWND m_hWnd;
	metadb_handle_ptr m_track;
	int m_lyriccount;
	boost::shared_ptr<LyricSearchResult> m_searchresult;
	int m_page;

	static UINT CALLBACK LyricModifyDialogProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
	UINT DialogProc(UINT iMessage, WPARAM wParam, LPARAM lParam);
	void PopulateListView();
public:
	AlsongLyricLinkDialog(HWND hWndParent, const metadb_handle_ptr &track = NULL);
	~AlsongLyricLinkDialog();

	static void OpenLyricLinkDialog(HWND hWndParent, const metadb_handle_ptr &track = NULL);
};