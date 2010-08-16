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

class SoapReceiveException : public std::exception
{
	virtual const char *what() const
	{
		return "Exception occured while receiving soap data";
	}
};

