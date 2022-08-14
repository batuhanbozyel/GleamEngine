#define GDEBUG
// EntryPoint
#include "Core/EntryPoint.h"
#include "Renderer/DebugRenderer.h"

namespace GEditor {

class SceneLayer : public Gleam::Layer
{
    virtual void OnAttach() override
    {
        mCamera.Translate({0.0f, 0.5f, 0.0f});
        
        Gleam::EventDispatcher<Gleam::MouseButtonPressedEvent>::Subscribe([&](Gleam::MouseButtonPressedEvent e) -> bool
        {
            if (e.GetMouseButton() == Gleam::MouseButton::Right)
            {
                mCursorVisible = !mCursorVisible;
                Gleam::Input::ShowCursor(mCursorVisible);
            }
			return false;
        });

		Gleam::EventDispatcher<Gleam::RendererResizeEvent>::Subscribe([&](const Gleam::RendererResizeEvent& e) -> bool
		{
			mCamera.SetViewport(e.GetWidth(), e.GetHeight());
			return false;
		});
    }
    
    virtual void OnUpdate() override
    {
		if (!Gleam::Input::GetCursorVisible())
		{
			constexpr float mouseSensitivity = 0.1f;
			mYaw += Gleam::Input::GetAxisX() * mouseSensitivity;
			mPitch += Gleam::Input::GetAxisY() * mouseSensitivity;
			mPitch = Gleam::Math::Clamp(mPitch, -80.0f, 80.0f);
			mCamera.SetRotation(Gleam::Quaternion(Gleam::Math::Deg2Rad(Gleam::Vector3{ mPitch, mYaw, 0.0f })));
		}

        constexpr float cameraSpeed = 5.0f;
		if (Gleam::Input::GetButtonDown(Gleam::KeyCode::A))
		{
			mCamera.Translate(-mCamera.RightVector() * cameraSpeed * Gleam::Time::deltaTime);
		}
		if (Gleam::Input::GetButtonDown(Gleam::KeyCode::D))
		{
			mCamera.Translate(mCamera.RightVector() * cameraSpeed * Gleam::Time::deltaTime);
		}
		if (Gleam::Input::GetButtonDown(Gleam::KeyCode::W))
		{
			mCamera.Translate(mCamera.ForwardVector() * cameraSpeed * Gleam::Time::deltaTime);
		}
		if (Gleam::Input::GetButtonDown(Gleam::KeyCode::S))
		{
			mCamera.Translate(-mCamera.ForwardVector() * cameraSpeed * Gleam::Time::deltaTime);
		}
        if (Gleam::Input::GetButtonDown(Gleam::KeyCode::Space))
        {
            mCamera.Translate(Gleam::Vector3::up * cameraSpeed * Gleam::Time::deltaTime);
        }
        if (Gleam::Input::GetButtonDown(Gleam::KeyCode::LeftControl))
        {
            mCamera.Translate(Gleam::Vector3::down * cameraSpeed * Gleam::Time::deltaTime);
        }
        mRenderer.UpdateView(mCamera);
        
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
    
    bool mCursorVisible = true;
    
    float mYaw = 0.0f;
    float mPitch = 0.0f;
    
    Gleam::World mWorld;
    Gleam::Camera mCamera;
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
