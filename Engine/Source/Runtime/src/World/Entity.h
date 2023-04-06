#pragma once
#include "entt/entt.hpp"

namespace Gleam {

using Entity = entt::entity;

template<typename ... Excludes>
using Exclude = entt::exclude_t<Excludes...>;

} // namespace Gleam
