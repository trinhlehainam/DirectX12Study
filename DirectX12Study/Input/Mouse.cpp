#include "Mouse.h"

Mouse::Mouse()
{
}

void Mouse::GetPos(int& x, int& y)
{
	x = m_pos[MOUSE_STATE_MOVE].x;
	y = m_pos[MOUSE_STATE_MOVE].y;
}

void Mouse::GetPosX(int& x)
{
	x = m_pos[MOUSE_STATE_MOVE].x;
}

void Mouse::GetPosY(int& y)
{
	y = m_pos[MOUSE_STATE_MOVE].y;
}

bool Mouse::IsRightPressed()
{
	return m_mouseDown[MOUSE_BUTTON_RIGHT];;
}

bool Mouse::IsRightPressed(int& x, int& y)
{
	if (!m_mouseDown[MOUSE_BUTTON_RIGHT]) return false;
	x = m_pos[MOUSE_STATE_RIGHT_DOWN].x;
	y = m_pos[MOUSE_STATE_RIGHT_DOWN].y;
	return true;
}

bool Mouse::IsLeftPressed()
{
	return m_mouseDown[MOUSE_BUTTON_LEFT];
}

bool Mouse::IsLeftPressed(int& x, int& y)
{
	if (!m_mouseDown[MOUSE_BUTTON_LEFT]) return false;
	x = m_pos[MOUSE_STATE_LEFT_DOWN].x;
	y = m_pos[MOUSE_STATE_LEFT_DOWN].y;
	return true;
}

bool Mouse::IsMiddlePressed()
{
	return m_mouseDown[MOUSE_BUTTON_LEFT];
}

bool Mouse::IsMiddlePressed(int& x, int& y)
{
	return m_mouseDown[MOUSE_BUTTON_LEFT];
}

void Mouse::OnMove(int x, int y)
{
	m_pos[MOUSE_STATE_MOVE].x = x;
	m_pos[MOUSE_STATE_MOVE].y = y;
}

void Mouse::OnRightUp(int x, int y)
{
	m_pos[MOUSE_STATE_RIGHT_UP].x = x;
	m_pos[MOUSE_STATE_RIGHT_UP].y = y;
	m_mouseDown[MOUSE_BUTTON_RIGHT] = false;
}

void Mouse::OnRightDown(int x, int y)
{
	m_pos[MOUSE_STATE_RIGHT_DOWN].x = x;
	m_pos[MOUSE_STATE_RIGHT_DOWN].y = y;
	m_mouseDown[MOUSE_BUTTON_RIGHT] = true;
}

void Mouse::OnLeftDown(int x, int y)
{
	m_pos[MOUSE_STATE_LEFT_DOWN].x = x;
	m_pos[MOUSE_STATE_LEFT_DOWN].y = y;
	m_mouseDown[MOUSE_BUTTON_LEFT] = true;
}

void Mouse::OnLeftUp(int x, int y)
{
	m_pos[MOUSE_STATE_LEFT_UP].x = x;
	m_pos[MOUSE_STATE_LEFT_UP].y = y;
	m_mouseDown[MOUSE_BUTTON_LEFT] = false;
}

void Mouse::OnMouseWheel(int x, int y, int delta)
{
	
}
