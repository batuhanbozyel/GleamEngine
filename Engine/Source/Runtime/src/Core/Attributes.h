#pragma once
#include <Reflection/Attribute.h>

namespace Gleam::Reflection::Attribute {

GLEAM_ATTRIBUTE(Version)
{
    uint32_t version;

    explicit constexpr Version(uint32_t version)
        : version(version)
    {

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
