#pragma once
#define NOMINMAX

#include <Windows.h>

inline void PrintLastErr()
{
	DWORD err = GetLastError();

	char* msgBuffer = nullptr;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		err,
		0,
		reinterpret_cast<LPSTR>(&msgBuffer),
		0,
		nullptr
	);

	if (msgBuffer)
	{
		OutputDebugStringA(msgBuffer);
		LocalFree(msgBuffer);
	}
}

#define WIN_CALL(x)					\
	{								\
	if (!(x))						\
	{								\
		PrintLastErr();				\
		__debugbreak();				\
	}								\
	}
