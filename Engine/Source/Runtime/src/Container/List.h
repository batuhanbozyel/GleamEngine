//
//  List.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 27.04.2023.
//

#pragma once
#include <EASTL/list.h>

namespace Gleam {

template<typename T, typename Allocator = eastl::allocator>
using List = eastl::list<T>;

} // namespace Gleam
