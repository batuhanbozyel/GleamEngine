#pragma once
#include <refl.hpp>

#define GLEAM_TYPE(TypeName, ...)                                                                           \
    namespace refl_impl::metadata {                                                                         \
        using namespace ::Gleam::Reflection::Attribute;                                                     \
        template<> struct type_info__<TypeName> {                                                           \
            REFL_DETAIL_TYPE_BODY((TypeName), __VA_ARGS__)

#define GLEAM_TEMPLATE(TemplateDeclaration, TypeName, ...)                                                  \
    namespace refl_impl::metadata {                                                                         \
        using namespace ::Gleam::Reflection::Attribute;                                                     \
        template <REFL_DETAIL_GROUP TemplateDeclaration> struct type_info__<REFL_DETAIL_GROUP TypeName> {   \
        REFL_DETAIL_TYPE_BODY(TypeName, __VA_ARGS__)

#define GLEAM_END REFL_END

#define GLEAM_ENUM(name, ...) GLEAM_TYPE(name, __VA_ARGS__) GLEAM_END
#define GLEAM_FIELD REFL_FIELD
#define GLEAM_FUNC REFL_FUNC

#define GLEAM_ATTRIBUTE(tag, ...)                                                                           \
    struct AttributeBase_##tag { static constexpr auto description = AttributeDescription(#tag); };         \
    struct tag : AttributeBase_##tag, __VA_ARGS__
