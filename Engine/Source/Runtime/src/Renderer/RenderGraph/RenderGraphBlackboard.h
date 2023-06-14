//
//  RenderGraphBlackboard.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 18.04.2023.
//

#pragma once

namespace Gleam {

class RenderGraphBlackboard final
{
public:
    
    template<typename T>
    T& Add(const T& passData)
    {
        GLEAM_ASSERT(!Has<T>(), "Render graph blackboard already contains the pass data!");
        return mBlackboard.emplace<T>(passData);
    }
    
    template<typename T>
    T& Get()
    {
        GLEAM_ASSERT(Has<T>(), "Render graph blackboard does not contain the pass data!");
        return mBlackboard.get_unsafe<T>();
    }
    
    template<typename T>
    bool Has() const
    {
        return mBlackboard.contains<T>();
    }
    
private:
    
    AnyArray mBlackboard;
    
};

} // namespace Gleam

