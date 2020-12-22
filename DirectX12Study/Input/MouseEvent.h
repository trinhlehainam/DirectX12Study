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
	MouseEvent(BUTTON button, STATE state, int x, int y);
	~MouseEvent();

private:
	friend class Mouse;

	BUTTON m_button;
	STATE m_state;
	int m_x, m_y;
};

