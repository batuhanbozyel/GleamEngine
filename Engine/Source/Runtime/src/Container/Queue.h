//
//  Queue.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 29.04.2023.
//

#pragma once
#include <EASTL/queue.h>
#include <EASTL/deque.h>

namespace Gleam {

template<typename T, typename Container = eastl::deque<T, EASTLAllocatorType, DEQUE_DEFAULT_SUBARRAY_SIZE(T)>>
using Queue = eastl::queue<T>;

template<typename T, typename Allocator = EASTLAllocatorType, unsigned kDequeSubarraySize = DEQUE_DEFAULT_SUBARRAY_SIZE(T)>
using Deque = eastl::deque<T>;

} // namespace Gleam
