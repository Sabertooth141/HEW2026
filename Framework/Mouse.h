#pragma once
#include <optional>
#include <queue>
#include <utility>

class Mouse
{
	friend class Window;

public:
	struct RawDelta
	{
		int x, y;
	};

	class Event
	{
	public:
		enum class Type
		{
			LPressed,
			LReleased,
			RPressed,
			RReleased,
			WheelUp,
			WheelDown,
			Move,
			Enter,
			Leave
		};

	public:
		Event(Type type, const Mouse& parent) :
			type(type),
			leftPressed(parent.leftPressed),
			rightPressed(parent.rightPressed),
			x(parent.x),
			y(parent.y)
		{
		}

		Type GetType() const
		{
			return type;
		}

		std::pair<int, int> GetPos() const
		{
			return {x, y};
		}

		int GetPosX() const
		{
			return x;
		}

		int GetPosY() const
		{
			return y;
		}

		bool LeftPressed() const
		{
			return leftPressed;
		}

		bool RightPressed() const
		{
			return rightPressed;
		}

	private:
		Type type;
		int x;
		int y;
		bool leftPressed;
		bool rightPressed;
	};

public:
	Mouse() = default;
	Mouse(const Mouse&) = delete;
	Mouse& operator=(const Mouse&) = delete;

	std::pair<int, int> GetPos() const;
	std::optional<RawDelta> readRawDelta();
	int GetPosX() const;
	int GetPosY() const;
	bool IsInWindow() const;
	bool LeftPressed() const;
	bool RightPressed() const;
	std::optional<Mouse::Event> Read();
	bool IsEmpty() const;
	void Flush();
	void EnableRaw();
	void DisableRaw();
	bool RawEnabled() const;

private:
	void OnMouseMove(int inX, int inY);
	void OnMouseLeave();
	void OnMouseEnter();
	void OnRawDelta(int dx, int dy);
	void OnLeftPressed(int inX, int inY);
	void OnLeftReleased(int inX, int inY);
	void OnRightPressed(int inX, int inY);
	void OnRightReleased(int inX, int inY);
	void OnWheelUp(int inX, int inY);
	void OnWheelDown(int inX, int inY);
	void TrimBuffer();
	void TrimRawInputBuffer();
	void OnWheelDelta(int inX, int inY, int inDelta);

private:
	static constexpr unsigned int bufferSize = 16;
	int x = {};
	int y = {};
	bool leftPressed = false;
	bool rightPressed = false;
	bool isInWindow = false;
	int wheelDeltaCarry = 0;
	bool rawEnabled = false;
	std::queue<Event> buffer;
	std::queue<RawDelta> rawDeltaBuffer;
};
