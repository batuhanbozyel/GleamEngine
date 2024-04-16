//
//  Tuple.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 27.03.2024.
//

#pragma once
#include <tuple>

namespace Gleam {

template<class ...Ts>
using Tuple = std::tuple<Ts...>;

} // namespace Gleam
