//
//  ViewStack.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 27.03.2023.
//

#include "ViewStack.h"
#include "GleamTheme.h"
#include "IconsLucide.h"

#include "Renderer/RenderSystem.h"
#include "Renderer/RenderPipeline.h"
#include "Renderer/Renderers/ImGuiRenderer.h"

#include "Core/Globals.h"
#include "Core/Engine.h"

#include <imgui.h>

using namespace GEditor;

ViewStack::ViewStack(const Gleam::TString& iniFilename, float fontScale)
	: mIniPath((Gleam::Globals::UserDataDirectory / iniFilename).String())
	, mFontScale(fontScale)
{

}

void ViewStack::Initialize(Gleam::Application* app)
{
	mApplication = app;

	static auto renderSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::RenderSystem>();
	mImgui = new Gleam::ImGuiRenderer();
	mImgui->OnCreate(renderSystem->GetRenderContext());
	ImGui::GetIO().IniFilename = mIniPath.c_str();

	renderSystem->GetRenderPipeline(Gleam::RenderPath::Default)->AddSharedRenderer(mImgui);
	renderSystem->GetRenderPipeline(Gleam::RenderPath::PathTracing)->AddSharedRenderer(mImgui);
	Gleam::Theme::Apply();
	LoadFonts();
}

void ViewStack::Shutdown(Gleam::Application* app)
{
	static auto renderSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::RenderSystem>();
	renderSystem->GetRenderPipeline(Gleam::RenderPath::Default)->RemoveSharedRenderer(mImgui);
	renderSystem->GetRenderPipeline(Gleam::RenderPath::PathTracing)->RemoveSharedRenderer(mImgui);
	mImgui->OnDestroy(renderSystem->GetRenderContext());
	delete mImgui;
	
	for (int i = (int)mViews.size() - 1; i >= 0; --i)
	{
		mViews[i]->OnDestroy(mApplication);
	}
	mViews.clear();
}

void ViewStack::Tick(Gleam::Application* app)
{
	static auto renderSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::RenderSystem>();
	auto imgui = renderSystem->GetActiveRenderPipeline()->GetRenderer<Gleam::ImGuiRenderer>();

	if (mFontsDirty)
	{
		mFontsDirty = false;
		LoadFonts();
	}

    for (auto view : mViews)
    {
        view->Update();
    }

	for (auto view : mViews)
    {
        view->Render(imgui);
    }
}

void ViewStack::LoadFonts()
{
	static constexpr ImWchar kIconRanges[] = { ICON_MIN_LC, ICON_MAX_LC, 0 };
	constexpr const char* kIconFont = "Resources/Fonts/Lucide/lucide.ttf";
	constexpr const char* kRegularFont = "Resources/Fonts/IBMPlexSans/IBMPlexSans-Regular.ttf";
	constexpr const char* kMediumFont = "Resources/Fonts/IBMPlexSans/IBMPlexSans-Medium.ttf";
	constexpr const char* kSemiBoldFont = "Resources/Fonts/IBMPlexSans/IBMPlexSans-SemiBold.ttf";
	constexpr const char* kMonoFont = "Resources/Fonts/JetBrainsMono/JetBrainsMono-Regular.ttf";

	constexpr float kFontSize = 16.0f;
	auto scaled = [this](float offset)
	{
		return Gleam::Math::Round((kFontSize + offset) * mFontScale);
	};
	const float fontSize = scaled(0.0f);

	if (mIconGlyphRanges.empty())
	{
		static constexpr const char* kIcons[] = {
			ICON_LC_AXIS_3D, ICON_LC_BOX, ICON_LC_CHEVRON_DOWN, ICON_LC_CHEVRON_RIGHT, ICON_LC_ELLIPSIS_VERTICAL,
			ICON_LC_EYE, ICON_LC_EYE_OFF, ICON_LC_FOLDER, ICON_LC_FOLDER_OPEN, ICON_LC_GLOBE,
			ICON_LC_MAGNET, ICON_LC_MOVE, ICON_LC_PAUSE, ICON_LC_PLAY, ICON_LC_PLUS, ICON_LC_ROTATE_CW, ICON_LC_SCALING,
			ICON_LC_SEARCH, ICON_LC_SLIDERS_HORIZONTAL, ICON_LC_SPARKLE, ICON_LC_SQUARE, ICON_LC_STEP_FORWARD, ICON_LC_SUN, ICON_LC_UPLOAD
		};

		ImFontGlyphRangesBuilder builder;
		for (const char* icon : kIcons)
		{
			builder.AddText(icon);
		}
		builder.BuildRanges(&mIconGlyphRanges);
	}
	const ImWchar* iconGlyphs = mIconGlyphRanges.Data;

	mImgui->ClearFonts();
	mFonts.regular = mImgui->AddFont(kRegularFont, fontSize, kIconRanges);
	mImgui->MergeFont(kIconFont, fontSize, iconGlyphs);
	mFonts.compact = mImgui->AddFont(kRegularFont, scaled(-1.0f), kIconRanges);
	mFonts.medium = mImgui->AddFont(kMediumFont, fontSize, kIconRanges);
	mImgui->MergeFont(kIconFont, fontSize, iconGlyphs);
	mFonts.semiBold = mImgui->AddFont(kSemiBoldFont, fontSize, kIconRanges);
	mImgui->MergeFont(kIconFont, fontSize, iconGlyphs);
	mFonts.numeric = mImgui->AddFont(kMonoFont, fontSize);
	mFonts.label = mImgui->AddFont(kMediumFont, scaled(1.0f), kIconRanges);
	mFonts.brand = mImgui->AddFont(kSemiBoldFont, scaled(2.0f), kIconRanges);
	mFonts.heading = mImgui->AddFont(kSemiBoldFont, scaled(5.0f), kIconRanges);
	mImgui->MergeFont(kIconFont, scaled(5.0f), iconGlyphs);
	mFonts.title = mImgui->AddFont(kSemiBoldFont, scaled(10.0f), kIconRanges);
	mFonts.caption = mImgui->AddFont(kRegularFont, scaled(-3.0f), kIconRanges);
	mFonts.micro = mImgui->AddFont(kRegularFont, scaled(-4.0f), kIconRanges);
	mFonts.microBold = mImgui->AddFont(kSemiBoldFont, scaled(-4.0f), kIconRanges);
	mImgui->MergeFont(kIconFont, scaled(-4.0f), iconGlyphs);
	mFonts.footnote = mImgui->AddFont(kRegularFont, scaled(-2.0f), kIconRanges);
	mImgui->MergeFont(kIconFont, scaled(-2.0f), iconGlyphs);
	mFonts.subheading = mImgui->AddFont(kSemiBoldFont, scaled(1.0f), kIconRanges);
	mFonts.mono = mImgui->AddFont(kMonoFont, scaled(-3.0f));
	mFonts.detail = mImgui->AddFont(kMonoFont, scaled(-2.0f));
	const float iconLargeSize = Gleam::Math::Round(32.0f * mFontScale);
	mFonts.iconLarge = mImgui->AddFont(kRegularFont, iconLargeSize, kIconRanges);
	mImgui->MergeFont(kIconFont, iconLargeSize, iconGlyphs);
	ImGui::GetIO().FontDefault = mFonts.regular;
	ImGui::GetStyle().FontSizeBase = fontSize;
	mImgui->BuildFontTexture();
}
