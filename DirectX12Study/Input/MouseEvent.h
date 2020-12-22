#pragma once

class MouseEvent
{
public:
	enum STATE
	{
		MOVE,
		DOWN,
		UP,
		WHEEL
	};

	enum BUTTON
	{
		LEFT,
		RIGHT,
		MIDDLE,
		NONE
	};

public:
	MouseEvent();
	MouseEvent(unsigned int msg, unsigned char wparam, __int64 lparam);
	~MouseEvent();

private:
	friend class Mouse;

	BUTTON m_button;
	STATE m_state;
	int m_x, m_y;
};

