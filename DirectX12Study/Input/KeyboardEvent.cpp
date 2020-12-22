#include "KeyboardEvent.h"
#include <Windows.h>

KeyboardEvent::KeyboardEvent()
{
}

KeyboardEvent::KeyboardEvent(unsigned int msg, unsigned char wparam, __int64 lparam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        // Bitwise with 30th bit of lParam to find previous key state
        if (lparam & 0x40000000)
        {

        }
        m_keycode = wparam;
        m_state = STATE::DOWN;
        break;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        m_keycode = wparam;
        m_state = STATE::UP;
        break;
    case WM_CHAR:
        m_keycode = wparam;
        m_state = STATE::CHAR;
        break;
    }
}

KeyboardEvent::~KeyboardEvent()
{
}
