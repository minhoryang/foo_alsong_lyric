#include "stdafx.h"

#include "SoapHelper.h"
#include "Socket.h"

SoapHelper::SoapHelper()
{
	pugi::xml_node envelope = m_Document.append_child();
	envelope.set_name("SOAP-ENV:Envelope");
	envelope.append_attribute("xmlns:SOAP-ENV").set_value("http://www.w3.org/2003/05/soap-envelope");
	envelope.append_attribute("xmlns:SOAP-ENC").set_value("http://www.w3.org/2003/05/soap-encoding");
	envelope.append_attribute("xmlns:xsi").set_value("http://www.w3.org/2001/XMLSchema-instance");
	envelope.append_attribute("xmlns:xsd").set_value("http://www.w3.org/2001/XMLSchema");
	envelope.append_attribute("xmlns:ns2").set_value("ALSongWebServer/Service1Soap");
	envelope.append_attribute("xmlns:ns1").set_value("ALSongWebServer");
	envelope.append_attribute("xmlns:ns3").set_value("ALSongWebServer/Service1Soap12");
	envelope.append_child().set_name("SOAP-ENV:Body");

	m_Body = envelope.child("SOAP-ENV:Body");
}

void SoapHelper::SetMethod(const char *MethodName)
{
	pugi::xml_node method = m_Body.append_child();
	method.set_name(MethodName);
	m_Query = method.append_child();
	m_Query.set_name("ns1:stQuery");
	m_MethodName = MethodName;
}

void SoapHelper::AddParameter(const char *ParameterName, const char *value)
{
	pugi::xml_node param = m_Query.append_child();
	param.set_name(ParameterName);
	param.append_child(pugi::node_pcdata).set_value(value);
}

boost::shared_ptr<pugi::xml_document> SoapHelper::Execute()
{
	std::stringstream str;
	pugi::xml_writer_stream writer(str);
	m_Document.save(writer, "", pugi::format_raw);

	Socket s("lyrics.alsong.co.kr", 80);
	char buf[255];

	CHAR Header[] = "POST /alsongwebservice/service1.asmx HTTP/1.1\r\n"
		"Host: lyrics.alsong.co.kr\r\n"
		"User-Agent: gSOAP/2.7\r\n"
		"Content-Type: application/soap+xml; charset=utf-8\r\n"
		"Content-Length: %d\r\n"
		"Connection: close\r\n"
		"SOAPAction: \"ALSongWebServer/%s\"\r\n\r\n";

	wsprintfA(buf, Header, str.str().length(), &*(boost::find_first(m_MethodName, "ns1:")).begin() + 4);

	s.Send(buf, lstrlenA(buf));
	s.Send(str.str().c_str(), str.str().length());

	std::vector<char> data = s.ReceiveUntilEOF();

	boost::shared_ptr<pugi::xml_document> ret = boost::shared_ptr<pugi::xml_document>(new pugi::xml_document());
	ret->load(&*boost::find_first(data, "\r\n\r\n").begin());

	return ret;
}
