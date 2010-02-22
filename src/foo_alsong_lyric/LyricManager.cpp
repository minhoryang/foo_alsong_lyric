#include "stdafx.h"
#include "ConfigStore.h"
#include "AlsongUI.h"
#include "LyricManager.h"
#include "pugixml/pugixml.hpp"
#include "md5.h"
//TODO: USLT 태그(4바이트 타임스탬프, 1바이트 길이, 문자열(유니코드), 0x08 순으로 들어있음)

LyricManager *LyricManagerInstance;

LyricManager::LyricManager() : m_Lyricpos(-1), m_Seconds(0)
{
	static_api_ptr_t<play_callback_manager> pcm;
	pcm->register_callback(this, flag_on_playback_all, false);
}

LyricManager::~LyricManager()
{
	m_fetchthread->interrupt();
	m_fetchthread->join();
	static_api_ptr_t<play_callback_manager> pcm;
	pcm->unregister_callback(this);
}

void LyricManager::on_playback_seek(double p_time)
{
}

void LyricManager::on_playback_new_track(metadb_handle_ptr p_track)
{
	if(m_fetchthread)
	{
		m_fetchthread->interrupt();
		m_fetchthread->join();
		m_fetchthread.reset();
	}
	if(m_countthread)
	{
		m_countthread->interrupt();
		m_countthread->join();
		m_countthread.reset();
	}
	m_fetchthread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&LyricManager::FetchLyric, this, p_track)));
	begin = boost::posix_time::microsec_clock::universal_time();
	tick = begin; //0sec
	m_Seconds = 0;
}

void LyricManager::on_playback_stop(play_control::t_stop_reason reason)
{
}
void LyricManager::on_playback_time(double p_time)
{
	tick = boost::posix_time::microsec_clock::universal_time();
	m_Seconds = (int)p_time;
}
void LyricManager::on_playback_pause(bool p_state)
{
}

const char *LyricManager::GetLyricBefore(int n)
{
	return NULL;
}

const char *LyricManager::GetLyric()
{
	if(m_Lyricpos >= 0)
		return m_Lyric[m_Lyricpos].lyric.get_ptr();
	return NULL;
}

const char *LyricManager::GetLyricAfter(int n)
{
	return NULL;
}

DWORD LyricManager::GetFileHash(metadb_handle_ptr track, CHAR *Hash)
{	
	int i;
	int tmp;
	DWORD Start; //Start Address
	BYTE MD5[16];
	service_ptr_t<file> file;
	abort_callback_impl abort_callback;
	pfc::string8 str = track->get_path();
	
	archive_impl::g_open(file, str, foobar2000_io::filesystem::open_mode_read, abort_callback);
	//TODO:cue일때 특별 처리(subsong_index가 있을 때)
	char *fmt = (char *)str.get_ptr() + str.find_last('.') + 1;
	file_info_impl info;
	track->get_info(info);
	const char *ttmp = info.info_get("referenced_offset");
	BYTE temp[255]; //아래코드 이용해서 cue에서 raw 뽑아와서 hash생성
	/*bool void g_decode_file(char const * p_path, abort_callback & p_abort)
{
    try
    {
        input_helper helper;
        file_info_impl info;

        // open input
        helper.open(service_ptr_t<file>(), make_playable_location(p_path, 0), input_flag_simpledecode, p_abort);

        helper.get_info(0, info, p_abort);
        if (info.get_length() <= 0)
            throw pfc::exception("Track length invalid");

        audio_chunk_impl chunk;

        if (!helper.run(chunk, p_abort)) return false;


        t_uint64 length_samples = audio_math::time_to_samples(info.get_length(), chunk.get_sample_rate());
        //chunk.get_channels();

        while (true)
        {
            // Store the data somewhere.

            bool decode_done = !helper.run(chunk, p_abort);
            if (decode_done) break;
        }

        // We now have the full data.

        return true;
    }
    catch (const exception_aborted &) {throw;}
    catch (const std::exception & exc)
    {
        console::formatter() << exc << ": " << p_path;
        return false;
    }
};
*/

	try
	{
		if(!StrCmpIA(fmt, "mp3"))
		{ //MP3	
			tmp = 0;
			Start = 0;

			while(1) //ID3가 여러개 있을수도 있음
			{ //ID3는 보통 맨 처음에 있음
				file->seek(tmp, abort_callback);
				file->read(temp, 3, abort_callback);
				if(temp[0] == 'I' && temp[1] == 'D' && temp[2] == '3')
				{
					int tmp;
					file->read(temp, 7, abort_callback);
					//ID3 Tag
					tmp = temp[6]; //Decode Tag Size
					tmp |= temp[5] << 7;
					tmp |= temp[4] << 14;
					tmp |= temp[3] << 21;
					tmp += 10; //Header Size
					Start += tmp;
				}
				else
					break;
				tmp ++;
			}
			file->seek(Start, abort_callback);
			for(;;Start ++)
			{
				BYTE temp;
				file->read_lendian_t(temp, abort_callback);
				if(temp == 0xFF) //MP3 Header까지
					break;
			}
		}
		else if(!StrCmpIA(fmt, "ogg"))
		{
			//처음 나오는 vorbis setup header 검색
			Start = 0;
			i = 0;
			CHAR SetupHeader[7] = {0x05, 0x76, 0x6F, 0x72, 0x62, 0x69, 0x73}; //Vorbis Setup Header
			CHAR BCV[3] = {'B', 'C', 'V'}; //codebook start?
			while(1)
			{
				file->seek(i, abort_callback);
				file->read(temp, 7, abort_callback);
				if(!memcmp(temp, SetupHeader, 7))
				{
					file->seek(i + 7 + 1, abort_callback);
					file->read(temp, 3, abort_callback);
					if(!memcmp(temp, BCV, 3)) //Setup Header와 BCV 사이에 뭔가 바이트가 하나 더 있다.
					{
						//여기부터다
						Start = i + 7 + 1 + 3;
						break;
					}
				}
				i ++;
				if(i > file->get_size(abort_callback))
					return false; //에러
			}

		}
		else if(!StrCmpIA(fmt, "wav") || !StrCmpIA(fmt, "flac") || !StrCmpIA(fmt, "ape")) //wav나 flac, ape. 죄다 시작부터
			Start = 0;
		else
		{
			//에러처리
			Start = false;
			return false;
		}
	}
	catch(...)
	{
		Start = 0;
		return false;
	}

	BYTE *buf = (BYTE *)malloc(0x28000);

	try
	{
		file->seek(Start, abort_callback);
		file->read(buf, min(0x28000, (size_t)file->get_size(abort_callback) - Start), abort_callback);
	}
	catch(...)
	{
		free(buf);
		return false;
	}

	md5(buf, min(0x28000, (size_t)file->get_size(abort_callback) - Start), MD5); //FileSize < 0x28000 일수도

	free(buf);

	CHAR HexArray[] = "0123456789abcdef";

	for(i = 0; i < 32; i += 2)
	{
		Hash[i] = HexArray[(MD5[i / 2] & 0xf0) >> 4];
		Hash[i + 1] = HexArray[MD5[i / 2] & 0x0f];
	}
	Hash[i] = 0;

	return true;
}

DWORD LyricManager::DownloadLyric(CHAR *Hash)
{//http://pugixml.googlecode.com/svn/trunk/docs/index.html
	CHAR GetLyricHashHeader[512] = "POST /alsongwebservice/service1.asmx HTTP/1.1\r\n"
		"Host: lyrics.alsong.co.kr\r\n"
		"User-Agent: gSOAP/2.7\r\n"
		"Content-Type: application/soap+xml; charset=utf-8\r\n"
		"Content-Length: %d\r\n"
		"Connection: close\r\n"
		"SOAPAction: \"ALSongWebServer/GetLyric5\"\r\n\r\n";
	CHAR GetLyricHashData1[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://www.w3.org/2003/05/soap-envelope\" xmlns:SOAP-ENC=\"http://www.w3.org/2003/05/soap-encoding\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:ns2=\"ALSongWebServer/Service1Soap\" xmlns:ns1=\"ALSongWebServer\" xmlns:ns3=\"ALSongWebServer/Service1Soap12\">"
		"<SOAP-ENV:Body><ns1:GetLyric5>"
		"<ns1:stQuery><ns1:strChecksum>";
	CHAR GetLyricHashData2[] = "</ns1:strChecksum><ns1:strVersion>";
	CHAR GetLyricHashData3[] = "</ns1:strVersion><ns1:strMACAddress>";
	CHAR GetLyricHashData4[] = "</ns1:strMACAddress><ns1:strIPAddress>";
	CHAR GetLyricHashData5[] = "</ns1:strIPAddress></ns1:stQuery></ns1:GetLyric5></SOAP-ENV:Body></SOAP-ENV:Envelope>";
	CHAR Version[] = "2.0";
	CHAR buf[256];
	struct hostent *host;
	CHAR Hostname[80];
	CHAR *Local_IP;
	CHAR Local_Mac[20];
	int i;
	int len;
	SOCKET s;
	CHAR *data = (CHAR *)malloc(sizeof(CHAR) * 600);
	int nAlloc = 600;
	int nUse = 0;
	int nRecv;

	//IP하고 MAC 찾기
	gethostname(Hostname, 80);
	host = gethostbyname(Hostname);

	struct in_addr addr;
	memcpy(&addr, host->h_addr_list[0], sizeof(struct in_addr));
	Local_IP = inet_ntoa(*((in_addr *)host->h_addr_list[0]));

	IP_ADAPTER_INFO AdapterInfo[16];
	DWORD dwBufLen = sizeof(AdapterInfo);

	GetAdaptersInfo(AdapterInfo, &dwBufLen);

	PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;

	while(pAdapterInfo) 
	{
		if(!lstrcmpA(pAdapterInfo->IpAddressList.IpAddress.String, Local_IP))
			break;
		pAdapterInfo = pAdapterInfo->Next;
	}
	if(pAdapterInfo == NULL)
		return false;

	CHAR HexArray[] = "0123456789ABCDEF";

	for(i = 0; i < 12; i += 2)
	{
		Local_Mac[i] = HexArray[(pAdapterInfo->Address[i / 2] & 0xf0) >> 4];
		Local_Mac[i + 1] = HexArray[pAdapterInfo->Address[i / 2] & 0x0f];
	}
	Local_Mac[i] = 0;

	s = InitateConnect("lyrics.alsong.co.kr", 80);
	if(s == 0)
	{
		free(data);
		return false;
	}

	len = lstrlenA(GetLyricHashData1) + lstrlenA(GetLyricHashData2) + lstrlenA(GetLyricHashData3) + lstrlenA(GetLyricHashData4) + lstrlenA(GetLyricHashData5) + lstrlenA(Hash) + lstrlenA(Version) + lstrlenA(Local_IP) + lstrlenA(Local_Mac);

	wsprintfA(buf, GetLyricHashHeader, len);

	send(s, buf, lstrlenA(buf), 0);
	send(s, GetLyricHashData1, lstrlenA(GetLyricHashData1), 0);
	send(s, Hash, lstrlenA(Hash), 0);
	send(s, GetLyricHashData2, lstrlenA(GetLyricHashData2), 0);
	send(s, Version, lstrlenA(Version), 0);
	send(s, GetLyricHashData3, lstrlenA(GetLyricHashData3), 0);
	send(s, Local_Mac, lstrlenA(Local_Mac), 0);
	send(s, GetLyricHashData4, lstrlenA(GetLyricHashData4), 0);
	send(s, Local_IP, lstrlenA(Local_IP), 0);
	send(s, GetLyricHashData5, lstrlenA(GetLyricHashData5), 0);

	while(nRecv = recv(s, buf, 255, 0))
	{
		if(SOCKET_ERROR == nRecv)
		{
			/*int t = WSAGetLastError();
			wsprintfA(buf, "Error receiving data. WSAGetLastError() = %d", t);
			MessageBoxA(NULL, buf, "Error", MB_OK);*/
			free(data);
			closesocket(s);
			return false;
		}
		buf[nRecv] = 0;
		CopyMemory(data + nUse, buf, nRecv + 1);
		nUse += nRecv;
		if(nUse + 255 > nAlloc - 100)
		{
			data = (CHAR *)realloc(data, nAlloc + 300);
			nAlloc += 300;
		}
		if(boost::this_thread::interruption_requested())
		{
			free(data);
			closesocket(s);
		}
	}

	//<?xml version="1.0" encoding="utf-8"?>
	//<soap:Envelope~~><soap:Body><GetLyric5Response xmlns="ALSongWebServer"><GetLyric5Result>
	//<strStatusID>2</strStatusID><strInfoID>-1</strInfoID><strRegistDate /><strTitle ~~
	pugi::xml_document doc;
	doc.load(boost::find_first(data, "\r\n\r\n").begin());
	pugi::xml_node xmlresult = doc.first_element_by_path("soap:Envelope/soap:Body/GetLyric5Response/GetLyric5Result");
	const char *result = xmlresult.child("strStatusID").child_value();
	if(buf[0] == '2')
	{
		free(data);
		closesocket(s);
		return false;
	}

	m_Title.assign(xmlresult.child("strTitle").child_value());
	m_Artist.assign(xmlresult.child("strArtist").child_value());
	m_Album.assign(xmlresult.child("strAlbum").child_value());
	m_Registrant.assign(xmlresult.child("strRegisterFirstName").child_value());

	if(!m_Album.compare(m_Title))
		m_Album.clear();

	ParseLyric(xmlresult.child("strLyric").child_value(), "<br>");

	free(data);
	closesocket(s);

	return S_OK;
}

void LyricManager::CountLyric(const metadb_handle_ptr &track)
{
	int microsec = (boost::posix_time::microsec_clock::universal_time() - tick).fractional_seconds() / 10000;	//sec:0, microsec:10
	int i;																										//0
	std::vector<lyricinfo>::iterator time_iterator;																//0   some song			
	for(m_Lyricpos = 0, time_iterator = m_Lyric.begin();														//0			         <-- m_LyricPos
		time_iterator != m_Lyric.end() && time_iterator->time < m_Seconds * 100 + microsec;						//100 blah			
		m_Lyricpos ++, time_iterator ++);
	if(time_iterator == m_Lyric.end())
		return;
	if(m_Lyricpos > 0)
	{
		m_Lyricpos --; //point to last visible line
		time_iterator --;
	}
	RedrawHandler();
	while(time_iterator != m_Lyric.end())
	{
		microsec = (boost::posix_time::microsec_clock::universal_time() - tick).fractional_seconds() / 10000;
		int lyricpos_temp = m_Lyricpos;
		while((int)time_iterator->time - (m_Seconds * 100 + microsec) < 0) lyricpos_temp ++, time_iterator ++;
		boost::this_thread::sleep(boost::posix_time::microseconds(time_iterator->time - (m_Seconds * 100 + microsec)) * 10000);
		if(boost::this_thread::interruption_requested())
			break;
		m_Lyricpos = lyricpos_temp;
		RedrawHandler();
	}

}

void LyricManager::Clear()
{
	m_Lyricpos = -1;
	m_Seconds = 0;
	m_Lyric.clear();
	m_Title.clear();
	m_Album.clear();
	m_Artist.clear();
	m_Registrant.clear();
}

DWORD LyricManager::FetchLyric(const metadb_handle_ptr &track)
{
	CHAR Hash[33];
	DWORD nRet;
	
	Clear();

	m_Lyricpos = 0;
	m_Lyric.push_back(lyricinfo(0, pfc::string8(pfc::stringcvt::string_utf8_from_wide(TEXT("파일 정보 처리중...")))));
	if(boost::this_thread::interruption_requested())
		return false;
	RedrawHandler();

	nRet = GetFileHash(track, Hash);
	m_Lyric[0] = lyricinfo(0, pfc::string8(pfc::stringcvt::string_utf8_from_wide(TEXT("가사 다운로드 중..."))));
	RedrawHandler();

	nRet = DownloadLyric(Hash);
	if(boost::this_thread::interruption_requested())
		return false;

	if(m_Lyric.size() == 0)
	{
		m_Lyric.push_back(lyricinfo(0, pfc::string8(pfc::stringcvt::string_utf8_from_wide(TEXT("실시간 가사를 찾을 수 없습니다.")))));
		RedrawHandler();
		return false;
	}

	m_countthread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&LyricManager::CountLyric, this, track)));

	return true;
}

DWORD LyricManager::ParseLyric(const char *InputLyric, const char *Delimiter)
{
	int i;

	m_Lyric.clear();

	const char *nowpos = InputLyric;
	const char *lastpos = nowpos;
	int pos;
	for(i = 0; ; i ++) //<br>자르기/ LineTotal은 안전빵
	{
		pos = pfc::strstr_ex(nowpos + 1, lstrlenA(nowpos + 1), Delimiter, lstrlenA(Delimiter));
		if(pos == -1)
			break;
		nowpos = nowpos + 1 + pos;
		
		int time = StrToIntA(lastpos + 1) * 60 * 100 + StrToIntA(lastpos + 4) * 100 + StrToIntA(lastpos + 7);
		lastpos += 10; //strlen("[34:56.78]");

		m_Lyric.push_back(lyricinfo(time, pfc::string8(lastpos, nowpos - lastpos)));
		lastpos = nowpos + lstrlenA(Delimiter);
	}

	return S_OK;
}

DWORD LyricManager::SearchLyricGetNext(CHAR **data, int &Info, pfc::string8 Title, pfc::string8 Artist, pfc::string8 Album, pfc::string8 Lyric, pfc::string8 Registrant)
{/*
	Title.set_string_nc(GET_XML_POS(*data, "strTitle"), GET_XML_LEN(*data, "strTitle"));
	Artist.set_string_nc(GET_XML_POS(*data, "strArtistName"), GET_XML_LEN(*data, "strArtistName"));
	Album.set_string_nc(GET_XML_POS(*data, "strAlbumName"), GET_XML_LEN(*data, "strAlbumName"));
	Registrant.set_string_nc(GET_XML_POS(*data, "strRegisterFirstName"), GET_XML_LEN(*data, "strRegisterFirstName"));
	string lyrictmp = string(GET_XML_POS(*data, "strLyric"), GET_XML_LEN(*data, "strLyric"));
	CHAR temp[255];
	GET_XML_DATA(*data, "strInfoID", temp);
	Info = StrToIntA(temp);
	*data = GET_XML_POS(*data, "ST_GET_RESEMBLELYRIC2_RETURN");
	RemoveHTMLEntities(Title);
	RemoveHTMLEntities(Artist);
	RemoveHTMLEntities(Album);
	RemoveHTMLEntities(Registrant);
	RemoveHTMLEntities(Lyric);

	while(lyrictmp.find("<br>") != string::npos)
		lyrictmp.replace(lyrictmp.find("<br>"), 4, "\r\n");
	Lyric = lyrictmp.c_str();
*/	
	return true;
}

DWORD LyricManager::SearchLyric(const pfc::string8 &Artist, const pfc::string8 Title, int nPage, CHAR **Output)
{
	CHAR GetLyricHeader[512] = "POST /alsongwebservice/service1.asmx HTTP/1.1\r\n"
		"Host: lyrics.alsong.co.kr\r\n"
		"User-Agent: gSOAP/2.7\r\n"
		"Content-Type: application/soap+xml; charset=utf-8\r\n"
		"Content-Length: %d\r\n"
		"Connection: close\r\n"
		"SOAPAction: \"ALSongWebServer/GetResembleLyric2\"\r\n\r\n";
	CHAR GetLyricData1[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://www.w3.org/2003/05/soap-envelope\" xmlns:SOAP-ENC=\"http://www.w3.org/2003/05/soap-encoding\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:ns2=\"ALSongWebServer/Service1Soap\" xmlns:ns1=\"ALSongWebServer\" xmlns:ns3=\"ALSongWebServer/Service1Soap12\">"
		"<SOAP-ENV:Body><ns1:GetResembleLyric2>"
		"<ns1:stQuery><ns1:strTitle>";
	CHAR GetLyricData2[] = "</ns1:strTitle><ns1:strArtistName>";
	CHAR GetLyricData3[] = "</ns1:strArtistName><ns1:nCurPage>";
	CHAR GetLyricData4[] = "</ns1:nCurPage></ns1:stQuery>"
		"</ns1:GetResembleLyric2></SOAP-ENV:Body></SOAP-ENV:Envelope>";

	SOCKET s;
	int len;
	CHAR buf[255];
	int nAlloc = 600;
	int nUse = 0;
	int nRecv;

	pfc::string8 ConvertedArtist = Artist, ConvertedTitle = Title;;

	*Output = (CHAR *)malloc(sizeof(CHAR) * 600);

	s = InitateConnect("lyrics.alsong.co.kr", 80);
	if(s == 0)
	{
		free(Output);
		return false;
	}
	len = lstrlenA(GetLyricData1) + lstrlenA(GetLyricData2) + lstrlenA(GetLyricData3) + lstrlenA(GetLyricData4)
		+ lstrlenA(ConvertedArtist.toString()) + lstrlenA(ConvertedTitle.toString());
	if(nPage != 0)
		len += (int)log10((float)nPage) + 1;
	else
		len += 1;
	wsprintfA(buf, GetLyricHeader, len);

	send(s, buf, lstrlenA(buf), 0);
	send(s, GetLyricData1, lstrlenA(GetLyricData1), 0);
	send(s, ConvertedTitle.toString(), lstrlenA(ConvertedTitle.toString()), 0);
	send(s, GetLyricData2, lstrlenA(GetLyricData2), 0);
	send(s, ConvertedArtist.toString(), lstrlenA(ConvertedArtist.toString()), 0);
	send(s, GetLyricData3, lstrlenA(GetLyricData3), 0);
	wsprintfA(buf, "%d", nPage);
	send(s, buf, lstrlenA(buf), 0);
	send(s, GetLyricData4, lstrlenA(GetLyricData4), 0);

	while(nRecv = recv(s, buf, 255, 0))
	{
		if(SOCKET_ERROR == nRecv)
		{/*
			int t = WSAGetLastError();
			wsprintfA(buf, "Error receiving data. WSAGetLastError() = %d", t);
			MessageBoxA(NULL, buf, "Error", MB_OK);*/
			free(Output);
			closesocket(s);
			return false;
		}
		CopyMemory(*Output + nUse, buf, nRecv);
		nUse += nRecv;
		if(nUse + 255 > nAlloc - 100)
		{
			*Output = (CHAR *)realloc(*Output, nAlloc + 300);
			nAlloc += 300;
		}
	}

	closesocket(s);

	return true;
}

int LyricManager::SearchLyricGetCount(const pfc::string8 &Artist, const pfc::string8 &Title)
{
	CHAR GetCountHeader[] = "POST /alsongwebservice/service1.asmx HTTP/1.1\r\n"
		"Host: lyrics.alsong.co.kr\r\n"
		"User-Agent: gSOAP/2.7\r\n"
		"Content-Type: application/soap+xml; charset=utf-8\r\n"
		"Content-Length: %d\r\n"
		"Connection: close\r\n"
		"SOAPAction: \"ALSongWebServer/GetResembleLyric2Count\"\r\n\r\n";

	CHAR GetCountData1[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://www.w3.org/2003/05/soap-envelope\" xmlns:SOAP-ENC=\"http://www.w3.org/2003/05/soap-encoding\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:ns2=\"ALSongWebServer/Service1Soap\" xmlns:ns1=\"ALSongWebServer\" xmlns:ns3=\"ALSongWebServer/Service1Soap12\">"
		"<SOAP-ENV:Body><ns1:GetResembleLyric2Count>"
		"<ns1:stQuery><ns1:strTitle>";
	CHAR GetCountData2[] = "</ns1:strTitle><ns1:strArtistName>";
	CHAR GetCountData3[] = "</ns1:strArtistName></ns1:stQuery>"
		"</ns1:GetResembleLyric2Count></SOAP-ENV:Body></SOAP-ENV:Envelope>";
	SOCKET s;
	int len, ret;
	CHAR buf[255];
	DWORD nRecv;

	s = InitateConnect("lyrics.alsong.co.kr", 80);
	if(s == 0)
		return 0;

	pfc::string8 ConvertedArtist = Artist, ConvertedTitle = Title;
/*

	ConvertToHTMLEntities(ConvertedArtist);
	ConvertToHTMLEntities(ConvertedTitle);

	len = lstrlenA(GetCountData1) + lstrlenA(GetCountData2) + lstrlenA(GetCountData3) + lstrlenA(ConvertedArtist.toString()) + lstrlenA(ConvertedTitle.toString());
	wsprintfA(buf, GetCountHeader, len);

	send(s, buf, lstrlenA(buf), 0);
	send(s, GetCountData1, lstrlenA(GetCountData1), 0);
	send(s, ConvertedTitle.toString(), lstrlenA(ConvertedTitle.toString()), 0);
	send(s, GetCountData2, lstrlenA(GetCountData2), 0);
	send(s, ConvertedArtist.toString(), lstrlenA(ConvertedArtist.toString()), 0);
	send(s, GetCountData3, lstrlenA(GetCountData3), 0);

	while(nRecv = recv(s, buf, 255, 0))
	{
		if(SOCKET_ERROR == nRecv)
		{/ *
			int t = WSAGetLastError();
			wsprintfA(buf, "Error receiving data. WSAGetLastError() = %d", t);
			MessageBoxA(NULL, buf, "Error", MB_OK);* /
			closesocket(s);
			return 0;
		}
		//<strResembleLyricCount>310
		CHAR *CountPtr;
		if(CountPtr = StrStrA(buf, "<strResembleLyricCount>"))
		{
			ret = StrToIntA(CountPtr + 23);
			break;
		}
	}

	closesocket(s);
*/

	return ret;
}

SOCKET LyricManager::InitateConnect(CHAR *Address, int port)
{
	SOCKET s;
	hostent *host = gethostbyname(Address);
	if(!host)
		return 0;
	in_addr *naddr;
	naddr = (in_addr *)host->h_addr;

	s = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in client_addr;
	ZeroMemory(&client_addr, sizeof(client_addr));
	client_addr.sin_addr = *naddr;
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(port);

	if(connect(s, (sockaddr *)(&client_addr), sizeof(sockaddr_in)) == SOCKET_ERROR)
		return NULL;

	return s;
}

DWORD LyricManager::LoadFromFile(WCHAR *LoadFrom, CHAR *fmt)
{
	Clear();
	if(!StrCmpIA(fmt, "lrc"))
	{
		HANDLE hFile = CreateFile(LoadFrom, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
		if(hFile == INVALID_HANDLE_VALUE)
			return FALSE;

		DWORD dwRead;
		CHAR *tmp;
		DWORD Size = SetFilePointer(hFile, 0, 0, FILE_END);
		SetFilePointer(hFile, 0, 0, FILE_BEGIN);
		if(Size == 0)
			return FALSE;

		tmp = new CHAR[Size + 10];

		ReadFile(hFile, tmp, Size, &dwRead, NULL);
		if(dwRead != Size)
			return FALSE;

		CloseHandle(hFile);

		DWORD ret = ParseLyric(tmp, "\r\n");

		delete[] tmp;

		return !ret;
	}

	return false;
}

DWORD LyricManager::UploadLyric(metadb_handle_ptr track, int PlayTime, int nInfo, int UploadType, 
								pfc::string8 Lyric, pfc::string8 Title, pfc::string8 Artist, pfc::string8 Album, pfc::string8 Registrant)
{
	//너무 많이 지역변수로 지정됨. 
	CHAR UploadLyricHeader[] =	"POST /alsongwebservice/service1.asmx HTTP/1.1\r\n"
								"Host: lyrics.alsong.co.kr\r\n"
								"User-Agent: gSOAP/2.7\r\n"
								"Content-Type: application/soap+xml; charset=utf-8\r\n"
								"Content-Length: %d\r\n"
								"Connection: close\r\n"
								"SOAPAction: \"ALSongWebServer/UploadLyric\"\r\n\r\n";
	CHAR strRegisterName[] = "Alsong Lyric Plugin for Foobar2000";//나머지는 생략

	//UploadLyricType - 1:Link 새거 2:Modify 수정 5:ReSetLink 아예 새거
	CHAR UploadLyricData[23][512] = {
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://www.w3.org/2003/05/soap-envelope\" xmlns:SOAP-ENC=\"http://www.w3.org/2003/05/soap-encoding\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:ns2=\"ALSongWebServer/Service1Soap\" xmlns:ns1=\"ALSongWebServer\" xmlns:ns3=\"ALSongWebServer/Service1Soap12\">"
		"<SOAP-ENV:Body><ns1:UploadLyric><ns1:stQuery><ns1:nUploadLyricType>",
		"</ns1:nUploadLyricType><ns1:strMD5>",
		"</ns1:strMD5><ns1:strRegisterFirstName>",
		"</ns1:strRegisterFirstName><ns1:strRegisterFirstEMail>",
		"</ns1:strRegisterFirstEMail><ns1:strRegisterFirstURL>",
		"</ns1:strRegisterFirstURL><ns1:strRegisterFirstPhone>",
		"</ns1:strRegisterFirstPhone><ns1:strRegisterFirstComment>",
		"</ns1:strRegisterFirstComment><ns1:strRegisterName>",
		"</ns1:strRegisterName><ns1:strRegisterEMail>",
		"</ns1:strRegisterEMail><ns1:strRegisterURL>",
		"</ns1:strRegisterURL><ns1:strRegisterPhone>",
		"</ns1:strRegisterPhone><ns1:strRegisterComment>",
		"</ns1:strRegisterComment><ns1:strFileName>",
		"</ns1:strFileName><ns1:strTitle>",
		"</ns1:strTitle><ns1:strArtist>",
		"</ns1:strArtist><ns1:strAlbum>",
		"</ns1:strAlbum><ns1:nInfoID>",
		"</ns1:nInfoID><ns1:strLyric>",
		"</ns1:strLyric><ns1:nPlayTime>",
		"</ns1:nPlayTime><ns1:strVersion>",
		"</ns1:strVersion><ns1:strMACAddress>",
		"</ns1:strMACAddress><ns1:strIPAddress>",
		"</ns1:strIPAddress></ns1:stQuery></ns1:UploadLyric></SOAP-ENV:Body></SOAP-ENV:Envelope>"
	};
	CHAR Version[] = "2.0";
	CHAR buf[255];
	struct hostent *host;
	CHAR Hostname[80];
	CHAR *Local_IP;
	CHAR Local_Mac[20];
	int i;
	int len = 0;
	SOCKET s;
	CHAR *data;
	int nAlloc = 600;
	int nUse = 0;
	int nRecv;
	CHAR Hash[255];
	pfc::string8 Filename = track->get_path();

	//IP하고 MAC 찾기
	gethostname(Hostname, 80);
	host = gethostbyname(Hostname);

	struct in_addr addr;
	memcpy(&addr, host->h_addr_list[0], sizeof(struct in_addr));
	Local_IP = inet_ntoa(*((in_addr *)host->h_addr_list[0]));

	IP_ADAPTER_INFO AdapterInfo[16];
	DWORD dwBufLen = sizeof(AdapterInfo);

	GetAdaptersInfo(AdapterInfo, &dwBufLen);

	PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;

	while(pAdapterInfo) 
	{
		if(!lstrcmpA(pAdapterInfo->IpAddressList.IpAddress.String, Local_IP))
			break;
		pAdapterInfo = pAdapterInfo->Next;
	}
	if(pAdapterInfo == NULL)
		return false;

	CHAR HexArray[] = "0123456789ABCDEF";

	for(i = 0; i < 12; i += 2)
	{
		Local_Mac[i] = HexArray[(pAdapterInfo->Address[i / 2] & 0xf0) >> 4];
		Local_Mac[i + 1] = HexArray[pAdapterInfo->Address[i / 2] & 0x0f];
	}
	Local_Mac[i] = 0;

	s = InitateConnect("lyrics.alsong.co.kr", 80);
	if(s == 0)
	{
		return false;
	}/*
	
	
		ConvertToHTMLEntities(Filename);
	
		GetFileHash(track, Hash);
		
		ConvertToHTMLEntities(Lyric);
		ConvertToHTMLEntities(Artist);
		ConvertToHTMLEntities(Album);
		ConvertToHTMLEntities(Title);
		ConvertToHTMLEntities(Registrant);	
	
		for(i = 0; i < 23; i ++)
			len += lstrlenA(UploadLyricData[i]);
		len += lstrlenA(strRegisterName);
		len += (int)log10((float)nInfo) + 1;
		len += Lyric.length() + Artist.length() + Registrant.length() + Album.length() + Title.length();
		len += 1;
		len += lstrlenA(Hash);
		len += Filename.length();
		len += (int)log10((float)PlayTime) + 1;
		len += lstrlenA(Local_IP) + lstrlenA(Local_Mac) + lstrlenA(Version);
	
		wsprintfA(buf, UploadLyricHeader, len);
	
		send(s, buf, lstrlenA(buf), 0);
		send(s, UploadLyricData[0], lstrlenA(UploadLyricData[0]), 0);
		wsprintfA(buf, "%d", UploadType);
		send(s, buf, lstrlenA(buf), 0);
		send(s, UploadLyricData[1], lstrlenA(UploadLyricData[1]), 0);
		send(s, Hash, lstrlenA(Hash), 0);
		send(s, UploadLyricData[2], lstrlenA(UploadLyricData[2]), 0);
		send(s, Registrant.toString(), Registrant.length(), 0);
		send(s, UploadLyricData[3], lstrlenA(UploadLyricData[3]), 0);
		send(s, UploadLyricData[4], lstrlenA(UploadLyricData[4]), 0);
		send(s, UploadLyricData[5], lstrlenA(UploadLyricData[5]), 0);
		send(s, UploadLyricData[6], lstrlenA(UploadLyricData[6]), 0);
		send(s, UploadLyricData[7], lstrlenA(UploadLyricData[7]), 0);
		send(s, strRegisterName, lstrlenA(strRegisterName), 0);
		send(s, UploadLyricData[8], lstrlenA(UploadLyricData[8]), 0);
		send(s, UploadLyricData[9], lstrlenA(UploadLyricData[9]), 0);
		send(s, UploadLyricData[10], lstrlenA(UploadLyricData[10]), 0);
		send(s, UploadLyricData[11], lstrlenA(UploadLyricData[11]), 0);
		send(s, UploadLyricData[12], lstrlenA(UploadLyricData[12]), 0);
		send(s, Filename.get_ptr(), Filename.length(), 0);
		send(s, UploadLyricData[13], lstrlenA(UploadLyricData[13]), 0);
		send(s, Title.toString(), Title.length(), 0);
		send(s, UploadLyricData[14], lstrlenA(UploadLyricData[14]), 0);
		send(s, Artist.toString(), Artist.length(), 0);
		send(s, UploadLyricData[15], lstrlenA(UploadLyricData[15]), 0);
		send(s, Album.toString(), Album.length(), 0);
		send(s, UploadLyricData[16], lstrlenA(UploadLyricData[16]), 0);
		wsprintfA(buf, "%d", nInfo);
		send(s, buf, lstrlenA(buf), 0);
		send(s, UploadLyricData[17], lstrlenA(UploadLyricData[17]), 0);
		send(s, Lyric.toString(), Lyric.length(), 0);
		send(s, UploadLyricData[18], lstrlenA(UploadLyricData[18]), 0);
		wsprintfA(buf, "%d", PlayTime);
		send(s, buf, lstrlenA(buf), 0);
		send(s, UploadLyricData[19], lstrlenA(UploadLyricData[19]), 0);
		send(s, Version, lstrlenA(Version), 0);
		send(s, UploadLyricData[20], lstrlenA(UploadLyricData[20]), 0);
		send(s, Local_Mac, lstrlenA(Local_Mac), 0);
		send(s, UploadLyricData[21], lstrlenA(UploadLyricData[21]), 0);
		send(s, Local_IP, lstrlenA(Local_IP), 0);
		send(s, UploadLyricData[22], lstrlenA(UploadLyricData[22]), 0);
	
		data = (CHAR *)malloc(sizeof(CHAR) * 600);
	
		while(nRecv = recv(s, buf, 255, 0))
		{
			if(SOCKET_ERROR == nRecv)
			{
				/ *int t = WSAGetLastError();
				wsprintfA(buf, "Error receiving data. WSAGetLastError() = %d", t);
				MessageBoxA(NULL, buf, "Error", MB_OK);* /
				free(data);
				closesocket(s);
				return false;
			}
			CopyMemory(data + nUse, buf, nRecv);
			nUse += nRecv;
			if(nUse + 255 > nAlloc - 100)
			{
				data = (CHAR *)realloc(data, nAlloc + 300);
				nAlloc += 300;
			}
		}
		
		
		if(!StrStrA(data, "UploadLyricResult"))
		{
			//실패
			free(data);
			return false;
		}
	
		free(data);
		
	*/
		return true;
}

void LyricManager::SaveToFile(WCHAR *SaveTo, CHAR *fmt)
{
	if(m_Lyric.size() == 0)
		return;
	if(!StrCmpIA(fmt, "lrc"))
	{
		HANDLE hFile = CreateFile(SaveTo, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, NULL, NULL);
		if(hFile == INVALID_HANDLE_VALUE)
			return;
		unsigned int i;
		DWORD dwWritten;

		for(i = 0; i < m_Lyric.size(); i ++)
		{
			CHAR temp[255];
			wsprintfA(temp, "[%02d:%02d.%02d]", (int)(m_Lyric[i].time / 100) / 60, ((int)m_Lyric[i].time / 100) % 60, (int)(m_Lyric[i].time % 100));
			WriteFile(hFile, (void *)temp, strlen(temp), &dwWritten, NULL);
			WriteFile(hFile, m_Lyric[i].lyric.toString(), m_Lyric[i].lyric.length(), &dwWritten, NULL);
			WriteFile(hFile, "\r\n", 2, &dwWritten, NULL);
		}

		CloseHandle(hFile);
	}
	else if(!StrCmpIA(fmt, "txt"))
	{
		HANDLE hFile = CreateFile(SaveTo, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, NULL, NULL);
		unsigned int i;
		DWORD dwWritten;

		for(i = 0; i < m_Lyric.size(); i ++)
		{
			WriteFile(hFile, m_Lyric[i].lyric.toString(), m_Lyric[i].lyric.length(), &dwWritten, NULL);
			WriteFile(hFile, "\r\n", 2, &dwWritten, NULL);
		}

		CloseHandle(hFile);
	}
}
