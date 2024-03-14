#pragma once
#include "entt/entt.hpp"

namespace Gleam {

using Entity = entt::entity;

template<typename ... Excludes>
using Exclude = entt::exclude_t<Excludes...>;

static constexpr Entity InvalidEntity = Entity();

} // namespace Gleam
