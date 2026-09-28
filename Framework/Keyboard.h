#pragma once
#include <bitset>
#include <optional>
#include <queue>

class Keyboard
{
	friend class Window;
public:
	class Event
	{
	public:
		enum class Type
		{
			Pressed,
			Released
		};

	private:
		Type type;
		unsigned char code;

	public:
		Event(Type type, unsigned char code) : type(type), code(code)
		{}
		
		bool IsPressed() const
		{
			return type == Type::Pressed;
		}

		bool IsReleased() const
		{
			return type == Type::Released;
		}

		unsigned char GetCode() const
		{
			return code;
		}
	};

public:
	Keyboard() = default;
	Keyboard(const Keyboard&) = delete;
	Keyboard& operator = (const Keyboard&) = delete;

	// key events
	bool KeyIsPressed(unsigned char keycode) const;    // held
	bool KeyIsTriggered(unsigned char keycode) const;  // went down this frame
	bool KeyIsReleased(unsigned char keycode) const;   // came up this frame
	void EndFrame();                                   // once per frame, after all Update()s

	std::optional<Event> ReadKey();
	bool KeyIsEmpty() const;
	void FlushKey();

	// char events
	std::optional<unsigned char> ReadChar();
	bool CharIsEmpty() const;
	void FlushChar();

	// autorepeat control
	void EnableAutorepeat();
	void DisableAutorepeat();
	bool AutorepeatIsEnabled() const;

private:
	void OnKeyPressed(unsigned char keycode);
	void OnKeyReleased(unsigned char keycode);
	void OnChar(unsigned char character);
	void ClearState();
	template<typename T>
	static void TrimBuffer(std::queue<T>& buffer);

private:
	static constexpr unsigned int nKeys = 256;
	static constexpr unsigned int bufferSize = 16;
	bool autorepeatEnabled = false;
	std::bitset<nKeys> keyStates;
	std::bitset<nKeys> keyTriggered;
	std::bitset<nKeys> keyReleasedFlags;
	std::queue<Event> keyBuffer;
	std::queue<unsigned char> charBuffer;
};

