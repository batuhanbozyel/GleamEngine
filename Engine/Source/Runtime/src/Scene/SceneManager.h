#pragma once
#include "Scene.h"

namespace Gleam {

class SceneManager
{
public:
    
    static Scene& CreateScene();
    
    static Scene& GetActiveScene();
    
    static void SetActiveScene(uint32_t sceneIdx);
    
private:
    
    static bool UpdateHandler();
    
    static inline uint32_t mActiveSceneIdx = 0u;
    static inline TArray<Scene> mScenes;
    
};

} // namespace Gleam
