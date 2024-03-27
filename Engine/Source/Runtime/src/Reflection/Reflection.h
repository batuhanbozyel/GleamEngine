#pragma once
#include "Meta.h"

namespace Gleam::Reflection {

template<typename T>
static constexpr ClassDescription GetClass()
{
    auto type = refl::reflect<T>();
    return ClassDescription(type);
}

} // namespace Gleam::Reflection

#define GLEAM_TYPE_RTTI                                                                                     \
    template<> struct rtti_info__<type>                                                                     \
    {                                                                                                       \
        rtti_info__()                                                                                       \
        {                                                                                                   \
            entt::meta<type>()                                                                              \
                .type(entt::hashed_string(name.c_str()));                                                   \
        }                                                                                                   \
    };


#define GLEAM_TYPE(TypeName, ...)                                                                           \
    namespace refl_impl::metadata {                                                                         \
        using namespace ::Gleam::Reflection::Attribute;                                                     \
        template<> struct type_info__<TypeName> {                                                           \
            template<typename Unused__> struct rtti_info__{};                                               \
            REFL_DETAIL_TYPE_BODY((TypeName), __VA_ARGS__)


#define GLEAM_END                                                                                           \
            static constexpr size_t member_count{ __COUNTER__ - member_index_offset };                      \
            GLEAM_TYPE_RTTI                                                                                 \
        };                                                                                                  \
    }

#define GLEAM_FIELD REFL_FIELD
#define GLEAM_FUNC REFL_FUNC
