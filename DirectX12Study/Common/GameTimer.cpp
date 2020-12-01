#include "GameTimer.h"
#include <windows.h>

GameTimer::GameTimer():mSecondsPerCount(0.0),mDeltaTime(-1.0),
mBaseTime(0),mPausedTime(0),mStopTime(0),mPrevTime(0),mCurrTime(0)
{
	__int64 countsPerSecond = 0;
	QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&countsPerSecond));
	mSecondsPerCount = 1.0 / static_cast<double>(countsPerSecond);
}

float GameTimer::GameTime() const
{
	return 0.0f;
}

float GameTimer::DeltaTime() const
{
	return static_cast<float>(mDeltaTime);
}

void GameTimer::Reset()
{
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&mCurrTime));

	mBaseTime = mCurrTime;
	mPrevTime = mCurrTime;
	mStopTime = 0;
	mIsStopped = false;
}

void GameTimer::Start()
{
	// -------------||------------|>--------------->
	// Base		  Stop<---------->Start		    Current
	//					   d
	//				pausedTime += d
	if (mIsStopped)
	{
		__int64 startTime = 0;
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&startTime));

		mPausedTime += startTime - mStopTime;

		// since the timer is stopped, mPrevTime is not valid
		// set mPrevTime to startTime to start calculate deltaTime
		mPrevTime = startTime;

		mIsStopped = false;
		mStopTime = 0;
	}
}

void GameTimer::Stop()
{
	// When it's already stopped (mIsStopped = true), DO NOT THING
	// Else
	if (!mIsStopped)
	{
		mIsStopped = true;
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&mStopTime));
	}
}

void GameTimer::Tick()
{
	if (mIsStopped)
	{
		mDeltaTime = 0.0;
		return;
	}

	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&mCurrTime));
	mDeltaTime = (mCurrTime - mPrevTime) * mSecondsPerCount;
	mPrevTime = mCurrTime;

	// if the processor goes into a power save mode or we get shuffled to
	// another processor, then mDeltaTime can be NEGATIVE
	if (mDeltaTime < 0.0) mDeltaTime = 0.0;
}

float GameTimer::TotalTime() const
{
	// when the timer is stopped, DO NOT COUNT the time has passed since timer stopped

	// If timer is STOPPING
	//  *-----------------------|          |------------||-------------->
	// Base______________________paused time___________Stop--X--X--X->Current
	//				|				  			 |
	//	   [		V			 ]	   +	[	 V	   ]
	//							  Total Time
	if (mIsStopped)
	{
		return (mStopTime - mBaseTime - mPausedTime) * static_cast<float>(mSecondsPerCount);
	}
	
	// If timer is running
	//  *-----------------------|          |-------------------------->
	// Base___________________Stop<----->Start______________________Current
	//				|				  					 |
	//	   [		V		  ]			+			[	 V	   ]
	//							  Total Time
	return (mCurrTime - mBaseTime - mPausedTime) * static_cast<float>(mSecondsPerCount);
}
