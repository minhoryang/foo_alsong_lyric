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

boost::shared_ptr<LyricSyncDialog> g_lyricsyncinst;

LyricSyncDialog::LyricSyncDialog(metadb_handle_ptr &track, HWND hParent) : m_track(track)
{
	static_api_ptr_t<play_callback_manager> pcm;
	pcm->register_callback(this, flag_on_playback_all, false);

	HWND hWnd = CreateDialogParam(core_api::get_my_instance(), MAKEINTRESOURCE(IDD_SYNCLYRIC), hParent, (DLGPROC)&LyricSyncDialog::DialogProc, (LPARAM)this);
	ShowWindow(hWnd, SW_SHOW);
	m_syncThread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&LyricSyncDialog::SyncThread, this)));
}

LyricSyncDialog::~LyricSyncDialog()
{
	static_api_ptr_t<play_callback_manager> pcm;
	pcm->unregister_callback(this);
	m_syncThread->interrupt();
	m_syncThread->join();
}

void LyricSyncDialog::SyncThread()
{
	class SyncCallback : public main_thread_callback
	{
	private:
		LyricSyncDialog *m_this;
	public:
		SyncCallback(LyricSyncDialog *_this) : m_this(_this) {}
		virtual void callback_run()
		{
			static_api_ptr_t<playback_control> pc;
			metadb_handle_ptr handle;
			double seconds = pc->playback_get_position();
			m_this->UpdateTrackbar(seconds);
		}
	};
	service_ptr_t<SyncCallback> p_callback = new service_impl_t<SyncCallback>(this);

	while(1)
	{
		if(boost::this_thread::interruption_requested())
			break;
		boost::this_thread::sleep(boost::posix_time::milliseconds(50));

		static_api_ptr_t<main_thread_callback_manager>()->add_callback(p_callback);
	}

	p_callback.release();
}

void LyricSyncDialog::Open(metadb_handle_ptr &track, HWND hParent)
{
	if(!g_lyricsyncinst)
		g_lyricsyncinst = boost::shared_ptr<LyricSyncDialog>(new LyricSyncDialog(track, hParent));
}


void LyricSyncDialog::on_playback_new_track(metadb_handle_ptr p_track)
{
	m_track = p_track;
	double len = m_track->get_length();
	SendMessage(GetDlgItem(m_hWnd, IDC_SYNC_TIME), TBM_SETRANGEMIN, TRUE, 0);
	SendMessage(GetDlgItem(m_hWnd, IDC_SYNC_TIME), TBM_SETRANGEMAX, TRUE, int(len * 10000));
	uSetWindowText(m_hWnd, m_track->get_path());
}

void LyricSyncDialog::on_playback_stop(play_control::t_stop_reason p_reason)
{

}

void LyricSyncDialog::UpdateTrackbar(double pos)
{
	SendMessage(GetDlgItem(m_hWnd, IDC_SYNC_TIME), TBM_SETPOS, TRUE, int(pos * 10000));
}

void LyricSyncDialog::on_playback_seek(double p_time)
{
	SendMessage(GetDlgItem(m_hWnd, IDC_SYNC_TIME), TBM_SETPOS, TRUE, int(p_time * 10000));
}

void LyricSyncDialog::on_playback_pause(bool p_state)
{

}

void LyricSyncDialog::on_playback_time(double p_time)
{
	SendMessage(GetDlgItem(m_hWnd, IDC_SYNC_TIME), TBM_SETPOS, TRUE, int(p_time * 10000));
}

void LyricSyncDialog::Initialize(HWND hWnd)
{
	m_hWnd = hWnd;
	on_playback_new_track(m_track);
}

void LyricSyncDialog::OnTrackbarMove(unsigned int pos)
{
	static_api_ptr_t<play_control> pc;
	pc->playback_seek((double)pos / 10000);
}

UINT CALLBACK LyricSyncDialog::DialogProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	static LyricSyncDialog *_this = NULL;
	switch(iMessage)
	{
	case WM_INITDIALOG:
		_this = (LyricSyncDialog *)lParam;
		_this->Initialize(hWnd);
		return TRUE;
	case WM_COMMAND:
		if(HIWORD(wParam) == BN_CLICKED)
		{
			switch(LOWORD(wParam))
			{
			case IDCANCEL:
				EndDialog(hWnd, 1);
				return TRUE;
			}
		}
		return TRUE;
	case WM_HSCROLL:
		{
			switch(LOWORD(wParam))
			{
			case TB_THUMBTRACK:
				{
					DWORD pos = SendMessage(GetDlgItem(hWnd, IDC_SYNC_TIME), TBM_GETPOS, 0, 0);
					//pos updated
					_this->OnTrackbarMove(pos);
				}
			}
		}
		return TRUE;
	case WM_CLOSE:
		EndDialog(hWnd, 1);
		return TRUE;
	case WM_DESTROY:
		g_lyricsyncinst.reset();
		return TRUE;
	}
	return FALSE;
}