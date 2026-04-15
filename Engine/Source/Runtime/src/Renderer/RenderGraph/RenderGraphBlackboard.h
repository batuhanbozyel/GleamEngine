//
//  RenderGraphBlackboard.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 18.04.2023.
//

#pragma once
#include "Container/Hash.h"

#include <entt/core/type_info.hpp>
#include <EASTL/any.h>

namespace Gleam {

class RenderGraphBlackboard final
{
	using Container = HashMap<uint32_t, eastl::any>;
public:

	template<typename T, class...Args>
	T& Add(Args&&... args)
	{
		GLEAM_ASSERT(!Has<T>(), "Render graph blackboard already contains the pass data!");
		return mBlackboard[entt::type_hash<T>().value()].template emplace<T>(T{std::forward<Args>(args)...});
	}
    
    template<typename T>
    T& Add(const T& passData)
    {
        GLEAM_ASSERT(!Has<T>(), "Render graph blackboard already contains the pass data!");
		return mBlackboard[entt::type_hash<T>().value()].template emplace<T>(passData);
    }
    
    template<typename T>
    T& Get()
    {
        GLEAM_ASSERT(Has<T>(), "Render graph blackboard does not contain the pass data!");
		auto it = mBlackboard.find(entt::type_hash<T>().value());
		return eastl::any_cast<T&>(it->second);
    }
    
    template<typename T>
    const T& Get() const
    {
        GLEAM_ASSERT(Has<T>(), "Render graph blackboard does not contain the pass data!");
		auto it = mBlackboard.find(entt::type_hash<T>().value());
		return eastl::any_cast<const T&>(it->second);
    }
    
    template<typename T>
    bool Has() const
    {
		return mBlackboard.find(entt::type_hash<T>().value()) != mBlackboard.end();
    }
    
private:
    
	Container mBlackboard;
    
};

} // namespace Gleam

