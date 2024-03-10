#pragma once
#include <string>
#include <sstream>

namespace Gleam {

using TString = std::string;

using TWString = std::wstring;

using TStringView = std::string_view;

using TStringStream = std::stringstream;

using TWStringStream = std::wstringstream;

namespace StringUtils {

static TWString Convert(const TString& as)
{
	if (as.empty()) return TWString();

	size_t reqLength = ::MultiByteToWideChar(CP_UTF8, 0, as.c_str(), (int)as.length(), 0, 0);
	TWString ret(reqLength, L'\0');

	::MultiByteToWideChar(CP_UTF8, 0, as.c_str(), (int)as.length(), &ret[0], (int)ret.length());
	return ret;
}

} // namespace StringUtils

} // namespace Gleam
