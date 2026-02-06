//
//  Queue.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 29.04.2023.
//

#pragma once
#include <EASTL/queue.h>
#include <EASTL/deque.h>
#include <EASTL/priority_queue.h>

namespace Gleam {

template<typename T, typename Container = eastl::deque<T, EASTLAllocatorType, DEQUE_DEFAULT_SUBARRAY_SIZE(T)>>
using Queue = eastl::queue<T, Container>;

template<typename T, typename Allocator = EASTLAllocatorType, unsigned kDequeSubarraySize = DEQUE_DEFAULT_SUBARRAY_SIZE(T)>
using Deque = eastl::deque<T, Allocator, kDequeSubarraySize>;

template <typename T, typename Container = eastl::vector<T>, typename Compare = eastl::less<typename Container::value_type>>
using PriorityQueue = eastl::priority_queue<T, Container, Compare>;

} // namespace Gleam
