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

#include "stdafx.h"
#include "resource.h"
#include "LyricSync.h"

LyricSyncDialog::LyricSyncDialog(metadb_handle_ptr &track, HWND hParent) : m_track(track)
{
	DialogBoxParam(core_api::get_my_instance(), MAKEINTRESOURCE(IDD_SYNCLYRIC), hParent, (DLGPROC)&LyricSyncDialog::DialogProc, (LPARAM)this);
}

void LyricSyncDialog::Open(metadb_handle_ptr &track, HWND hParent)
{
	LyricSyncDialog dlg(track, hParent);
}

UINT CALLBACK LyricSyncDialog::DialogProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	static LyricSyncDialog *_this = NULL;
	switch(iMessage)
	{
	case WM_INITDIALOG:
		_this = (LyricSyncDialog *)lParam;
		return TRUE;
	}
	return FALSE;
}