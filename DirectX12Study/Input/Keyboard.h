#pragma once


class Keyboard
{
public:
	Keyboard();
	~Keyboard();
	bool IsPressed(unsigned char keycode) const;
	bool IsReleased(unsigned char keycode) const;

	void OnWindowsMessage(unsigned int msg, unsigned char keycode, __int64 lparam);
	void Update();
	// Release all keys' state when process message changes FOCUS
	// it means when messages move to another window
	// the PREVIOUS window STOP processes messages
	void Reset();
private:
	Keyboard(const Keyboard&) = delete;
	Keyboard& operator = (const Keyboard&) = delete;
	
private:
	static constexpr unsigned char num_keyboard = 255;
	bool m_keyDown[num_keyboard] = {};

	class Impl;
	Impl* mp_impl = nullptr;
};

