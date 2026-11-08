#pragma once
#include <cstdint>


#define OBJECT_TAG_LIST(X)	\
	X(DEFAULT)				\
	X(PLAYER)				\
	X(BLOCK)				\
	X(GROUND)				\
	X(CHECKPOINT)

enum class ObjectTag : uint8_t
{
#define X(name) name,
	OBJECT_TAG_LIST(X)
#undef X
};

inline constexpr const char* objectTagNames[] = 
{
#define X(name) #name,
	OBJECT_TAG_LIST(X)
#undef X
};