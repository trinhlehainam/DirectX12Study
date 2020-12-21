#include "KeyboardEvent.h"

KeyboardEvent::KeyboardEvent()
{
}

KeyboardEvent::KeyboardEvent(unsigned char keycode, STATE state)
	:m_keycode(keycode), m_state(state)
{
}

KeyboardEvent::~KeyboardEvent()
{
}
