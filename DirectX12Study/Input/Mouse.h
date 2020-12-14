#pragma once

class Mouse
{
public:
	enum
	{
		MOUSE_BUTTON_LEFT = 1,
		MOUSE_BUTTON_RIGHT = 2,
		MOUSE_BUTTON_MIDDLE = 3
	};
public:
	Mouse();

	void GetPos(int& x, int& y);
	void GetPosX(int& x);
	void GetPosY(int& y);

	bool IsRightPressed();
	bool IsRightPressed(int& x, int& y);
	bool IsLeftPressed();
	bool IsLeftPressed(int& x, int& y);
	bool IsMiddlePressed();
	bool IsMiddlePressed(int& x, int& y);

private:
	Mouse(const Mouse&) = delete;
	Mouse& operator = (const Mouse&) = delete;
private:
	friend class D3D12App;
	void OnMove(int x, int y);
	void OnRightDown(int x, int y);
	void OnRightUp(int x, int y);
	void OnLeftDown(int x, int y);
	void OnLeftUp(int x, int y);
	void OnMouseWheel(int x, int y, int delta);
private:
	// 0 : left , 1 : right, 2: middle
	static constexpr unsigned char num_mouse_button = 3;
	bool m_mouseDown[num_mouse_button];

	enum
	{
		MOUSE_STATE_MOVE = 0,
		MOUSE_STATE_LEFT_DOWN = 1,
		MOUSE_STATE_LEFT_UP = 2,
		MOUSE_STATE_RIGHT_DOWN = 3,
		MOUSE_STATE_RIGHT_UP = 4
	};
	struct Pos
	{
		int x, y;
	};
	// 0 : move, 
	// 1 : right pressed, 2 : right released
	// 3 : left pressed, 4 : left released
	static constexpr unsigned char num_mouse_state = 5;
	Pos m_pos[num_mouse_state];
	
	int m_delta;
};

