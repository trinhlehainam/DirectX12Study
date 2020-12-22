#include "MouseEvent.h"
#include <Windows.h>

MouseEvent::MouseEvent()
{
}

MouseEvent::MouseEvent(unsigned int msg, unsigned char wparam, __int64 lparam)
{
	const POINTS p = MAKEPOINTS(lparam);
	m_x = p.x;
	m_y = p.y;
	switch (msg)
	{
	case WM_MOUSEMOVE:
		m_button = BUTTON::NONE;
		m_state = STATE::MOVE;
		break;
	case WM_LBUTTONDOWN:
		m_button = BUTTON::LEFT;
		m_state = STATE::DOWN;
		break;
	case WM_RBUTTONDOWN:
		m_button = BUTTON::RIGHT;
		m_state = STATE::DOWN;
		break;
	case WM_MBUTTONDOWN:
		m_button = BUTTON::MIDDLE;
		m_state = STATE::DOWN;
		break;
	case WM_LBUTTONUP:
		m_button = BUTTON::LEFT;
		m_state = STATE::UP;
		break;
	case WM_RBUTTONUP:
		m_button = BUTTON::RIGHT;
		m_state = STATE::UP;
		break;
	case WM_MBUTTONUP:
		m_button = BUTTON::MIDDLE;
		m_state = STATE::UP;
		break;
	case WM_MOUSEWHEEL:
		m_button = BUTTON::NONE;
		m_state = STATE::WHEEL;
		break;
	}
}

MouseEvent::~MouseEvent()
{
}
