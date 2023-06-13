//
//  ViewStack.h
//  Editor
//
//  Created by Batuhan Bozyel on 27.03.2023.
//

#pragma once
#include "View.h"
#include "Gleam.h"

namespace GEditor {

template <typename T>
concept ViewType = std::is_base_of<View, T>::value;

class ViewStack : public Gleam::System
{
public:
    
    virtual void OnCreate() override;

	virtual void OnDestroy() override;
    
    virtual void OnUpdate() override;
    
    template<ViewType T>
    T* AddView()
    {
        GLEAM_ASSERT(!HasView<T>(), "Editor already has the view!");
        T* view = mViews.emplace<T>();
        return view;
    }
    
    template<ViewType T>
    void RemoveView()
    {
        GLEAM_ASSERT(HasView<T>(), "Editor does not have the view!");
        T* view = mViews.get<T>();
        mViews.erase<T>();
    }
    
    template<ViewType T>
    T* GetView()
    {
        GLEAM_ASSERT(HasView<T>(), "Editor does not have the view!");
        return mViews.get<T>();
    }

    Gleam::PolyArray<View>& GetViews()
    {
        return mViews;
    }
    
	const Gleam::PolyArray<View>& GetViews() const
	{
		return mViews;
	}
    
private:
    
    template<ViewType T>
    bool HasView() const
    {
        return mViews.contains<T>();
    }
    
    Gleam::PolyArray<View> mViews;
    
};

} // namespace GEditor
