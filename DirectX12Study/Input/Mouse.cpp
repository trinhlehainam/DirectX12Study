#include "Mouse.h"
#include <Windows.h>
#include <vector>
#include "MouseEvent.h"

#define IMPL (*mp_impl)

enum
{
	MOUSE_BUTTON_LEFT = 0,
	MOUSE_BUTTON_RIGHT = 1,
	MOUSE_BUTTON_MIDDLE = 2
};

class Mouse::Impl
{
public:
	Impl();
	~Impl();
private:
	friend Mouse;

	// 0 : left , 1 : right, 2: middle
	static constexpr unsigned char num_mouse_button = 3;
	bool m_mouseDown[num_mouse_button];

	int m_x, m_y;
	int m_delta;
private:
	static constexpr uint8_t MAX_NUM_EVENT = 10;
	std::vector<MouseEvent> m_events;
	uint8_t m_head = 0;
	uint8_t m_tail = 0;
};

Mouse::Impl::Impl()
{
	m_events.resize(MAX_NUM_EVENT);
}

Mouse::Impl::~Impl()
{

}


//
/***********MOUSE PUBLIC METHOD************/
//

Mouse::Mouse():mp_impl(new Impl())
{
}

Mouse::~Mouse()
{
	if (mp_impl != nullptr)
	{
		delete mp_impl;
		mp_impl = nullptr;
	}
}

void Mouse::GetPos(int& x, int& y)
{
	x = IMPL.m_x;
	y = IMPL.m_y;
}

void Mouse::GetPosX(int& x)
{
	x = IMPL.m_x;
}

void Mouse::GetPosY(int& y)
{
	y = IMPL.m_y;
}

int Mouse::GetPosX() const
{
	return IMPL.m_x;
}

int Mouse::GetPosY() const
{
	return IMPL.m_y;
}

bool Mouse::IsRightPressed()
{
	return IMPL.m_mouseDown[MOUSE_BUTTON_RIGHT];;
}

bool Mouse::IsRightPressed(int& x, int& y)
{
	if (!IMPL.m_mouseDown[MOUSE_BUTTON_RIGHT]) return false;
	x = IMPL.m_x;
	y = IMPL.m_y;
	return true;
}

bool Mouse::IsLeftPressed()
{
	return IMPL.m_mouseDown[MOUSE_BUTTON_LEFT];
}

bool Mouse::IsLeftPressed(int& x, int& y)
{
	if (!IMPL.m_mouseDown[MOUSE_BUTTON_LEFT]) return false;
	x = IMPL.m_x;
	y = IMPL.m_y;
	return true;
}

bool Mouse::IsMiddlePressed()
{
	return IMPL.m_mouseDown[MOUSE_BUTTON_MIDDLE];
}

bool Mouse::IsMiddlePressed(int& x, int& y)
{
	return IMPL.m_mouseDown[MOUSE_BUTTON_MIDDLE];
}

bool Mouse::OnWindowsMessage(unsigned int msg, unsigned char wparam, __int64 lparam)
{
	const POINTS p = MAKEPOINTS(lparam);
	switch (msg)
	{
	case WM_MOUSEMOVE:
		IMPL.m_events[IMPL.m_tail] = 
			MouseEvent(MouseEvent::BUTTON::NONE, MouseEvent::STATE::MOVE, p.x, p.y);
		break;
	case WM_LBUTTONDOWN:
		IMPL.m_events[IMPL.m_tail] = 
			MouseEvent(MouseEvent::BUTTON::LEFT, MouseEvent::STATE::DOWN, p.x, p.y);
		break;
	case WM_RBUTTONDOWN:
		IMPL.m_events[IMPL.m_tail] = 
			MouseEvent(MouseEvent::BUTTON::RIGHT, MouseEvent::STATE::DOWN, p.x, p.y);
		break;
	case WM_MBUTTONDOWN:
		IMPL.m_events[IMPL.m_tail] = 
			MouseEvent(MouseEvent::BUTTON::MIDDLE, MouseEvent::STATE::DOWN, p.x, p.y);
		break;
	case WM_LBUTTONUP:
		IMPL.m_events[IMPL.m_tail] = 
			MouseEvent(MouseEvent::BUTTON::LEFT, MouseEvent::STATE::UP, p.x, p.y);
		break;
	case WM_RBUTTONUP:
		IMPL.m_events[IMPL.m_tail] =
			MouseEvent(MouseEvent::BUTTON::RIGHT, MouseEvent::STATE::UP, p.x, p.y);
		break;
	case WM_MBUTTONUP:
		IMPL.m_events[IMPL.m_tail] = 
			MouseEvent(MouseEvent::BUTTON::MIDDLE, MouseEvent::STATE::UP, p.x, p.y);
		break;
	case WM_MOUSEWHEEL:
		IMPL.m_events[IMPL.m_tail] = 
			MouseEvent(MouseEvent::BUTTON::NONE, MouseEvent::STATE::WHEEL, p.x, p.y);
		break;
	}

	IMPL.m_tail = (IMPL.m_tail + 1) % IMPL.MAX_NUM_EVENT;
	return true;
}

bool Mouse::ProcessMessage()
{
	if (IMPL.m_head == IMPL.m_tail) return false;

	while (IMPL.m_head != IMPL.m_tail)
	{
		const auto& event = IMPL.m_events[IMPL.m_head];

		IMPL.m_x = event.m_x;
		IMPL.m_y = event.m_y;

		switch (event.m_state)
		{
		case MouseEvent::STATE::MOVE:
			break;
		case MouseEvent::STATE::DOWN:
			switch (event.m_button)
			{
			case MouseEvent::BUTTON::LEFT:
				IMPL.m_mouseDown[MOUSE_BUTTON_LEFT] = true;
				break;
			case MouseEvent::BUTTON::RIGHT:
				IMPL.m_mouseDown[MOUSE_BUTTON_RIGHT] = true;
				break;
			case MouseEvent::BUTTON::MIDDLE:
				IMPL.m_mouseDown[MOUSE_BUTTON_MIDDLE] = true;
				break;
			}
			break;
		case MouseEvent::STATE::UP:
			switch (event.m_button)
			{
			case MouseEvent::BUTTON::LEFT:
				IMPL.m_mouseDown[MOUSE_BUTTON_LEFT] = false;
				break;
			case MouseEvent::BUTTON::RIGHT:
				IMPL.m_mouseDown[MOUSE_BUTTON_RIGHT] = false;
				break;
			case MouseEvent::BUTTON::MIDDLE:
				IMPL.m_mouseDown[MOUSE_BUTTON_MIDDLE] = false;
				break;
			}
			break;
		case MouseEvent::STATE::WHEEL:
			break;
		}

		IMPL.m_head = (IMPL.m_head + 1) % IMPL.MAX_NUM_EVENT;
	}

	return true;
}

