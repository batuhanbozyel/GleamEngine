#pragma once
#include <string>
#include <sstream>
#include <locale>
#include <codecvt>
#include <bitset>
#include <algorithm>
#include <entt/core/hashed_string.hpp>

static constexpr uint32_t operator"" _hs(const char* str, size_t size)
{
    return entt::hashed_string(str);
}

namespace Gleam {

using TString = std::string;

using TWString = std::wstring;

using TStringView = std::string_view;

using TWStringView = std::wstring_view;

using TStringStream = std::stringstream;

using TWStringStream = std::wstringstream;

namespace StringUtils {

static constexpr uint32_t Hash(const char* str)
{
	return entt::hashed_string(str);
}

static constexpr uint32_t Hash(const TStringView str)
{
	return Hash(str.data());
}

static constexpr uint32_t Hash(const TString& str)
{
	return Hash(str.c_str());
}

static TString ToLower(const TString& str)
{
    TString newStr = str;
    std::transform(newStr.begin(), newStr.end(), newStr.begin(),
        [](char c){ return std::tolower(c); });
    return newStr;
}

static TString ToUpper(const TString& str)
{
    TString newStr = str;
    std::transform(newStr.begin(), newStr.end(), newStr.begin(),
        [](char c){ return std::toupper(c); });
    return newStr;
}

#pragma warning(push)
#pragma warning(disable:4996)
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

static TWString Convert(const TStringView as)
{
	if (as.empty()) return TWString();

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(as.data());
}

static TString Convert(const TWStringView as)
{
	if (as.empty()) return TString();

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.to_bytes(as.data());
}
#pragma warning(pop)

static TString Serialize(const TString& str)
{
    TString binary;
    for (char c : str)
    {
        std::bitset<8> bits(c);
        binary += bits.to_string();
    }
    return binary;
}

static TString Deserialize(const TString& binary)
{
    if (binary.length() % 8 != 0)
    {
        return "";
    }

    TString text;
    for (size_t i = 0; i < binary.length(); i += 8)
    {
        TString byte = binary.substr(i, 8);
        char c = static_cast<char>(std::bitset<8>(byte).to_ulong());
        text += c;
    }
    return text;
}

} // namespace StringUtils

} // namespace Gleam
