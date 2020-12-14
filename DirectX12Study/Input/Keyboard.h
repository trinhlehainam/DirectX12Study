#pragma once

class Keyboard
{
public:
	Keyboard() = default;
	bool IsPressed(unsigned char keycode) const;
	bool IsReleased(unsigned char keycode) const;
private:
	Keyboard(const Keyboard&) = delete;
	Keyboard& operator = (const Keyboard&) = delete;
private:
	friend class D3D12App;

	void OnKeyDown(unsigned char keycode);
	void OnKeyUp(unsigned char keycode);
	void Update();

	// Release all keys' state when process message changes FOCUS
	// it means when messages move to another window
	// the PREVIOUS window STOP processes messages
	void Reset();
private:
	static constexpr unsigned char num_keyboard = 255;

	bool m_keyDown[num_keyboard];
};

