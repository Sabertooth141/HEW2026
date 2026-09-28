#include "Mouse.h"

#include "App.h"

std::pair<int, int> Mouse::GetPos() const
{
	return {x, y};
}

std::optional<Mouse::RawDelta> Mouse::readRawDelta()
{
	if (rawDeltaBuffer.empty())
	{
		return std::nullopt;
	}
	const RawDelta d = rawDeltaBuffer.front();
	rawDeltaBuffer.pop();
	return d;
}

int Mouse::GetPosX() const
{
	return x;
}

int Mouse::GetPosY() const
{
	return y;
}

bool Mouse::IsInWindow() const
{
	return isInWindow;
}

bool Mouse::LeftPressed() const
{
	return leftPressed;
}

bool Mouse::RightPressed() const
{
	return rightPressed;
}

std::optional<Mouse::Event> Mouse::Read()
{
	if (buffer.empty())
	{
		return {};
	}
	Mouse::Event e = buffer.front();
	buffer.pop();
	return e;
}

bool Mouse::IsEmpty() const
{
	return buffer.empty();
}

void Mouse::Flush()
{
	buffer = std::queue<Event>();
}

void Mouse::EnableRaw()
{
	rawEnabled = true;
}

void Mouse::DisableRaw()
{
	rawEnabled = false;
}

bool Mouse::RawEnabled() const
{
	return rawEnabled;
}

void Mouse::OnMouseMove(int inX, int inY)
{
	x = inX;
	y = inY;

	buffer.emplace(Mouse::Event::Type::Move, *this);
	TrimBuffer();
}

void Mouse::OnMouseLeave()
{
	isInWindow = false;
	buffer.emplace(Mouse::Event::Type::Leave, *this);
	TrimBuffer();
}

void Mouse::OnMouseEnter()
{
	isInWindow = true;
	buffer.emplace(Mouse::Event::Type::Enter, *this);
	TrimBuffer();
}

void Mouse::OnRawDelta(int dx, int dy)
{
	rawDeltaBuffer.emplace(dx, dy);
	TrimBuffer();
}

auto Mouse::OnLeftPressed(int inX, int inY) -> void
{
	leftPressed = true;

	buffer.emplace(Mouse::Event::Type::LPressed, *this);
	TrimBuffer();
}

void Mouse::OnLeftReleased(int inX, int inY)
{
	leftPressed = false;

	buffer.emplace(Mouse::Event::Type::LReleased, *this);
	TrimBuffer();
}

void Mouse::OnRightPressed(int inX, int inY)
{
	rightPressed = true;

	buffer.emplace(Mouse::Event::Type::RPressed, *this);
	TrimBuffer();
}

void Mouse::OnRightReleased(int inX, int inY)
{
	rightPressed = false;

	buffer.emplace(Mouse::Event::Type::RReleased, *this);
	TrimBuffer();
}

void Mouse::OnWheelUp(int inX, int inY)
{
	buffer.emplace(Mouse::Event::Type::WheelUp, *this);
	TrimBuffer();
}

void Mouse::OnWheelDown(int inX, int inY)
{
	buffer.emplace(Mouse::Event::Type::WheelDown, *this);
	TrimBuffer();
}

void Mouse::TrimBuffer()
{
	while (buffer.size() > bufferSize)
	{
		buffer.pop();
	}
}

void Mouse::TrimRawInputBuffer()
{
	while (rawDeltaBuffer.size() > bufferSize)
	{
		buffer.pop();
	}
}

void Mouse::OnWheelDelta(int inX, int inY, int inDelta)
{
	wheelDeltaCarry += inDelta;
	while (wheelDeltaCarry >= WHEEL_DELTA)
	{
		wheelDeltaCarry -= WHEEL_DELTA;
		OnWheelUp(inX, inY);
	}

	while (wheelDeltaCarry <= -WHEEL_DELTA)
	{
		wheelDeltaCarry += WHEEL_DELTA;
		OnWheelDown(inX, inY);
	}

}
