//
//  Stack.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 24.03.2023.
//

#pragma once
#include <EASTL/stack.h>

namespace Gleam {

template<typename T, typename Container = eastl::vector<T>>
using Stack = eastl::stack<T>;

} // namespace Gleam
