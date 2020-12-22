#pragma once

class Mouse
{
public:
	void GetPos(int& x, int& y);
	void GetPosX(int& x);
	void GetPosY(int& y);
	int GetPosX() const;
	int GetPosY() const;

	bool IsRightPressed();
	bool IsRightPressed(int& x, int& y);
	bool IsLeftPressed();
	bool IsLeftPressed(int& x, int& y);
	bool IsMiddlePressed();
	bool IsMiddlePressed(int& x, int& y);

	bool OnWindowsMessage(unsigned int msg, unsigned char wparam, __int64 lparam);
	bool ProcessMessage();
public:
	Mouse();
	~Mouse();
private:
	Mouse(const Mouse&) = delete;
	Mouse& operator = (const Mouse&) = delete;
private:
	
	class Impl;
	Impl* mp_impl = nullptr;
};

