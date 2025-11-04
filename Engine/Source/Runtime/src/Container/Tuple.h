//
//  Tuple.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 27.03.2024.
//

#pragma once
#include <EASTL/tuple.h>

namespace Gleam {

template<class ...Ts>
using Tuple = eastl::tuple<Ts...>;

} // namespace Gleam
