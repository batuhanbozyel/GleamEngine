//
//  Queue.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 29.04.2023.
//

#pragma once
#include <queue>
#include <deque>

namespace Gleam {

template<class T>
using Queue = std::queue<T>;

template<class T>
using Deque = std::deque<T>;

} // namespace Gleam
