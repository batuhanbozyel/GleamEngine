//
//  ViewStack.h
//  Editor
//
//  Created by Batuhan Bozyel on 27.03.2023.
//

#pragma once
#include "View.h"
#include "EditorFonts.h"
#include "Core/Subsystem.h"
#include "Container/PolyArray.h"
#include "Container/String.h"
#include "IO/Path.h"

#include <imgui.h>

namespace GEditor {

template <typename T>
concept ViewType = std::is_base_of<View, T>::value;

class ViewStack : public Gleam::TickableGameInstanceSubsystem
{
public:

	ViewStack(const Gleam::Path& iniPath, float fontScale = 1.0f);

    virtual void Initialize(Gleam::Application* app) override;

	virtual void Shutdown(Gleam::Application* app) override;

    virtual void Tick(Gleam::Application* app) override;
	
    template<ViewType T, class...Args>
    T* AddView(Args&&... args)
    {
        GLEAM_ASSERT(!HasView<T>(), "Editor already has the view!");
        T* view = mViews.emplace_back<T>(std::forward<Args>(args)...);
		view->OnCreate(mApplication);
        return view;
    }
    
    template<ViewType T>
    void RemoveView()
    {
        GLEAM_ASSERT(HasView<T>(), "Editor does not have the view!");
        T* view = mViews.get<T>();
		view->OnDestroy(mApplication);
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

	const EditorFonts& GetFonts() const
	{
		return mFonts;
	}

	Gleam::ImGuiRenderer* GetImGuiRenderer() const
	{
		return mImgui;
	}

	void SetFontScale(float fontScale)
	{
		mFontScale = fontScale;
		mFontsDirty = true;
	}
    
private:
    
    void LoadFonts();
    
    template<ViewType T>
    bool HasView() const
    {
        return mViews.contains<T>();
    }

	Gleam::Application* mApplication = nullptr;
    
    Gleam::PolyArray<View> mViews;

	Gleam::ImGuiRenderer* mImgui = nullptr;

	EditorFonts mFonts;

	float mFontScale = 1.0f;

	bool mFontsDirty = false;

	ImVector<ImWchar> mIconGlyphRanges;

	Gleam::TString mIniPath;
    
};

} // namespace GEditor
