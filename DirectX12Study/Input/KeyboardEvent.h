#pragma once

class KeyboardEvent
{
	
public:
	enum STATE
	{
		UP,
		DOWN,
		CHAR
	};
public:
	~KeyboardEvent();
	KeyboardEvent();
	KeyboardEvent(unsigned int msg, unsigned char wparam, __int64 lparam);
	
private:
	friend class Keyboard;
	STATE m_state;
	unsigned char m_keycode;
};

