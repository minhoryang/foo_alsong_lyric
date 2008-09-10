class Common_Lyric_Manipulation
{
private:
	std::wstring Status;
	std::vector<std::string> Lyric;
	std::string Title;
	std::string Album;
	std::string Artist;
	std::string Registrant;
	std::vector<WORD> Time;

	static DWORD GetFileHash(unsigned char *Data, int Size, CHAR *Hash, CHAR *fmt);
	DWORD ParseLyric(CHAR *InputLyric, CHAR *Delimiter);
	DWORD DownloadLyric(CHAR *Hash);
	
	static SOCKET InitateConnect(CHAR *Address, int port);

	enum 
	{
		ERROR_CONNECTION = 1,
		ERROR_LYRIC_NOT_FOUND,
		ERROR_UNSUPPORTED_EXTENSION,
		ERROR_UNKNOWN,
	};

public:
	Common_Lyric_Manipulation();
	~Common_Lyric_Manipulation();

	static DWORD UploadLyric(CHAR *FileName, int PlayTime, int nInfo, int UploadType, std::string *Lyric, std::string *Title, std::string *Artist, std::string *Album, std::string *Registrant);
	static DWORD SearchLyricGetNext(CHAR **data, int *nInfo, std::string *Title, std::string *Artist, std::string *Album, std::string *Lyric, std::string *Registrant);
	static int SearchLyricGetCount(CHAR *Artist, CHAR *Title);
	static DWORD SearchLyric(CHAR *InArtist, CHAR *InTitle, int nPage, CHAR **Output);
	DWORD FetchLyric(BYTE *FileHead, int Size, CHAR *fmt);
	WCHAR *GetStatus();
	const char *GetLyric(DWORD Line); //Line번째 줄의 가사 얻어오기
	DWORD GetLyricTime(DWORD Line); //Line번째 줄의 시간 얻어오기
	DWORD GetNumberOfLine();
	void ClearLyric();

	void SaveToFile(WCHAR *SaveTo, CHAR *fmt);
	DWORD LoadFromFile(WCHAR *LoadFrom, CHAR *fmt);
};

void md5( unsigned char *input, int ilen, unsigned char output[16] );