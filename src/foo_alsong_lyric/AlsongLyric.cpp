#include "stdafx.h"

#include "AlsongLyric.h"
#include "SoapHelper.h"
#include "md5.h"

#define ALSONG_VERSION "2.11"

AlsongLyric::AlsongLyric(const pugi::xml_node &node)
{
	if(node.child("strAlbum").child_value())
	{ //getlyric
		m_Title = node.child("strTitle").child_value();
		m_Artist = node.child("strArtist").child_value();
		m_Album = node.child("strAlbum").child_value();
		m_Registrant = node.child("strRegisterFirstName").child_value();
		m_Lyric = node.child("strLyric").child_value();

		if(!m_Album.compare(m_Title))
			m_Album.clear();
		m_nInfoID = -1;
	}
	//search
	else
	{
		m_Album = node.child("strAlbumName").child_value();
		m_Title = node.child("strTitle").child_value();
		m_Artist = node.child("strArtistName").child_value();
		m_Registrant = node.child("strRegisterFirstName").child_value();
		m_Lyric = node.child("strLyric").child_value();
		m_nInfoID = boost::lexical_cast<int>(node.child("strInfoID").child_value());
	}

	Split("<br>");
}

DWORD AlsongLyric::GetFileHash(metadb_handle_ptr track, CHAR *Hash)
{	
	int i;
	DWORD Start = 0; //Start Address
	BYTE MD5[16];
	BYTE temp[255]; 

	service_ptr_t<file> file;
	abort_callback_impl abort_callback;
	pfc::string8 str = track->get_path();
	
	try
	{
		archive_impl::g_open(file, str, foobar2000_io::filesystem::open_mode_read, abort_callback);
	}
	catch(...)
	{
		return false;
	}
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

AlsongLyric AlsongLyric::LyricFromAlsong(const metadb_handle_ptr &track)
{
	struct hostent *host;
	CHAR Hostname[80];
	CHAR *Local_IP;
	CHAR Local_Mac[20];

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
		return AlsongLyric();

	CHAR HexArray[] = "0123456789ABCDEF";
	int i;
	for(i = 0; i < 12; i += 2)
	{
		Local_Mac[i] = HexArray[(pAdapterInfo->Address[i / 2] & 0xf0) >> 4];
		Local_Mac[i + 1] = HexArray[pAdapterInfo->Address[i / 2] & 0x0f];
	}
	Local_Mac[i] = 0;

	CHAR Hash[50];
	GetFileHash(track, Hash);

	SoapHelper helper;
	helper.SetMethod("ns1:GetLyric5");
	helper.AddParameter("ns1:strChecksum", Hash);
	helper.AddParameter("ns1:strVersion", ALSONG_VERSION);
	helper.AddParameter("ns1:strMACAddress", Local_Mac);
	helper.AddParameter("ns1:strIPAddress", Local_IP);

	if(boost::this_thread::interruption_requested())
		return AlsongLyric();

	AlsongLyric ret(helper.Execute()->first_element_by_path("soap:Envelope/soap:Body/GetLyric5Response/GetLyric5Result"));

	return ret;
}

DWORD AlsongLyric::LyricToAlsong(metadb_handle_ptr track, const AlsongLyric &Lyric)
{
	CHAR strRegisterName[] = "Alsong Lyric Plugin for Foobar2000";//나머지는 생략

	//UploadLyricType - 1:Link 새거 2:Modify 수정 5:ReSetLink 아예 새거

	struct hostent *host;
	CHAR Hostname[80];
	CHAR *Local_IP;
	CHAR Local_Mac[20];
	CHAR Hash[50];
	
	std::string Filename = track->get_path();
	int PlayTime = (int)(track->get_length() * 1000);

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
	int i;
	for(i = 0; i < 12; i += 2)
	{
		Local_Mac[i] = HexArray[(pAdapterInfo->Address[i / 2] & 0xf0) >> 4];
		Local_Mac[i + 1] = HexArray[pAdapterInfo->Address[i / 2] & 0x0f];
	}
	Local_Mac[i] = 0;

	GetFileHash(track, Hash);

	SoapHelper helper;
	helper.SetMethod("ns1:UploadLyric");
	helper.AddParameter("ns1:nUploadLyricType", "1");
	helper.AddParameter("ns1:strVersion", ALSONG_VERSION);
	helper.AddParameter("ns1:strMD5", Hash);
	helper.AddParameter("ns1:strRegisterFirstName", Lyric.m_Registrant.c_str());
	helper.AddParameter("ns1:strRegisterFirstEMail", "");
	helper.AddParameter("ns1:strRegisterFirstURL", "");
	helper.AddParameter("ns1:strRegisterFirstPhone", "");
	helper.AddParameter("ns1:strRegisterFirstComment", "");
	helper.AddParameter("ns1:strRegisterName", strRegisterName);
	helper.AddParameter("ns1:strRegisterEMail", "");
	helper.AddParameter("ns1:strRegisterURL", "");
	helper.AddParameter("ns1:strRegisterPhone", "");
	helper.AddParameter("ns1:strRegisterComment", "");
	helper.AddParameter("ns1:strFileName", Filename.c_str());
	helper.AddParameter("ns1:strTitle", Lyric.m_Title.c_str());
	helper.AddParameter("ns1:strArtist", Lyric.m_Artist.c_str());
	helper.AddParameter("ns1:strAlbum", Lyric.m_Album.c_str());
	helper.AddParameter("ns1:nInfoID", boost::lexical_cast<std::string>(Lyric.m_nInfoID).c_str());
	helper.AddParameter("ns1:strLyric", Lyric.m_Lyric.c_str());
	helper.AddParameter("ns1:nPlayTime", boost::lexical_cast<std::string>(PlayTime).c_str());
	helper.AddParameter("ns1:strVersion", ALSONG_VERSION);
	helper.AddParameter("ns1:strMACAddress", Local_Mac);
	helper.AddParameter("ns1:strIPAddress", Local_IP);

	const char *res = helper.Execute()->first_element_by_path("soap:Envelope/soap:Body/UploadLyricResponse/UploadLyricResult").child_value();
	if(boost::find_first(res, "Successed"))
		return true;
	return false;
}

