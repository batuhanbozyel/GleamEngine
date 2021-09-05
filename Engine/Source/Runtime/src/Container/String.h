#pragma once
#include <string>

namespace Gleam {

class String
{
public:

	String(const char* str) noexcept
		: m_Str(str)
	{

	}

	const char* Raw() const noexcept
	{
		return m_Str.c_str();
	}

	operator std::string() const noexcept
	{
		return m_Str;
	}

private:

	std::string m_Str;
};

}