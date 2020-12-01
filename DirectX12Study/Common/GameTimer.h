#pragma once
class GameTimer
{
public:
	GameTimer();

	float GameTime() const; // in seconds
	float DeltaTime() const; // in seconds

	void Reset(); // Call before message loop

	void Start();
	void Stop();

	// Process DELTA TIME when timer is running
	void Tick();

	float TotalTime() const;
private:
	double mSecondsPerCount;

	// elapsed time between CLOSEST frames
	// valid only when timer is running
	double mDeltaTime;

	__int64 mBaseTime;
	__int64 mPausedTime;
	__int64 mStopTime;
	__int64 mPrevTime;
	__int64 mCurrTime;

	bool mIsStopped = false;
};

