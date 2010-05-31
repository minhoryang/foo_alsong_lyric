#pragma once

#include <exception>

class Socket
{
private:
	SOCKET m_Socket;
public:
	Socket(const char *address, int port);
	~Socket();

	std::vector<char> ReceiveUntilEOF();
	int Send(const char *buf, int len);
};

class SocketTransferException : public std::exception
{
	virtual const char *what() const
	{
		return "Exception occured while transferring from network";
	}
};
