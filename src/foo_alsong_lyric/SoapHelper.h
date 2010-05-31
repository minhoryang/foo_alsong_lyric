#pragma once

class SoapHelper
{
private:
	pugi::xml_document m_Document;
	pugi::xml_node m_Body;
	pugi::xml_node m_Query;
	std::string m_MethodName;
public:
	SoapHelper();
	void SetMethod(const char *MethodName);
	void AddParameter(const char *ParameterName, const char *value);

	boost::shared_ptr<pugi::xml_document> Execute();
};