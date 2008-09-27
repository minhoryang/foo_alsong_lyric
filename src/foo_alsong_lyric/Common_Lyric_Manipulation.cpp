#include "stdafx.h"
#include "Common_Settings.h"
#include "Common_UI.h"
#include "Common_Lyric_Manipulation.h"
//TODO: USLT 태그(4바이트 타임스탬프, 1바이트 길이, 문자열(유니코드), 0x08 순으로 들어있음)

#define GET_XML_DATA(xml, tag, data) lstrcpynA(data, GET_XML_POS((xml), (tag)), GET_XML_LEN((xml), (tag)) + 1)
#define GET_XML_LEN(xml, tag) (abs(StrStrA((xml), (tag)) - StrStrA(StrStrA((xml), (tag)) + lstrlenA((tag)), (tag))) - lstrlenA((tag)) - 3)
#define GET_XML_POS(xml, tag) (StrStrA((xml), (tag)) + lstrlenA((tag)) + 1)

Common_Lyric_Manipulation::Common_Lyric_Manipulation()
{

}

Common_Lyric_Manipulation::~Common_Lyric_Manipulation()
{
	
}

void Common_Lyric_Manipulation::ClearLyric()
{
	Lyric.clear();
	Time.clear();
}

const char *Common_Lyric_Manipulation::GetLyric(DWORD Line)
{
	if(Line < Lyric.size())
		return Lyric[Line].c_str();
	else
		return NULL;
}

DWORD Common_Lyric_Manipulation::GetNumberOfLine()
{
	return Lyric.size();
}

DWORD Common_Lyric_Manipulation::GetLyricTime(DWORD Line)
{
	if(Line < Time.size())
		return Time[Line];
	else
		return 4294967295;
}

WCHAR *Common_Lyric_Manipulation::GetStatus()
{
	return (WCHAR *)Status.c_str();
}

DWORD Common_Lyric_Manipulation::GetFileHash(unsigned char *Data, int Size, CHAR *Hash, CHAR *fmt)
{	
	int i;
	int tmp;
	DWORD Start; //Start Address
	BYTE MD5[16];

	__try
	{

		if(!StrCmpIA(fmt, "mp3"))
		{ //MP3	
			tmp = 0;
			Start = 0;

			while(1) //ID3가 여러개 있을수도 있음
			{ //ID3는 보통 맨 처음에 있음
				Start += tmp;
				if(Data[Start] == 'I' && Data[Start + 1] == 'D' && Data[Start + 2] == '3')
				{
					//ID3 Tag
					tmp = Data[Start + 9]; //Decode Tag Size
					tmp |= Data[Start + 8] << 7;
					tmp |= Data[Start + 7] << 14;
					tmp |= Data[Start + 6] << 21;
					tmp += 10; //Header Size
				}
				else
					break;
			}
			if(Start != 0)
				for(;;Start ++)
					if(Data[Start] == 0xFF) //MP3 Header까지
						break;
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
				if(!memcmp(Data + i, SetupHeader, 7))
				{
					if(!memcmp(Data + i + 7 + 1, BCV, 3)) //Setup Header와 BCV 사이에 뭔가 바이트가 하나 더 있다.
					{
						//여기부터다
						Start = i + 7 + 1 + 3;
						break;
					}
				}
				i ++;
				if(i > Size)
					return ERROR_UNSUPPORTED_EXTENSION; //에러
			}

		}
		else if(!StrCmpIA(fmt, "wav"))
			Start = 0;
		else
		{
			//에러처리
			Start = 0;
			return ERROR_UNSUPPORTED_EXTENSION;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		Start = 0;
	}

	md5(Data + Start, min(0x28000, Size - Start), MD5); //FileSize < 0x28000 일수도

	CHAR HexArray[] = "0123456789abcdef";

	for(i = 0; i < 32; i += 2)
	{
		Hash[i] = HexArray[(MD5[i / 2] & 0xf0) >> 4];
		Hash[i + 1] = HexArray[MD5[i / 2] & 0x0f];
	}
	Hash[i] = 0;

	return S_OK;
}

DWORD Common_Lyric_Manipulation::DownloadLyric(CHAR *Hash)
{
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
	CHAR Version[] = "1.97";
	CHAR buf[255];
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
		return ERROR_CONNECTION;

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
		return ERROR_CONNECTION;
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
			return ERROR_CONNECTION;
		}
		CopyMemory(data + nUse, buf, nRecv);
		nUse += nRecv;
		if(nUse + 255 > nAlloc - 100)
		{
			data = (CHAR *)realloc(data, nAlloc + 300);
			nAlloc += 300;
		}
	}

	//이제 XML 처리를 하자..

	__try
	{
		GET_XML_DATA(data, "strStatusID", buf);
		if(buf[0] == '2')
		{
			free(data);
			closesocket(s);
			return ERROR_LYRIC_NOT_FOUND;
		}
		
		Title.assign(GET_XML_POS(data, "strTitle"), GET_XML_LEN(data, "strTitle"));
		Artist.assign(GET_XML_POS(data, "strArtist"), GET_XML_LEN(data, "strArtist"));
		Album.assign(GET_XML_POS(data, "strAlbum"), GET_XML_LEN(data, "strAlbum"));
		Registrant.assign(GET_XML_POS(data, "strRegisterFirstName"), GET_XML_LEN(data, "strRegisterFirstName"));

		if(!Album.compare(Title))
			Album[0] = 0;//Album이 Title과 같은 경우(없는걸로 처리)

		GET_XML_POS(data, "strLyric")[GET_XML_LEN(data, "strLyric")] = 0;
		ParseLyric(GET_XML_POS(data, "strLyric"), "&lt;br&gt;");
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		free(data);
		closesocket(s);
		//MessageBox(NULL, TEXT("오류 발생"), TEXT("오류"), MB_OK);
		return ERROR_LYRIC_NOT_FOUND;
	}
	free(data);
	closesocket(s);

	return S_OK;
}

DWORD Common_Lyric_Manipulation::FetchLyric(BYTE *FileHead, int Size, CHAR *fmt)
{
	CHAR Hash[33];
	DWORD nRet;

	//기존 정보 삭제
	Lyric.clear();
	Time.clear();

	Status = TEXT("파일 정보 처리중...");
	Common_UI->InvalidateAllWindow();

	nRet = GetFileHash(FileHead, Size, Hash, fmt);
	if(nRet == ERROR_UNSUPPORTED_EXTENSION)
	{
		Status = TEXT("지원하지 않는 파일 형식입니다.");
		Common_UI->InvalidateAllWindow();
		return S_OK;
	}

	Status = TEXT("가사 다운로드 중...");
	Common_UI->InvalidateAllWindow();

	nRet = DownloadLyric(Hash);

	if(nRet == ERROR_LYRIC_NOT_FOUND)
		Status = TEXT("실시간 가사를 찾을 수 없습니다.");
	else if(nRet == ERROR_CONNECTION)
		Status = TEXT("알송 서버와의 연결에 실패했습니다.");
	else if(nRet == S_OK)
		Status = TEXT("");
	Common_UI->InvalidateAllWindow();

	return S_OK;
}

DWORD Common_Lyric_Manipulation::ParseLyric(CHAR *InputLyric, CHAR *Delimiter)
{
	int i;

	CHAR *nowpos = InputLyric;
	CHAR *lastpos = nowpos;
	int pos;
	for(i = 0; ; i ++) //<br>자르기/ LineTotal은 안전빵
	{
		pos = pfc::strstr_ex(nowpos + 1, lstrlenA(nowpos + 1), Delimiter, lstrlenA(Delimiter));
		if(pos == -1)
			break;
		nowpos = nowpos + 1 + pos;
		nowpos[0] = 0;
		
		Time.push_back(StrToIntA(lastpos + 1) * 60 * 100 + StrToIntA(lastpos + 4) * 100 + StrToIntA(lastpos + 7));
		lastpos += 10; //시간정보는 10글자다

		Lyric.push_back(lastpos);
		lastpos = nowpos + lstrlenA(Delimiter);

		while(Lyric[i].find("&amp;") != string::npos)
			Lyric[i].replace(Lyric[i].find("&amp;"), 5, "&");
		while(Lyric[i].find("&lt;") != string::npos)
			Lyric[i].replace(Lyric[i].find("&lt;"), 4, "<");
		while(Lyric[i].find("&gt;") != string::npos)
			Lyric[i].replace(Lyric[i].find("&gt;"), 4, ">");
	}

	return S_OK;
}

DWORD Common_Lyric_Manipulation::SearchLyricGetNext(CHAR **data, int *Info, string *Title, string *Artist, string *Album, string *Lyric, string *Registrant)
{
	__try
	{
		Title->assign(GET_XML_POS(*data, "strTitle"), GET_XML_LEN(*data, "strTitle"));
		Artist->assign(GET_XML_POS(*data, "strArtistName"), GET_XML_LEN(*data, "strArtistName"));
		Album->assign(GET_XML_POS(*data, "strAlbumName"), GET_XML_LEN(*data, "strAlbumName"));
		Registrant->assign(GET_XML_POS(*data, "strRegisterFirstName"), GET_XML_LEN(*data, "strRegisterFirstName"));
		Lyric->assign(GET_XML_POS(*data, "strLyric"), GET_XML_LEN(*data, "strLyric"));
		CHAR temp[255];
		GET_XML_DATA(*data, "strInfoID", temp);
		*Info = StrToIntA(temp);
		*data = GET_XML_POS(*data, "ST_GET_RESEMBLELYRIC2_RETURN");
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return ERROR_LYRIC_NOT_FOUND;
	}

	return S_OK;
}

DWORD Common_Lyric_Manipulation::SearchLyric(CHAR *InArtist, CHAR *InTitle, int nPage, CHAR **Output)
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

	*Output = (CHAR *)malloc(sizeof(CHAR) * 600);

	s = InitateConnect("lyrics.alsong.co.kr", 80);
	if(s == 0)
	{
		free(Output);
		return ERROR_CONNECTION;
	}
	len = lstrlenA(GetLyricData1) + lstrlenA(GetLyricData2) + lstrlenA(GetLyricData3) + lstrlenA(GetLyricData4) + lstrlenA(InArtist) + lstrlenA(InTitle);
	if(nPage != 0)
		len += (int)log10((float)nPage) + 1;
	else
		len += 1;
	wsprintfA(buf, GetLyricHeader, len);

	send(s, buf, lstrlenA(buf), 0);
	send(s, GetLyricData1, lstrlenA(GetLyricData1), 0);
	send(s, InTitle, lstrlenA(InTitle), 0);
	send(s, GetLyricData2, lstrlenA(GetLyricData2), 0);
	send(s, InArtist, lstrlenA(InArtist), 0);
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
			return ERROR_CONNECTION;
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

	return S_OK;
}

int Common_Lyric_Manipulation::SearchLyricGetCount(CHAR *Artist, CHAR *Title)
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
	{
		return 0;
	}

	len = lstrlenA(GetCountData1) + lstrlenA(GetCountData2) + lstrlenA(GetCountData3) + lstrlenA(Artist) + lstrlenA(Title);
	wsprintfA(buf, GetCountHeader, len);

	send(s, buf, lstrlenA(buf), 0);
	send(s, GetCountData1, lstrlenA(GetCountData1), 0);
	send(s, Title, lstrlenA(Title), 0);
	send(s, GetCountData2, lstrlenA(GetCountData2), 0);
	send(s, Artist, lstrlenA(Artist), 0);
	send(s, GetCountData3, lstrlenA(GetCountData3), 0);

	while(nRecv = recv(s, buf, 255, 0))
	{
		if(SOCKET_ERROR == nRecv)
		{/*
			int t = WSAGetLastError();
			wsprintfA(buf, "Error receiving data. WSAGetLastError() = %d", t);
			MessageBoxA(NULL, buf, "Error", MB_OK);*/
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

	return ret;
}

SOCKET Common_Lyric_Manipulation::InitateConnect(CHAR *Address, int port)
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
		return 0;

	return s;
}

DWORD Common_Lyric_Manipulation::LoadFromFile(WCHAR *LoadFrom, CHAR *fmt)
{
	ClearLyric();
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

	return FALSE;
}

DWORD Common_Lyric_Manipulation::UploadLyric(CHAR *FileName, int PlayTime, int nInfo, int UploadType, string *Lyric, string *Title, string *Artist, string *Album, string *Registrant)
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
	CHAR Version[] = "1.97";
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
		return ERROR_CONNECTION;

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
		return ERROR_CONNECTION;
	}

	pfc::string8 str;
	str = FileName;

	service_ptr_t<file> file;
	abort_callback_impl abort_callback;
	archive_impl::g_open(file, str, foobar2000_io::filesystem::open_mode_read, abort_callback);
	//TODO:cue일때 특별 처리(subsong_index가 있을 때)
	data = (CHAR *)malloc(min(0x50000, (size_t)file->get_size(abort_callback)));
	//다 읽지 말고 처음 부분만 읽자
	file->read(data, min(0x50000, (size_t)file->get_size(abort_callback)), abort_callback);
	GetFileHash((unsigned char *)data, min(0x50000, (size_t)file->get_size(abort_callback)), Hash, (char *)str.get_ptr() + str.find_last('.') + 1);
	free(data);

	while(Lyric->find("\r\n") != string::npos)
		Lyric->replace(Lyric->find("\r\n"), 2, "&lt;br&gt;");

	for(i = 0; i < 23; i ++)
		len += lstrlenA(UploadLyricData[i]);
	len += lstrlenA(strRegisterName);
	len += (int)log10((float)nInfo) + 1;
	len += Lyric->length() + Artist->length() + Registrant->length() + Album->length() + Title->length();
	len += 1;
	len += lstrlenA(Hash);
	len += str.length();
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
	send(s, Registrant->c_str(), Registrant->length(), 0);
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
	send(s, str.get_ptr(), str.length(), 0);
	send(s, UploadLyricData[13], lstrlenA(UploadLyricData[13]), 0);
	send(s, Title->c_str(), Title->length(), 0);
	send(s, UploadLyricData[14], lstrlenA(UploadLyricData[14]), 0);
	send(s, Artist->c_str(), Artist->length(), 0);
	send(s, UploadLyricData[15], lstrlenA(UploadLyricData[15]), 0);
	send(s, Album->c_str(), Album->length(), 0);
	send(s, UploadLyricData[16], lstrlenA(UploadLyricData[16]), 0);
	wsprintfA(buf, "%d", nInfo);
	send(s, buf, lstrlenA(buf), 0);
	send(s, UploadLyricData[17], lstrlenA(UploadLyricData[17]), 0);
	send(s, Lyric->c_str(), Lyric->length(), 0);
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
			/*int t = WSAGetLastError();
			wsprintfA(buf, "Error receiving data. WSAGetLastError() = %d", t);
			MessageBoxA(NULL, buf, "Error", MB_OK);*/
			free(data);
			closesocket(s);
			return ERROR_CONNECTION;
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
		return ERROR_UNKNOWN;
	}

	free(data);
	
	return S_OK;
}

void Common_Lyric_Manipulation::SaveToFile(WCHAR *SaveTo, CHAR *fmt)
{
	if(GetLyric(0) == NULL)
		return;
	if(!StrCmpIA(fmt, "lrc"))
	{
		HANDLE hFile = CreateFile(SaveTo, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, NULL, NULL);
		if(hFile == INVALID_HANDLE_VALUE)
			return;
		unsigned int i;
		CHAR temp[255];
		DWORD dwWritten;

		for(i = 0; i < GetNumberOfLine(); i ++)
		{
			wsprintfA(temp, "[%02d:%02d.%02d]", (int)(Time[i] / 100) / 60, ((int)Time[i] / 100) % 60, (int)(Time[i] % 100));
			WriteFile(hFile, temp, lstrlenA(temp), &dwWritten, NULL);
			WriteFile(hFile, Lyric[i].c_str(), Lyric[i].length(), &dwWritten, NULL);
			temp[0] = '\r';
			temp[1] = '\n';
			temp[2] = 0;
			WriteFile(hFile, temp, 2, &dwWritten, NULL);
		}

		CloseHandle(hFile);
	}
	else if(!StrCmpIA(fmt, "txt"))
	{
		HANDLE hFile = CreateFile(SaveTo, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, NULL, NULL);
		unsigned int i;
		CHAR temp[255];
		DWORD dwWritten;

		for(i = 0; i < GetNumberOfLine(); i ++)
		{
			WriteFile(hFile, Lyric[i].c_str(), Lyric[i].length(), &dwWritten, NULL);
			temp[0] = '\r';
			temp[1] = '\n';
			temp[2] = 0;
			WriteFile(hFile, temp, 2, &dwWritten, NULL);
		}

		CloseHandle(hFile);
	}
}

typedef struct
{
	unsigned long total[2];
	unsigned long state[4];
	unsigned char buffer[64];

	unsigned char ipad[64];  
	unsigned char opad[64];  
} md5_context;

#define GET_ULONG_LE(n,b,i)                             \
{                                                       \
	(n) = ( (unsigned long) (b)[(i)    ]   )            \
	| ( (unsigned long) (b)[(i) + 1] <<  8 )            \
	| ( (unsigned long) (b)[(i) + 2] << 16 )            \
	| ( (unsigned long) (b)[(i) + 3] << 24 );           \
}

#define PUT_ULONG_LE(n,b,i)                             \
{                                                       \
	(b)[(i)    ] = (unsigned char) ( (n)       );       \
	(b)[(i) + 1] = (unsigned char) ( (n) >>  8 );       \
	(b)[(i) + 2] = (unsigned char) ( (n) >> 16 );       \
	(b)[(i) + 3] = (unsigned char) ( (n) >> 24 );       \
}

void md5_starts( md5_context *ctx )
{
	ctx->total[0] = 0;
	ctx->total[1] = 0;

	ctx->state[0] = 0x67452301;
	ctx->state[1] = 0xEFCDAB89;
	ctx->state[2] = 0x98BADCFE;
	ctx->state[3] = 0x10325476;
}

static void md5_process( md5_context *ctx, unsigned char data[64] )
{
	unsigned long X[16], A, B, C, D;

	GET_ULONG_LE( X[ 0], data,  0 );
	GET_ULONG_LE( X[ 1], data,  4 );
	GET_ULONG_LE( X[ 2], data,  8 );
	GET_ULONG_LE( X[ 3], data, 12 );
	GET_ULONG_LE( X[ 4], data, 16 );
	GET_ULONG_LE( X[ 5], data, 20 );
	GET_ULONG_LE( X[ 6], data, 24 );
	GET_ULONG_LE( X[ 7], data, 28 );
	GET_ULONG_LE( X[ 8], data, 32 );
	GET_ULONG_LE( X[ 9], data, 36 );
	GET_ULONG_LE( X[10], data, 40 );
	GET_ULONG_LE( X[11], data, 44 );
	GET_ULONG_LE( X[12], data, 48 );
	GET_ULONG_LE( X[13], data, 52 );
	GET_ULONG_LE( X[14], data, 56 );
	GET_ULONG_LE( X[15], data, 60 );

#define S(x,n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))

#define P(a,b,c,d,k,s,t)                                \
	{                                                   \
	a += F(b,c,d) + X[k] + t; a = S(a,s) + b;           \
	}

	A = ctx->state[0];
	B = ctx->state[1];
	C = ctx->state[2];
	D = ctx->state[3];

#define F(x,y,z) (z ^ (x & (y ^ z)))

	P( A, B, C, D,  0,  7, 0xD76AA478 );
	P( D, A, B, C,  1, 12, 0xE8C7B756 );
	P( C, D, A, B,  2, 17, 0x242070DB );
	P( B, C, D, A,  3, 22, 0xC1BDCEEE );
	P( A, B, C, D,  4,  7, 0xF57C0FAF );
	P( D, A, B, C,  5, 12, 0x4787C62A );
	P( C, D, A, B,  6, 17, 0xA8304613 );
	P( B, C, D, A,  7, 22, 0xFD469501 );
	P( A, B, C, D,  8,  7, 0x698098D8 );
	P( D, A, B, C,  9, 12, 0x8B44F7AF );
	P( C, D, A, B, 10, 17, 0xFFFF5BB1 );
	P( B, C, D, A, 11, 22, 0x895CD7BE );
	P( A, B, C, D, 12,  7, 0x6B901122 );
	P( D, A, B, C, 13, 12, 0xFD987193 );
	P( C, D, A, B, 14, 17, 0xA679438E );
	P( B, C, D, A, 15, 22, 0x49B40821 );

#undef F

#define F(x,y,z) (y ^ (z & (x ^ y)))

	P( A, B, C, D,  1,  5, 0xF61E2562 );
	P( D, A, B, C,  6,  9, 0xC040B340 );
	P( C, D, A, B, 11, 14, 0x265E5A51 );
	P( B, C, D, A,  0, 20, 0xE9B6C7AA );
	P( A, B, C, D,  5,  5, 0xD62F105D );
	P( D, A, B, C, 10,  9, 0x02441453 );
	P( C, D, A, B, 15, 14, 0xD8A1E681 );
	P( B, C, D, A,  4, 20, 0xE7D3FBC8 );
	P( A, B, C, D,  9,  5, 0x21E1CDE6 );
	P( D, A, B, C, 14,  9, 0xC33707D6 );
	P( C, D, A, B,  3, 14, 0xF4D50D87 );
	P( B, C, D, A,  8, 20, 0x455A14ED );
	P( A, B, C, D, 13,  5, 0xA9E3E905 );
	P( D, A, B, C,  2,  9, 0xFCEFA3F8 );
	P( C, D, A, B,  7, 14, 0x676F02D9 );
	P( B, C, D, A, 12, 20, 0x8D2A4C8A );

#undef F

#define F(x,y,z) (x ^ y ^ z)

	P( A, B, C, D,  5,  4, 0xFFFA3942 );
	P( D, A, B, C,  8, 11, 0x8771F681 );
	P( C, D, A, B, 11, 16, 0x6D9D6122 );
	P( B, C, D, A, 14, 23, 0xFDE5380C );
	P( A, B, C, D,  1,  4, 0xA4BEEA44 );
	P( D, A, B, C,  4, 11, 0x4BDECFA9 );
	P( C, D, A, B,  7, 16, 0xF6BB4B60 );
	P( B, C, D, A, 10, 23, 0xBEBFBC70 );
	P( A, B, C, D, 13,  4, 0x289B7EC6 );
	P( D, A, B, C,  0, 11, 0xEAA127FA );
	P( C, D, A, B,  3, 16, 0xD4EF3085 );
	P( B, C, D, A,  6, 23, 0x04881D05 );
	P( A, B, C, D,  9,  4, 0xD9D4D039 );
	P( D, A, B, C, 12, 11, 0xE6DB99E5 );
	P( C, D, A, B, 15, 16, 0x1FA27CF8 );
	P( B, C, D, A,  2, 23, 0xC4AC5665 );

#undef F

#define F(x,y,z) (y ^ (x | ~z))

	P( A, B, C, D,  0,  6, 0xF4292244 );
	P( D, A, B, C,  7, 10, 0x432AFF97 );
	P( C, D, A, B, 14, 15, 0xAB9423A7 );
	P( B, C, D, A,  5, 21, 0xFC93A039 );
	P( A, B, C, D, 12,  6, 0x655B59C3 );
	P( D, A, B, C,  3, 10, 0x8F0CCC92 );
	P( C, D, A, B, 10, 15, 0xFFEFF47D );
	P( B, C, D, A,  1, 21, 0x85845DD1 );
	P( A, B, C, D,  8,  6, 0x6FA87E4F );
	P( D, A, B, C, 15, 10, 0xFE2CE6E0 );
	P( C, D, A, B,  6, 15, 0xA3014314 );
	P( B, C, D, A, 13, 21, 0x4E0811A1 );
	P( A, B, C, D,  4,  6, 0xF7537E82 );
	P( D, A, B, C, 11, 10, 0xBD3AF235 );
	P( C, D, A, B,  2, 15, 0x2AD7D2BB );
	P( B, C, D, A,  9, 21, 0xEB86D391 );

#undef F

	ctx->state[0] += A;
	ctx->state[1] += B;
	ctx->state[2] += C;
	ctx->state[3] += D;
}

/*
* MD5 process buffer
*/
void md5_update( md5_context *ctx, unsigned char *input, int ilen )
{
	int fill;
	unsigned long left;

	if( ilen <= 0 )
		return;

	left = ctx->total[0] & 0x3F;
	fill = 64 - left;

	ctx->total[0] += ilen;
	ctx->total[0] &= 0xFFFFFFFF;

	if( ctx->total[0] < (unsigned long) ilen )
		ctx->total[1]++;

	if( left && ilen >= fill )
	{
		memcpy( (void *) (ctx->buffer + left),
			(void *) input, fill );
		md5_process( ctx, ctx->buffer );
		input += fill;
		ilen  -= fill;
		left = 0;
	}

	while( ilen >= 64 )
	{
		md5_process( ctx, input );
		input += 64;
		ilen  -= 64;
	}

	if( ilen > 0 )
	{
		memcpy( (void *) (ctx->buffer + left),
			(void *) input, ilen );
	}
}

static const unsigned char md5_padding[64] =
{
	0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/*
* MD5 final digest
*/
void md5_finish( md5_context *ctx, unsigned char output[16] )
{
	unsigned long last, padn;
	unsigned long high, low;
	unsigned char msglen[8];

	high = ( ctx->total[0] >> 29 )
		| ( ctx->total[1] <<  3 );
	low  = ( ctx->total[0] <<  3 );

	PUT_ULONG_LE( low,  msglen, 0 );
	PUT_ULONG_LE( high, msglen, 4 );

	last = ctx->total[0] & 0x3F;
	padn = ( last < 56 ) ? ( 56 - last ) : ( 120 - last );

	md5_update( ctx, (unsigned char *) md5_padding, padn );
	md5_update( ctx, msglen, 8 );

	PUT_ULONG_LE( ctx->state[0], output,  0 );
	PUT_ULONG_LE( ctx->state[1], output,  4 );
	PUT_ULONG_LE( ctx->state[2], output,  8 );
	PUT_ULONG_LE( ctx->state[3], output, 12 );
}

/*
* output = MD5( input buffer )
*/
void md5( unsigned char *input, int ilen, unsigned char output[16] )
{
	md5_context ctx;

	md5_starts( &ctx );
	md5_update( &ctx, input, ilen );
	md5_finish( &ctx, output );

	memset( &ctx, 0, sizeof( md5_context ) );
}
