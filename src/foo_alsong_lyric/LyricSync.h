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

class LyricSyncDialog :  public play_callback
{
private:
	HWND m_hWnd;
	metadb_handle_ptr m_track;
	boost::shared_ptr<boost::thread> m_syncThread;
	LyricSyncDialog(metadb_handle_ptr &track, HWND hParent);
	static UINT CALLBACK DialogProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

	void Initialize(HWND hWnd);
	void OnTrackbarMove(unsigned int pos);
	void SyncThread();
	void UpdateTrackbar(double pos);
public:
	~LyricSyncDialog();
	static void Open(metadb_handle_ptr &track, HWND hParent);

	//play_callback
	
	virtual void FB2KAPI on_playback_new_track(metadb_handle_ptr p_track);
	virtual void FB2KAPI on_playback_stop(play_control::t_stop_reason p_reason);
	virtual void FB2KAPI on_playback_seek(double p_time);
	virtual void FB2KAPI on_playback_pause(bool p_state);
	virtual void FB2KAPI on_playback_time(double p_time);
	
	virtual void FB2KAPI on_playback_starting(play_control::t_track_command p_command,bool p_paused) {};
	virtual void FB2KAPI on_playback_edited(metadb_handle_ptr p_track) {};
	virtual void FB2KAPI on_playback_dynamic_info(const file_info & p_info) {};
	virtual void FB2KAPI on_playback_dynamic_info_track(const file_info & p_info) {};
	virtual void FB2KAPI on_volume_change(float p_new_val) {};
};