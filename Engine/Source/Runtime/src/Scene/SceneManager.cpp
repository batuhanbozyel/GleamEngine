#include "gpch.h"
#include "SceneManager.h"

using namespace Gleam;

Scene& SceneManager::CreateScene()
{
    static std::once_flag flag;
    std::call_once(flag, []()
    {
        EventDispatcher<AppTickEvent>::Subscribe(EventHandlerFn(UpdateHandler));
    });
    
    return mScenes.emplace_back(Scene());
}

void SceneManager::SetActiveScene(uint32_t sceneIdx)
{
    mActiveSceneIdx = sceneIdx;
}

Scene& SceneManager::GetActiveScene()
{
    GLEAM_ASSERT(mScenes.size() > 0);
    return mScenes[mActiveSceneIdx];
}

bool SceneManager::UpdateHandler()
{
    return false;
}
