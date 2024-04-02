#pragma once
#include <type_traits>

namespace Gleam::Reflection::Traits {

// is primitive
template<typename T>
using IsPrimitive = std::is_fundamental<T>;

// is array
template <class T> struct IsArray : public std::false_type {};
template <class T> struct IsArray<T[]> : public std::true_type {};
template <class T> struct IsArray<std::vector<T>> : public std::true_type {};
template <class T, size_t N> struct IsArray<T[N]> : public std::true_type {};
template <class T, size_t N> struct IsArray<std::array<T, N>> : public std::true_type {};

// is enum
template<typename T>
using IsEnum = std::is_enum<T>;

// is class
template<typename T>
using IsClass = std::is_class<T>;

// is template
template<class Instance> struct IsTemplate : std::false_type {};
template<class ... TemplateArgs, template<class...> class Template>
struct IsTemplate<Template<TemplateArgs...>> : std::true_type {};

// is instance of template
template<template<class...> class Template, class Instance>
struct IsInstanceOfTemplate : std::false_type {};
template<class ...TemplateArgs, template<class...> class Template>
struct IsInstanceOfTemplate<Template, Template<TemplateArgs...> > : std::true_type {};

namespace Detail {

template<template<class...> class Template, class ...TemplateArgs>
std::true_type IsBaseChecker(const volatile Template<TemplateArgs...>*);
template<template<class...> class>
std::false_type IsBaseChecker(const volatile void*);

// Additional checker is needed to avoid hard compilation error in case of private inheritance
template<template<class...> class Template, class Instance, class = void>
struct IsBaseCheckerHelper : std::true_type {};

template<template<class...> class Template, class Instance>
struct IsBaseCheckerHelper<Template, Instance, 
    std::void_t<decltype(IsBaseChecker<Template>(std::declval<Instance*>()))>
> : decltype(IsBaseChecker<Template>(std::declval<Instance*>())) {};

} // namespace Detail

// is derived of template
template<template<class...> class Template, class Instance>
struct IsDerivedOfTemplate : Detail::IsBaseCheckerHelper<Template, Instance> {};

} // namespace Gleam::Reflection::Traits
