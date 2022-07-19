#define GDEBUG
// EntryPoint
#include "Core/EntryPoint.h"
#include "Renderer/DebugRenderer.h"

namespace GEditor {

class SceneLayer : public Gleam::Layer
{
    virtual void OnUpdate() override
    {
        Gleam::Camera camera(ApplicationInstance.GetActiveWindow().GetProperties().display.width,
                             ApplicationInstance.GetActiveWindow().GetProperties().display.height);
        camera.Translate({0.0f, -0.5f, -1.0f});
        mRenderer.UpdateView(camera);
        
        constexpr int gridWidth = 32;
        constexpr int gridHeight = 32;
        for (int i = 0; i < gridWidth; i++)
        {
            for (int j = 0; j < gridHeight; j++)
            {
                mRenderer.DrawQuad({float(i - gridWidth / 2), 0.0f, float(j - gridHeight / 2)}, 1.0f, 1.0f, Gleam::Color::HSVToRGB(Gleam::Time::time, 1.0f, 1.0f));
            }
        }
    }
    
    virtual void OnRender() override
    {
        mRenderer.Render();
    }

private:
    
    Gleam::World mWorld;
    Gleam::DebugRenderer mRenderer;
};
    
class GleamEditor : public Gleam::Application, public Gleam::Layer
{
public:

	GleamEditor(const Gleam::ApplicationProperties& props)
		: Gleam::Application(props), mSceneLayer(Gleam::CreateRef<SceneLayer>())
	{
        PushLayer(mSceneLayer);
	}

	~GleamEditor()
	{
        RemoveLayer(mSceneLayer);
	}

private:
    
    Gleam::RefCounted<SceneLayer> mSceneLayer;

};

} // namespace GEditor

Gleam::Application* Gleam::CreateApplication()
{
	Gleam::ApplicationProperties props;
	props.appVersion = Gleam::Version(1, 0, 0);
	props.windowProps.title = "Gleam Editor";
	props.windowProps.windowFlag = Gleam::WindowFlag::MaximizedWindow;
	return new GEditor::GleamEditor(props);
}
