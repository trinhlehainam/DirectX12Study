#pragma once
class Keyboard
{
public:
	bool IsPressed(unsigned char keycode) const;
	bool IsReleased(unsigned char keycode) const;

	void OnWindowsMessage(unsigned int msg, unsigned char keycode, __int64 lparam);
	void ProcessMessage();
	// Release all keys' state when process message changes FOCUS
	// it means when messages move to another window
	// the PREVIOUS window STOP processes messages
	void Reset();
public:
	Keyboard();
	~Keyboard();
private:
	// Prevent copy semantics
	Keyboard(const Keyboard&);
	void operator = (const Keyboard&);
private:
	class Impl;
	Impl* mp_impl = nullptr;
};

