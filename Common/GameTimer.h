//***************************************************************************************
// GameTimer.h by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#ifndef GAMETIMER_H
#define GAMETIMER_H

class GameTimer {
public:
	GameTimer();

	float TotalTime() const; //�� ����
	float DeltaTime() const; //�� ����
	
	void Reset(); //�޽��� ���� ������ ȣ���ؾ� ��
	void Start(); //Ÿ�̸Ӹ� ���� �Ǵ� �簳�� �� ȣ���ؾ� ��
	void Stop();  //Ÿ�̸Ӹ� ����(�Ͻ� ����)�� �� ȣ���ؾ� ��
	void Tick();  //�� ������ ȣ���ؾ� ��

private:
	double mSecondsPerCount;
	double mDeltaTime;

	__int64 mBaseTime;
	__int64 mPausedTime;
	__int64 mStopTime;
	__int64 mPrevTime;
	__int64 mCurrTime;

	bool mStopped;
};

#endif //GAMETIMER_H
