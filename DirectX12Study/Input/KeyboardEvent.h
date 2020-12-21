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
	KeyboardEvent(unsigned char keycode, STATE state);
	
private:
	friend class Keyboard;
	STATE m_state;
	unsigned char m_keycode;
};

