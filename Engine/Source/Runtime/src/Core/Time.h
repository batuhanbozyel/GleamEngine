#pragma once
#include <chrono>

namespace Gleam {
    
class Time
{
    using Clock = std::chrono::steady_clock;
    using Duration = std::chrono::duration<double>;
    using TimePoint = std::chrono::steady_clock::time_point;
    
    static inline TimePoint mInitialTime;
    static inline TimePoint mLastFrameTime;
    static inline Duration mFrameTime;
    
    static inline double mDeltaTime = 0.0;
    static inline double mElapsedTime = 0.0;
    static inline double mFixedTime = 0.0;
    
public:
    
    static inline double timeScale = 1.0;
    static inline double maxDeltaTime = 0.01;
    static inline double fixedDeltaTime = 1.0 / 60.0;
    
    static inline const double& time = mElapsedTime;
    static inline const double& deltaTime = mDeltaTime;
    static inline const double& fixedTime = mFixedTime;
    
    static void Reset()
    {
        mInitialTime = Clock::now();
        mLastFrameTime = mInitialTime;
        mFrameTime = Duration::zero();
        
        mDeltaTime = 0.0;
        mElapsedTime = 0.0;
        mFixedTime = 0.0;
    }
    
    static void Step()
    {
        auto currFrameTime = Clock::now();
        mFrameTime = currFrameTime - mLastFrameTime;
        mLastFrameTime = currFrameTime;
        
        mDeltaTime = Math::Min(mFrameTime.count(), maxDeltaTime);
        mElapsedTime = std::chrono::duration_cast<Duration>(mLastFrameTime - mInitialTime).count() * timeScale;
        
        EventDispatcher<AppTickEvent>::Publish(AppTickEvent());
    }
    
    static void FixedStep()
    {
        mFixedTime += fixedDeltaTime;
    }

};

} // namespace Gleam
