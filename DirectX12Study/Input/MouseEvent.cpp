#include "MouseEvent.h"

MouseEvent::MouseEvent()
{
}

MouseEvent::MouseEvent(BUTTON button, STATE state, int x, int y)
	:m_button(button), m_state(state), m_x(x), m_y(y)
{
}

MouseEvent::~MouseEvent()
{
}
