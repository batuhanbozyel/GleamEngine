#pragma once
#include <string>
#include <sstream>
#include <locale>
#include <codecvt>

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
    
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(as.c_str());
}

static TString Convert(const TWString& as)
{
    if (as.empty()) return TString();
    
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(as.c_str());
}

} // namespace StringUtils

} // namespace Gleam
