#include "Keyboard.h"
#include <windows.h>
#include <vector>
#include "KeyboardEvent.h"

#define IMPL (*mp_impl)

class Keyboard::Impl
{
    friend Keyboard;
public:
    Impl();
    ~Impl();
private:
    uint32_t m_head = 0;
    uint32_t m_tail = 0;
    static constexpr uint32_t MAX_NUM_EVENT = 5;
    std::vector<KeyboardEvent> m_events;
};

Keyboard::Impl::Impl()
{
    m_events.resize(MAX_NUM_EVENT);
}

Keyboard::Impl::~Impl()
{

}

Keyboard::Keyboard():mp_impl(new Impl())
{

}

Keyboard::~Keyboard()
{
    delete mp_impl;
    mp_impl = nullptr;
}

bool Keyboard::IsPressed(unsigned char keycode) const
{
    return m_keyDown[keycode];
}

bool Keyboard::IsReleased(unsigned char keycode) const
{
    return !m_keyDown[keycode];
}

void Keyboard::OnWindowsMessage(unsigned int msg, unsigned char keycode, __int64 lparam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        // Bitwise with 30th bit of lParam to find previous key state
        if (lparam & 0x40000000)
        {

        }
        IMPL.m_events[IMPL.m_tail] = KeyboardEvent(keycode, KeyboardEvent::DOWN);
        break;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        IMPL.m_events[IMPL.m_tail] = KeyboardEvent(keycode, KeyboardEvent::UP);
        break;
    case WM_CHAR:
        IMPL.m_events[IMPL.m_tail] = KeyboardEvent(keycode, KeyboardEvent::CHAR);
        break;
    }
    IMPL.m_tail = (IMPL.m_tail + 1) % IMPL.MAX_NUM_EVENT;
}

void Keyboard::Update()
{
    if (IMPL.m_head == IMPL.m_tail) return;
    while (IMPL.m_head != IMPL.m_tail)
    {
        const auto& event = IMPL.m_events[IMPL.m_head];
        switch (event.m_state)
        {
        case KeyboardEvent::DOWN:
            m_keyDown[event.m_keycode] = true;
            break;
        case KeyboardEvent::UP:
            m_keyDown[event.m_keycode] = false;
            break;
        case KeyboardEvent::CHAR:
            break;
        }
        IMPL.m_head = (IMPL.m_head + 1) % IMPL.MAX_NUM_EVENT;
    }
}

void Keyboard::Reset()
{
    for (auto& state : m_keyDown)
        state = false;
}

