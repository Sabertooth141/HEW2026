#include "Keyboard.h"

bool Keyboard::KeyIsPressed(unsigned char keycode) const
{
	return keyStates[keycode];
}

bool Keyboard::KeyIsTriggered(unsigned char keycode) const
{
	return keyTriggered[keycode];
}

bool Keyboard::KeyIsReleased(unsigned char keycode) const
{
	return keyReleasedFlags[keycode];
}

void Keyboard::EndFrame()
{
	keyTriggered.reset();
	keyReleasedFlags.reset();
}

std::optional<Keyboard::Event> Keyboard::ReadKey()
{
	if (keyBuffer.empty())
	{
		return {};
	}
	Keyboard::Event e = keyBuffer.front();
	keyBuffer.pop();
	return e;
}

bool Keyboard::KeyIsEmpty() const
{
	return keyBuffer.empty();
}

void Keyboard::FlushKey()
{
	keyBuffer = std::queue<Event>();
}

std::optional<unsigned char> Keyboard::ReadChar()
{
	if (charBuffer.empty())
	{
		return {};
	}

	unsigned char charCode = charBuffer.front();
	charBuffer.pop();
	return charCode;
}

bool Keyboard::CharIsEmpty() const
{
	return charBuffer.empty();
}

void Keyboard::FlushChar()
{
	charBuffer = std::queue<unsigned char>();
}

void Keyboard::EnableAutorepeat()
{
	autorepeatEnabled = true;
}

void Keyboard::DisableAutorepeat()
{
	autorepeatEnabled = false;
}

bool Keyboard::AutorepeatIsEnabled() const
{
	return autorepeatEnabled;
}

void Keyboard::OnKeyPressed(unsigned char keycode)
{
	if (!keyStates[keycode])           // ignores autorepeat
	{
		keyTriggered[keycode] = true;
	}
	keyStates[keycode] = true;
	keyBuffer.emplace(Keyboard::Event::Type::Pressed, keycode);
	TrimBuffer(keyBuffer);
}

void Keyboard::OnKeyReleased(unsigned char keycode)
{
	if (keyStates[keycode])
	{
		keyReleasedFlags[keycode] = true;
	}
	keyStates[keycode] = false;
	keyBuffer.emplace(Keyboard::Event::Type::Released, keycode);
	TrimBuffer(keyBuffer);
}

void Keyboard::OnChar(unsigned char character)
{
	charBuffer.emplace(character);
	TrimBuffer(charBuffer);
}

void Keyboard::ClearState()
{
	keyStates.reset();
	keyTriggered.reset();
	keyReleasedFlags.reset();
}

template <typename T>
void Keyboard::TrimBuffer(std::queue<T>& buffer)
{
	while (buffer.size() > bufferSize)
	{
		buffer.pop();
	}
}
