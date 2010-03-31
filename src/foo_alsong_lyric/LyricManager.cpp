#include "stdafx.h"
#include "ConfigStore.h"
#include "AlsongUI.h"
#include "LyricManager.h"
#include "pugixml/pugixml.hpp"
#include "md5.h"
#include "resource.h"
//TODO: USLT 태그(4바이트 타임스탬프, 1바이트 길이, 문자열(유니코드), 0x08 순으로 들어있음)

#define ALSONG_VERSION "2.11"

LyricManager *LyricManagerInstance = NULL;

LyricManager::LyricManager() : m_Lyricpos(-1), m_Seconds(0), m_haslyric(0)
{
	static_api_ptr_t<play_callback_manager> pcm;
	pcm->register_callback(this, flag_on_playback_all, false);
}

LyricManager::~LyricManager()
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
	static_api_ptr_t<play_callback_manager> pcm;
	pcm->unregister_callback(this);
}

void LyricManager::on_playback_seek(double p_time)
{
	if(m_countthread)
	{
		m_countthread->interrupt();
		m_countthread->join();
		m_countthread.reset();
	}
	m_Seconds = (int)p_time;
	tick = boost::posix_time::microsec_clock::universal_time() - boost::posix_time::microseconds((int)(p_time - m_Seconds) * 1000000);
	m_countthread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&LyricManager::CountLyric, this)));
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
	if(m_countthread)
	{
		m_countthread->interrupt();
		m_countthread->join();
		m_countthread.reset();
	}
}

void LyricManager::on_playback_time(double p_time)
{
	tick = boost::posix_time::microsec_clock::universal_time();
	m_Seconds = (int)p_time;
}

void LyricManager::on_playback_pause(bool p_state)
{
	if(p_state == true && m_countthread)
	{
		m_countthread->interrupt();
		m_countthread->join();
		m_countthread.reset();
	}
	else if(p_state == false && m_haslyric == 1)
		m_countthread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&LyricManager::CountLyric, this)));
}

std::vector<std::string> LyricManager::GetLyricBefore(int n)
{
	if(m_Lyricpos >= 0)
	{
		std::vector<std::string> ret;
		for(int i = max(0, m_Lyricpos - n); i < m_Lyricpos; i ++)
			ret.push_back(m_Lyric[i].lyric);

		return ret;
	}
	return std::vector<std::string>();
}

std::vector<std::string> LyricManager::GetLyric()
{
	if(m_Lyricpos >= 0)
	{
		std::vector<std::string> ret;
		for(unsigned int i = m_Lyricpos; i < m_Lyric.size() && m_Lyric[i].time == m_Lyric[m_Lyricpos].time; i ++)
			ret.push_back(m_Lyric[i].lyric);
		return ret;
	}
	return std::vector<std::string>();
}

std::vector<std::string> LyricManager::GetLyricAfter(int n)
{
	if(m_Lyricpos >= 0)
	{
		std::vector<std::string> ret;
		for(unsigned int i = m_Lyricpos + 1; i < min(m_Lyric.size(), (unsigned int)m_Lyricpos + n + 1); i ++)
			ret.push_back(m_Lyric[i].lyric);

		return ret;
	}
	return std::vector<std::string>();
}

DWORD LyricManager::GetFileHash(metadb_handle_ptr track, CHAR *Hash)
{	
	int i;
	DWORD Start = 0; //Start Address
	BYTE MD5[16];
	BYTE temp[255]; 

	service_ptr_t<file> file;
	abort_callback_impl abort_callback;
	pfc::string8 str = track->get_path();
	
	archive_impl::g_open(file, str, foobar2000_io::filesystem::open_mode_read, abort_callback);
	//TODO:cue일때 특별 처리(subsong_index가 있을 때)
	char *fmt = (char *)str.get_ptr() + str.find_last('.') + 1;
	/*
	file_info_impl info;
	track->get_info(info);
	const char *ttmp = info.info_get("referenced_offset");*/
	//아래코드 이용해서 cue에서 raw 뽑아와서 hash생성
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
		{
			while(1) //ID3가 여러개 있을수도 있음
			{ //ID3는 보통 맨 처음에 있음
				file->seek(Start, abort_callback);
				file->read(temp, 3, abort_callback);
				if(temp[0] == 'I' && temp[1] == 'D' && temp[2] == '3')
				{
					file->read(temp, 7, abort_callback);
#define ID3_TAGSIZE(x) ((*(x) << 21) | (*((x) + 1) << 14) | (*((x) + 2) << 7) | *((x) + 3))
					Start += ID3_TAGSIZE(temp + 3) + 10;
#undef ID3_TAGSIZE
				}
				else
					break;
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
			return false;
	}
	catch(...)
	{
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
	CHAR GetLyricHashHeader[] = "POST /alsongwebservice/service1.asmx HTTP/1.1\r\n"
		"Host: lyrics.alsong.co.kr\r\n"
		"User-Agent: gSOAP/2.7\r\n"
		"Content-Type: application/soap+xml; charset=utf-8\r\n"
		"Content-Length: %d\r\n"
		"Connection: close\r\n"
		"SOAPAction: \"ALSongWebServer/GetLyric5\"\r\n\r\n";

	CHAR buf[256];
	struct hostent *host;
	CHAR Hostname[80];
	CHAR *Local_IP;
	CHAR Local_Mac[20];
	int i;
	SOCKET s;
	std::vector<char> data;
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

	pugi::xml_document xmldoc;
	pugi::xml_node envelope = xmldoc.append_child();
	envelope.set_name("SOAP-ENV:Envelope");
	envelope.append_attribute("xmlns:SOAP-ENV").set_value("http://www.w3.org/2003/05/soap-envelope");
	envelope.append_attribute("xmlns:SOAP-ENC").set_value("http://www.w3.org/2003/05/soap-encoding");
	envelope.append_attribute("xmlns:xsi").set_value("http://www.w3.org/2001/XMLSchema-instance");
	envelope.append_attribute("xmlns:xsd").set_value("http://www.w3.org/2001/XMLSchema");
	envelope.append_attribute("xmlns:ns2").set_value("ALSongWebServer/Service1Soap");
	envelope.append_attribute("xmlns:ns1").set_value("ALSongWebServer");
	envelope.append_attribute("xmlns:ns3").set_value("ALSongWebServer/Service1Soap12");
	envelope.append_child().set_name("SOAP-ENV:Body");
	pugi::xml_node getlyric = envelope.child("SOAP-ENV:Body").append_child();
	getlyric.set_name("ns1:GetLyric5");
	pugi::xml_node query = getlyric.append_child();
	query.set_name("ns1:stQuery");
	pugi::xml_node checksum = query.append_child();
	checksum.set_name("ns1:strChecksum");
	checksum.append_child(pugi::node_pcdata).set_value(Hash);
	pugi::xml_node version = query.append_child();
	version.set_name("ns1:strVersion");
	version.append_child(pugi::node_pcdata).set_value(ALSONG_VERSION);
	pugi::xml_node macaddr = query.append_child();
	macaddr.set_name("ns1:strMACAddress");
	macaddr.append_child(pugi::node_pcdata).set_value(Local_Mac);
	pugi::xml_node ip = query.append_child();
	ip.set_name("ns1:strIPAddress");
	ip.append_child(pugi::node_pcdata).set_value(Local_IP);
	std::stringstream str;
	pugi::xml_writer_stream writer(str);
	xmldoc.save(writer, "", pugi::format_raw);

	wsprintfA(buf, GetLyricHashHeader, str.str().length());

	send(s, buf, lstrlenA(buf), 0);
	send(s, str.str().c_str(), str.str().length(), 0);

	while(nRecv = recv(s, buf, 255, 0))
	{
		if(SOCKET_ERROR == nRecv)
		{
			/*int t = WSAGetLastError();
			wsprintfA(buf, "Error receiving data. WSAGetLastError() = %d", t);
			MessageBoxA(NULL, buf, "Error", MB_OK);*/
			closesocket(s);
			return false;
		}
		buf[nRecv] = 0;
		data.insert(data.end(), buf, buf + nRecv);
		if(boost::this_thread::interruption_requested())
		{
			closesocket(s);
			return false;
		}
	}

	pugi::xml_document doc;
	doc.load(&*boost::find_first(data, "\r\n\r\n").begin());
	pugi::xml_node xmlresult = doc.first_element_by_path("soap:Envelope/soap:Body/GetLyric5Response/GetLyric5Result");
	m_Title.assign(xmlresult.child("strTitle").child_value());
	m_Artist.assign(xmlresult.child("strArtist").child_value());
	m_Album.assign(xmlresult.child("strAlbum").child_value());
	m_Registrant.assign(xmlresult.child("strRegisterFirstName").child_value());

	if(!m_Album.compare(m_Title))
		m_Album.clear();

	ParseLyric(xmlresult.child("strLyric").child_value(), "<br>");
	closesocket(s);

	return S_OK;
}

void LyricManager::CountLyric()
{
	long long microsec = (boost::posix_time::microsec_clock::universal_time() - tick).fractional_seconds() / 10000;	//sec:0, microsec:10
																												//0					<-- m_LyricPos
	std::vector<lyricinfo>::iterator time_iterator;																//0   some song
	for(m_Lyricpos = 0, time_iterator = m_Lyric.begin();														//0
		time_iterator != m_Lyric.end() && time_iterator->time < m_Seconds * 100 + microsec;						//100 blah			
		m_Lyricpos ++, time_iterator ++);
	if(time_iterator == m_Lyric.end())
		return;
	if(m_Lyricpos > 0)
	{
		m_Lyricpos --, time_iterator --;
		while(time_iterator != m_Lyric.begin() && time_iterator->time == 0) m_Lyricpos --, time_iterator --;//point to last visible line
		while(time_iterator != m_Lyric.begin() && (time_iterator - 1)->time == time_iterator->time) time_iterator --, m_Lyricpos --;
	}
	RedrawHandler();
	try
	{
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
	catch(...)
	{ //ignore exception
	}
}

void LyricManager::Clear()
{
	m_Lyricpos = -1;
	m_Seconds = 0;
	m_haslyric = 0;
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
	m_Lyric.push_back(lyricinfo(0, std::string(pfc::stringcvt::string_utf8_from_wide(TEXT("파일 정보 처리중...")))));
	if(boost::this_thread::interruption_requested())
		return false;
	RedrawHandler();

	nRet = GetFileHash(track, Hash);
	m_Lyric[0] = lyricinfo(0, std::string(pfc::stringcvt::string_utf8_from_wide(TEXT("가사 다운로드 중..."))));
	RedrawHandler();

	nRet = DownloadLyric(Hash);
	if(boost::this_thread::interruption_requested())
		return false;

	if(m_Lyric.size() == 0)
	{
		m_Lyric.push_back(lyricinfo(0, std::string(pfc::stringcvt::string_utf8_from_wide(TEXT("실시간 가사를 찾을 수 없습니다.")))));
		RedrawHandler();
		return false;
	}

	m_countthread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&LyricManager::CountLyric, this)));

	//cleanup
	m_fetchthread->detach();
	m_fetchthread.reset();
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
		//lastpos += 10; //strlen("[34:56.78]");

		m_Lyric.push_back(lyricinfo(time, std::string(lastpos, nowpos - lastpos)));
		lastpos = nowpos + lstrlenA(Delimiter);
	}
	if(i)
		m_haslyric = 1;
	return S_OK;
}

DWORD LyricManager::SearchLyric(const std::string &Artist, const std::string Title, int nPage, LyricSearchResult &data)
{
	CHAR GetLyricHeader[] = "POST /alsongwebservice/service1.asmx HTTP/1.1\r\n"
		"Host: lyrics.alsong.co.kr\r\n"
		"User-Agent: gSOAP/2.7\r\n"
		"Content-Type: application/soap+xml; charset=utf-8\r\n"
		"Content-Length: %d\r\n"
		"Connection: close\r\n"
		"SOAPAction: \"ALSongWebServer/GetResembleLyric2\"\r\n\r\n";
	
	SOCKET s;
	int nRecv;
	CHAR buf[256];
	data.data.clear();

	pugi::xml_document xmldoc;
	pugi::xml_node envelope = xmldoc.append_child();
	envelope.set_name("SOAP-ENV:Envelope");
	envelope.append_attribute("xmlns:SOAP-ENV").set_value("http://www.w3.org/2003/05/soap-envelope");
	envelope.append_attribute("xmlns:SOAP-ENC").set_value("http://www.w3.org/2003/05/soap-encoding");
	envelope.append_attribute("xmlns:xsi").set_value("http://www.w3.org/2001/XMLSchema-instance");
	envelope.append_attribute("xmlns:xsd").set_value("http://www.w3.org/2001/XMLSchema");
	envelope.append_attribute("xmlns:ns2").set_value("ALSongWebServer/Service1Soap");
	envelope.append_attribute("xmlns:ns1").set_value("ALSongWebServer");
	envelope.append_attribute("xmlns:ns3").set_value("ALSongWebServer/Service1Soap12");
	envelope.append_child().set_name("SOAP-ENV:Body");
	pugi::xml_node searchlyric = envelope.child("SOAP-ENV:Body").append_child();
	searchlyric.set_name("ns1:GetResembleLyric2");
	pugi::xml_node query = searchlyric.append_child();
	query.set_name("ns1:stQuery");
	pugi::xml_node title = query.append_child();
	title.set_name("ns1:strTitle");
	title.append_child(pugi::node_pcdata).set_value(Title.c_str());
	pugi::xml_node artist = query.append_child();
	artist.set_name("ns1:strArtistName");
	artist.append_child(pugi::node_pcdata).set_value(Artist.c_str());
	pugi::xml_node page = query.append_child();
	page.set_name("ns1:nCurPage");
	page.append_child(pugi::node_pcdata).set_value(boost::lexical_cast<std::string>(nPage).c_str());
	std::stringstream str;
	pugi::xml_writer_stream writer(str);
	xmldoc.save(writer, "", pugi::format_raw);

	s = InitateConnect("lyrics.alsong.co.kr", 80);
	if(s == 0)
		return false;
	
	wsprintfA(buf, GetLyricHeader, str.str().length());

	send(s, buf, lstrlenA(buf), 0);
	send(s, str.str().c_str(), str.str().length(), 0);

	while(nRecv = recv(s, buf, 255, 0))
	{
		if(SOCKET_ERROR == nRecv)
		{/*
			int t = WSAGetLastError();
			wsprintfA(buf, "Error receiving data. WSAGetLastError() = %d", t);
			MessageBoxA(NULL, buf, "Error", MB_OK);*/
			closesocket(s);
			return false;
		}
		buf[nRecv] = 0;
		data.data.insert(data.data.end(), buf, buf + nRecv);
	}

	data.doc.load(&*boost::find_first(data.data, "\r\n\r\n").begin());
	data.node = data.doc.first_element_by_path("soap:Envelope/soap:Body/GetResembleLyric2Response/GetResembleLyric2Result/ST_GET_RESEMBLELYRIC2_RETURN"); //TODO: Test

	closesocket(s);

	return true;
}

int LyricManager::SearchLyricGetCount(const std::string &Artist, const std::string &Title)
{
	CHAR GetCountHeader[] = "POST /alsongwebservice/service1.asmx HTTP/1.1\r\n"
		"Host: lyrics.alsong.co.kr\r\n"
		"User-Agent: gSOAP/2.7\r\n"
		"Content-Type: application/soap+xml; charset=utf-8\r\n"
		"Content-Length: %d\r\n"
		"Connection: close\r\n"
		"SOAPAction: \"ALSongWebServer/GetResembleLyric2Count\"\r\n\r\n";

	SOCKET s;
	CHAR buf[256];
	DWORD nRecv;
	std::vector<char> data;

	s = InitateConnect("lyrics.alsong.co.kr", 80);
	if(s == NULL)
		return false;

	pugi::xml_document xmldoc;
	pugi::xml_node envelope = xmldoc.append_child();
	envelope.set_name("SOAP-ENV:Envelope");
	envelope.append_attribute("xmlns:SOAP-ENV").set_value("http://www.w3.org/2003/05/soap-envelope");
	envelope.append_attribute("xmlns:SOAP-ENC").set_value("http://www.w3.org/2003/05/soap-encoding");
	envelope.append_attribute("xmlns:xsi").set_value("http://www.w3.org/2001/XMLSchema-instance");
	envelope.append_attribute("xmlns:xsd").set_value("http://www.w3.org/2001/XMLSchema");
	envelope.append_attribute("xmlns:ns2").set_value("ALSongWebServer/Service1Soap");
	envelope.append_attribute("xmlns:ns1").set_value("ALSongWebServer");
	envelope.append_attribute("xmlns:ns3").set_value("ALSongWebServer/Service1Soap12");
	envelope.append_child().set_name("SOAP-ENV:Body");
	pugi::xml_node getlyriccount = envelope.child("SOAP-ENV:Body").append_child();
	getlyriccount.set_name("ns1:GetResembleLyric2Count");
	pugi::xml_node query = getlyriccount.append_child();
	query.set_name("ns1:stQuery");
	pugi::xml_node title = query.append_child();
	title.set_name("ns1:strTitle");
	title.append_child(pugi::node_pcdata).set_value(Title.c_str());
	pugi::xml_node artist = query.append_child();
	artist.set_name("ns1:strArtistName");
	artist.append_child(pugi::node_pcdata).set_value(Artist.c_str());
	std::stringstream str;
	pugi::xml_writer_stream writer(str);
	xmldoc.save(writer, "", pugi::format_raw);

	wsprintfA(buf, GetCountHeader, str.str().length());

	send(s, buf, lstrlenA(buf), 0);
	send(s, str.str().c_str(), str.str().length(), 0);

	while(nRecv = recv(s, buf, 255, 0))
	{
		if(SOCKET_ERROR == nRecv)
		{/*
			int t = WSAGetLastError();
			wsprintfA(buf, "Error receiving data. WSAGetLastError() = %d", t);
			MessageBoxA(NULL, buf, "Error", MB_OK);*/
			closesocket(s);
			return false;
		}
		//<strResembleLyricCount>310
		buf[nRecv] = 0;
		data.insert(data.end(), buf, buf + nRecv);
	}

	pugi::xml_document doc;
	doc.load(&*boost::find_first(data, "\r\n\r\n").begin());
	pugi::xml_node xmlresult = doc.first_element_by_path("soap:Envelope/soap:Body/GetResembleLyric2CountResponse/GetResembleLyric2CountResult/strResembleLyricCount"); //TODO: Test
	
	closesocket(s);

	return boost::lexical_cast<int>(xmlresult.child_value());
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

DWORD LyricManager::UploadLyric(metadb_handle_ptr track, int PlayTime, int UploadType, const LyricResult &Lyric)
{
	CHAR UploadLyricHeader[] =	"POST /alsongwebservice/service1.asmx HTTP/1.1\r\n"
								"Host: lyrics.alsong.co.kr\r\n"
								"User-Agent: gSOAP/2.7\r\n"
								"Content-Type: application/soap+xml; charset=utf-8\r\n"
								"Content-Length: %d\r\n"
								"Connection: close\r\n"
								"SOAPAction: \"ALSongWebServer/UploadLyric\"\r\n\r\n";
	CHAR strRegisterName[] = "Alsong Lyric Plugin for Foobar2000";//나머지는 생략

	//UploadLyricType - 1:Link 새거 2:Modify 수정 5:ReSetLink 아예 새거

	CHAR buf[256];
	struct hostent *host;
	CHAR Hostname[80];
	CHAR *Local_IP;
	CHAR Local_Mac[20];
	int i;
	SOCKET s;
	int nRecv;
	CHAR Hash[255];
	std::string Filename = track->get_path();
	std::vector<char> data;

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

	GetFileHash(track, Hash);

	s = InitateConnect("lyrics.alsong.co.kr", 80);
	if(s == 0)
		return false;	
		
	pugi::xml_document xmldoc;
	pugi::xml_node envelope = xmldoc.append_child();
	envelope.set_name("SOAP-ENV:Envelope");
	envelope.append_attribute("xmlns:SOAP-ENV").set_value("http://www.w3.org/2003/05/soap-envelope");
	envelope.append_attribute("xmlns:SOAP-ENC").set_value("http://www.w3.org/2003/05/soap-encoding");
	envelope.append_attribute("xmlns:xsi").set_value("http://www.w3.org/2001/XMLSchema-instance");
	envelope.append_attribute("xmlns:xsd").set_value("http://www.w3.org/2001/XMLSchema");
	envelope.append_attribute("xmlns:ns2").set_value("ALSongWebServer/Service1Soap");
	envelope.append_attribute("xmlns:ns1").set_value("ALSongWebServer");
	envelope.append_attribute("xmlns:ns3").set_value("ALSongWebServer/Service1Soap12");
	envelope.append_child().set_name("SOAP-ENV:Body");
	pugi::xml_node uploadlyric = envelope.child("SOAP-ENV:Body").append_child();
	uploadlyric.set_name("ns1:UploadLyric");
	pugi::xml_node query = uploadlyric.append_child();
	query.set_name("ns1:stQuery");

	pugi::xml_node uploadlyrictype = query.append_child();
	uploadlyrictype.set_name("ns1:nUploadLyricType");
	uploadlyrictype.append_child(pugi::node_pcdata).set_value(boost::lexical_cast<char *>(UploadType));

	pugi::xml_node md5 = query.append_child();
	md5.set_name("ns1:strMD5");
	md5.append_child(pugi::node_pcdata).set_value(Hash);

	pugi::xml_node strRegisterFirstName = query.append_child();
	strRegisterFirstName.set_name("ns1:strRegisterFirstName");
	strRegisterFirstName.append_child(pugi::node_pcdata).set_value(Lyric.Registrant.c_str());

	pugi::xml_node strRegisterFirstEMail = query.append_child();
	strRegisterFirstEMail.set_name("ns1:strRegisterFirstEMail");

	pugi::xml_node strRegisterFirstURL = query.append_child();
	strRegisterFirstURL.set_name("ns1:strRegisterFirstURL");

	pugi::xml_node strRegisterFirstPhone = query.append_child();
	strRegisterFirstPhone.set_name("ns1:strRegisterFirstPhone");

	pugi::xml_node strRegisterFirstComment = query.append_child();
	strRegisterFirstComment.set_name("ns1:strRegisterFirstComment");

	pugi::xml_node registername = query.append_child();
	registername.set_name("ns1:strRegisterName");
	registername.append_child(pugi::node_pcdata).set_value(strRegisterName);

	pugi::xml_node strRegisterEMail = query.append_child();
	strRegisterEMail.set_name("ns1:strRegisterEMail");

	pugi::xml_node strRegisterURL = query.append_child();
	strRegisterURL.set_name("ns1:strRegisterURL");

	pugi::xml_node strRegisterPhone = query.append_child();
	strRegisterPhone.set_name("ns1:strRegisterPhone");

	pugi::xml_node strRegisterComment = query.append_child();
	strRegisterComment.set_name("ns1:strRegisterComment");

	pugi::xml_node strFileName = query.append_child();
	strFileName.set_name("ns1:strFileName");
	strFileName.append_child(pugi::node_pcdata).set_value(Filename.c_str());

	pugi::xml_node title = query.append_child();
	title.set_name("ns1:strTitle");
	title.append_child(pugi::node_pcdata).set_value(Lyric.Title.c_str());

	pugi::xml_node artist = query.append_child();
	artist.set_name("ns1:strArtist");
	artist.append_child(pugi::node_pcdata).set_value(Lyric.Artist.c_str());

	pugi::xml_node strAlbum = query.append_child();
	strAlbum.set_name("ns1:strAlbum");
	strAlbum.append_child(pugi::node_pcdata).set_value(Lyric.Album.c_str());

	pugi::xml_node nInfoID = query.append_child();
	nInfoID.set_name("ns1:nInfoID");
	nInfoID.append_child(pugi::node_pcdata).set_value(boost::lexical_cast<char *>(Lyric.nInfo));

	pugi::xml_node strLyric = query.append_child();
	strLyric.set_name("ns1:strLyric");
	strLyric.append_child(pugi::node_pcdata).set_value(Lyric.Lyric.c_str());

	pugi::xml_node nPlayTime = query.append_child();
	nPlayTime.set_name("ns1:nPlayTime");
	nPlayTime.append_child(pugi::node_pcdata).set_value(boost::lexical_cast<char *>(PlayTime));

	pugi::xml_node strVersion = query.append_child();
	strVersion.set_name("ns1:strVersion");
	strVersion.append_child(pugi::node_pcdata).set_value(ALSONG_VERSION);

	pugi::xml_node strMACAddress = query.append_child();
	strMACAddress.set_name("ns1:strMACAddress");
	strMACAddress.append_child(pugi::node_pcdata).set_value(Local_Mac);

	pugi::xml_node strIPAddress = query.append_child();
	strIPAddress.set_name("ns1:strIPAddress");
	strIPAddress.append_child(pugi::node_pcdata).set_value(Local_IP);

	std::stringstream str;
	pugi::xml_writer_stream writer(str);
	xmldoc.save(writer, "", pugi::format_raw);

	wsprintfA(buf, UploadLyricHeader, str.str().length());
	
	send(s, buf, lstrlenA(buf), 0);
	send(s, str.str().c_str(), str.str().length(), 0);
	
	while(nRecv = recv(s, buf, 255, 0))
	{
		if(SOCKET_ERROR == nRecv)
		{
			/*int t = WSAGetLastError();
			wsprintfA(buf, "Error receiving data. WSAGetLastError() = %d", t);
			MessageBoxA(NULL, buf, "Error", MB_OK);*/
			closesocket(s);
			return false;
		}
		buf[nRecv] = 0;
		data.insert(data.end(), buf, buf + nRecv);
	}

	pugi::xml_document doc;
	doc.load(&*boost::find_first(data, "\r\n\r\n").begin());
	pugi::xml_node xmlresult = doc.first_element_by_path("soap:Envelope/soap:Body/UploadLyricResult/UploadLyricResult"); //TODO: Test

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
			WriteFile(hFile, m_Lyric[i].lyric.c_str(), m_Lyric[i].lyric.length(), &dwWritten, NULL);
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
			WriteFile(hFile, m_Lyric[i].lyric.c_str(), m_Lyric[i].lyric.length(), &dwWritten, NULL);
			WriteFile(hFile, "\r\n", 2, &dwWritten, NULL);
		}

		CloseHandle(hFile);
	}
}

void LyricManager::OpenLyricModifyDialog(HWND hWndParent)
{
	DialogBox(core_api::get_my_instance(), MAKEINTRESOURCE(IDD_LYRIC_MODIFY), hWndParent, (DLGPROC)&LyricManager::LyricModifyDialogProc);
}

void LyricManager::PopulateListView(HWND hListView, LyricSearchResult &res)
{
	LyricResult lrc;
	int n = 0;
	ListView_DeleteAllItems(hListView);
	while(lrc = res.Get())
	{
		std::wstring artist = pfc::stringcvt::string_wide_from_utf8(lrc.Artist.c_str()).get_ptr();
		std::wstring title = pfc::stringcvt::string_wide_from_utf8(lrc.Title.c_str()).get_ptr();
		LVITEM item;
		item.mask = LVIF_TEXT | LVIF_PARAM;
		item.iItem = n ++;
		item.iSubItem = 0;
		item.pszText = const_cast<WCHAR *>(artist.c_str());
		item.lParam = lrc.nInfo;
		ListView_InsertItem(hListView, &item);
		item.iSubItem = 1;
		item.pszText = const_cast<WCHAR *>(title.c_str());
		ListView_SetItem(hListView, &item);
	}
}

UINT CALLBACK LyricManager::LyricModifyDialogProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	static int lyriccount;
	static LyricSearchResult searchresult;
	static int page = 0;
	switch(iMessage)
	{
	case WM_INITDIALOG:
		{
			//get title text
			static_api_ptr_t<play_control> pc;
			metadb_handle_ptr handle;
			pc->get_now_playing(handle);
			uSetWindowText(hWnd, handle->get_path());
			
			//set artist, title field
			service_ptr_t<titleformat_object> to;
			pfc::string8 artist;
			pfc::string8 title;

			static_api_ptr_t<titleformat_compiler>()->compile_safe(to, "%artist%");
			handle->format_title(NULL, artist, to, NULL);
			static_api_ptr_t<titleformat_compiler>()->compile_safe(to, "%title%");
			handle->format_title(NULL, title, to, NULL);
			uSetDlgItemText(hWnd, IDC_ARTIST, artist.get_ptr());
			uSetDlgItemText(hWnd, IDC_TITLE, title.get_ptr());
			//perform listview initialization.

			LVCOLUMN lv;
			lv.mask = LVCF_WIDTH | LVCF_TEXT;
			lv.cx = 150;
			lv.pszText = TEXT("아티스트");
			ListView_InsertColumn(GetDlgItem(hWnd, IDC_LYRICLIST), 0, &lv);
			lv.pszText = TEXT("제목");
			ListView_InsertColumn(GetDlgItem(hWnd, IDC_LYRICLIST), 1, &lv);
		}
		return TRUE;
	case WM_COMMAND:
		if(HIWORD(wParam) == BN_CLICKED)
		{
			switch(LOWORD(wParam))
			{
			case IDC_SEARCH:
				{
					pfc::string8 artist;
					uGetDlgItemText(hWnd, IDC_ARTIST, artist);
					pfc::string8 title;
					uGetDlgItemText(hWnd, IDC_TITLE, title);
					
					page = 0;
					lyriccount = LyricManager::SearchLyricGetCount(artist.toString(), title.toString());
					std::stringstream str;
					str << page * 100 + 1 << "~" << min(lyriccount, (page + 1) * 100) << "/" << lyriccount;
					uSetDlgItemText(hWnd, IDC_STATUS, str.str().c_str());
					LyricManager::SearchLyric(artist.toString(), title.toString(), 0, searchresult);
					LyricManager::PopulateListView(GetDlgItem(hWnd, IDC_LYRICLIST), searchresult);
				}
				break;
			case IDC_RESET:
				SetDlgItemText(hWnd, IDC_ARTIST, TEXT(""));
				SetDlgItemText(hWnd, IDC_TITLE, TEXT(""));
				SetDlgItemText(hWnd, IDC_STATUS, TEXT(""));
				ListView_DeleteAllItems(GetDlgItem(hWnd, IDC_LYRICLIST));
				SetDlgItemText(hWnd, IDC_LYRIC, TEXT(""));
				SetFocus(GetDlgItem(hWnd, IDC_ARTIST));
				//reset;
				break;
			case IDC_NEWLYRIC:
				//something
				break;
			case IDC_PREV:
				//something;
				break;
			case IDC_NEXT:
				//some
				break;
			case IDC_SYNCEDIT:
				break;
			case IDC_REGISTER:
				break;
			case IDC_CANCEL:
				EndDialog(hWnd, 0);
				break;
			}
		}
		return TRUE;
	}
	return FALSE;
}

LyricResult &LyricSearchResult::Get()
{
	if(!node)
		return LyricResultMap[-1]; //invalid item
	
	LyricResult ret;
	ret.Album = node.child("strAlbumName").child_value();
	ret.Title = node.child("strTitle").child_value();
	ret.Artist = node.child("strArtistName").child_value();
	ret.Registrant = node.child("strRegisterFirstName").child_value();
	ret.Lyric = node.child("strLyric").child_value();
	ret.nInfo = boost::lexical_cast<int>(node.child("strInfoID").child_value());
	node = node.next_sibling("ST_GET_RESEMBLELYRIC2_RETURN");
	LyricResultMap[ret.nInfo] = ret;

	return LyricResultMap[ret.nInfo];
}

LyricResult &LyricSearchResult::Get(int nInfo)
{
	if(LyricResultMap.find(nInfo) != LyricResultMap.end())
		return LyricResultMap[nInfo];
	else
		return LyricResultMap[-1];
}
