#pragma once
#include <Reflection/Attribute.h>

#include <string_view>

namespace Gleam {

template<typename T>
struct TypeTag {};

template<typename T>
inline constexpr TypeTag<T> TypeOf{};

} // namespace Gleam

namespace Gleam::Reflection::Attribute {

GLEAM_ATTRIBUTE(Version)
{
    uint32_t version;

    explicit constexpr Version(uint32_t version)
        : version(version)
    {

    }
};

GLEAM_ATTRIBUTE(AssetType)
{
    static constexpr uint32_t MaxTypes = 4;

    uint32_t types[MaxTypes] = {};
    uint32_t count = 0;

    template<typename... Types>
    explicit constexpr AssetType(TypeTag<Types>...)
        : types{ HashTypeName<Types>()... }
        , count(static_cast<uint32_t>(sizeof...(Types)))
    {
        static_assert(sizeof...(Types) <= MaxTypes, "AssetType supports up to four asset types.");
    }

    template<typename T>
    static constexpr uint32_t HashTypeName()
    {
#if defined(__clang__)
        constexpr std::string_view signature = __PRETTY_FUNCTION__;
        constexpr std::string_view prefix = "T = ";
        constexpr auto begin = signature.find(prefix) + prefix.size();
        constexpr auto end = signature.find_first_of("];", begin);
        constexpr auto name = signature.substr(begin, end - begin);
#elif defined(_MSC_VER)
        constexpr std::string_view signature = __FUNCSIG__;
        constexpr std::string_view prefix = "HashTypeName<";
        constexpr auto begin = signature.find(prefix) + prefix.size();
        constexpr auto end = signature.rfind(">(");
        constexpr auto qualified = signature.substr(begin, end - begin);
        constexpr auto keyword = qualified.find(' ');
        constexpr auto name = (qualified.starts_with("struct ") or qualified.starts_with("class ") or qualified.starts_with("enum ")) ? qualified.substr(keyword + 1) : qualified;
#endif
        return ::Gleam::Reflection::Utils::HashString(name.data(), name.size());
    }
};

GLEAM_TAG_ATTRIBUTE(EntityComponent);

GLEAM_TAG_ATTRIBUTE(Serializable);

GLEAM_TAG_ATTRIBUTE(EditorOnly);

GLEAM_ATTRIBUTE(PrettyName)
{
    char name[64] = {};

	template<size_t N>
	explicit constexpr PrettyName(const char(&str)[N])
    {
		static_assert(N <= sizeof(name));
		for (size_t i = 0; i < N; ++i)
		{
			name[i] = str[i];
		}
    }
};

} // namespace Gleam::Reflection::Attribute
