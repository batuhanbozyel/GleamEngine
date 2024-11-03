#pragma once
#include <chrono>

namespace Gleam {
    
class Timestep
{
    using Clock = std::chrono::steady_clock;
    using Duration = std::chrono::duration<double>;
    using TimePoint = std::chrono::steady_clock::time_point;
    
    static inline TimePoint mInitialTime;
    static inline TimePoint mLastFrameTime;
    
    static inline double mFixedTime = 0.0;
	static inline double mAccumulator = 0.0;
	static inline double mElapsedTime = 0.0;
	static inline double mDeltaTime = 0.0;
	static inline double mAlpha = 0.0;
    
public:
	static inline double maxDeltaTime = 0.25;
    static inline double fixedDeltaTime = 1.0 / 60.0;
    static inline double timeScale = 1.0;

	static inline const double& elapsedTime = mElapsedTime;
    static inline const double& fixedTime = mFixedTime;
    static inline const double& deltaTime = mDeltaTime;
	static inline const double& alpha = mAlpha;
    
    static void Reset()
    {
        mInitialTime = Clock::now();
        mLastFrameTime = mInitialTime;

        mFixedTime = 0.0;
		mAccumulator = 0.0;
		mElapsedTime = 0.0;
		mDeltaTime = 0.0;
		mAlpha = 0.0;
    }
    
    static void Step()
    {
        auto currFrameTime = Clock::now();
		mDeltaTime = Duration(currFrameTime - mLastFrameTime).count();
        mLastFrameTime = currFrameTime;

		if (mDeltaTime > maxDeltaTime)
		{
			mDeltaTime = maxDeltaTime;
		}
		mDeltaTime *= timeScale;

		mAccumulator += mDeltaTime;
		mElapsedTime += mDeltaTime;
    }

	static void FixedStep()
	{
		mFixedTime += fixedDeltaTime;
		mAccumulator -= fixedDeltaTime;
	}

	static void Update()
	{
		mAlpha = mAccumulator / fixedDeltaTime;
	}

	static bool InFixedTimeStep()
	{
		return mAccumulator >= fixedDeltaTime;
	}

};

} // namespace Gleam
