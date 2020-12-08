#include "Keyboard.h"
#include <windows.h>

bool Keyboard::IsPressed(unsigned char keycode) const
{
    return m_keyDown[keycode];
}

bool Keyboard::IsReleased(unsigned char keycode) const
{
    return !m_keyDown[keycode];
}

void Keyboard::OnKeyDown(unsigned char keycode)
{
    m_keyDown[keycode] = true;
}

void Keyboard::OnKeyUp(unsigned char keycode)
{
    m_keyDown[keycode] = false;
}

void Keyboard::Update()
{

}

void Keyboard::Reset()
{
    for (auto& state : m_keyDown)
        state = false;
}

