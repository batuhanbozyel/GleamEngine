#pragma once
#include "Meta.h"
#include "Core/Subsystem.h"

namespace Gleam::Reflection {

class System final : public Subsystem
{
public:

	virtual void Initialize() override;
    
	virtual void Shutdown() override;

private:

};

template<typename T>
constexpr ClassDescription GetClass()
{
    auto type = refl::reflect<T>();
    return ClassDescription(type);
}

} // namespace Gleam::Reflection

#define GLEAM_TYPE(TypeName, ...) \
    namespace refl_impl::metadata { \
        using namespace ::Gleam::Reflection; \
        template<> struct type_info__<TypeName> { \
        REFL_DETAIL_TYPE_BODY((TypeName), __VA_ARGS__)

#define GLEAM_TEMPLATE(TemplateDeclaration, TypeName, ...) \
    namespace refl_impl::metadata { \
        using namespace ::Gleam::Reflection; \
        template <REFL_DETAIL_GROUP TemplateDeclaration> struct type_info__<REFL_DETAIL_GROUP TypeName> { \
        REFL_DETAIL_TYPE_BODY(TypeName, __VA_ARGS__)

#define GLEAM_END REFL_END
#define GLEAM_FIELD REFL_FIELD
#define GLEAM_FUNC REFL_FUNC
