#include "SpriteAnimatorComponent.h"
#include "SpriteRendererComponent.h"
#include <iostream>

SpriteAnimatorComponent::SpriteAnimatorComponent(SpriteRendererComponent& sr, int cols, int rows, int totalFrames) :
	srComp(&sr),
	spriteSheetW(1.f), spriteSheetH(1.f),
	cols(cols), rows(rows),
	startFrame(0),
	endFrame(totalFrames - 1),
	currFrame(0),
	frameDuration(0.1f),
	timer(0), loop(true)
{
	loopStartFrame = startFrame;
	loopEndFrame = endFrame;
	frameW = srComp->GetTexture().GetTextureSize().x;
	frameH = srComp->GetTexture().GetTextureSize().y;
	PushUV();
}

SpriteAnimatorComponent::SpriteAnimatorComponent(SpriteRendererComponent& sr, const std::wstring& jsonPath) :
	srComp(&sr),
	spriteSheetW(1.f), spriteSheetH(1.f),
	cols(0), rows(0),
	startFrame(0),
	endFrame(0),
	currFrame(0),
	frameDuration(0.1f),
	timer(0), loop(true)
{
	loopStartFrame = startFrame;
	loopEndFrame = endFrame;
	ParseJson(ReadJson(jsonPath));
	endFrame = static_cast<int>(animInfo.size()) - 1;
	if (loopEndFrame == 0)
	{
		loopEndFrame = endFrame;
	}
	PushUV();
}

void SpriteAnimatorComponent::Update(float deltaTime)
{
	timer += deltaTime;
	if (timer >= frameDuration)
	{
		timer -= frameDuration;
		AdvanceFrame();
	}
}

void SpriteAnimatorComponent::SetAnimation(int inStartFrame, int inEndFrame, float fps, bool inLoop)
{
	if (startFrame == inStartFrame && endFrame == inEndFrame)
	{
		return;
	}
	startFrame = inStartFrame;
	endFrame = inEndFrame;
	currFrame = startFrame;
	frameDuration = 1.f / fps;
	timer = 0.f;
	loop = inLoop;
	PushUV();
}

void SpriteAnimatorComponent::AdvanceFrame()
{
	if (++currFrame > loopEndFrame)
	{
		currFrame = loop ? loopStartFrame : endFrame;
	}
	if (!animInfo.empty() && currFrame < static_cast<int>(animInfo.size()))
	{
		if (animInfo[currFrame].duration > 0.f)
		{
			frameDuration = animInfo[currFrame].duration;
		}
	}
	PushUV();
}

void SpriteAnimatorComponent::PushUV()
{
	if (!animInfo.empty() && currFrame < static_cast<int>(animInfo.size()))
	{
		const SpriteFrame& frame = animInfo[currFrame];
		float u = static_cast<float>(frame.x) / spriteSheetW;
		float v = static_cast<float>(frame.y) / spriteSheetH;
		float uvW = static_cast<float>(frame.w) / spriteSheetW;
		float uvH = static_cast<float>(frame.h) / spriteSheetH;
		srComp->UpdateUV({{u, v}, {uvW, uvH}});
	}
	else if (animInfo.empty())
	{
		float uvW = 1.f / static_cast<float>(cols);
		float uvH = 1.f / static_cast<float>(rows);
		int col = currFrame % cols;
		int row = currFrame / cols;
		srComp->UpdateUV({{col * uvW, row * uvH}, {uvW, uvH}});
	}
}

// ---------HELPERS---------

bool SpriteAnimatorComponent::ParseJson(const std::string& jsonString)
{
	if (jsonString.empty())
	{
		return false;
	}

	size_t pos = jsonString.find("\"frames\"");
	if (pos == std::string::npos)
	{
		return false;
	}

	while (true)
	{
		const size_t framePos = jsonString.find("\"frame\":", pos + 1);
		if (framePos == std::string::npos)
		{
			break;
		}

		const size_t frameInfoStart = jsonString.find('{', framePos);
		const size_t frameInfoEnd = jsonString.find('}', frameInfoStart);
		if (frameInfoStart == std::string::npos || frameInfoEnd == std::string::npos)
		{
			break;
		}

		std::string frameInfo = jsonString.substr(frameInfoStart, frameInfoEnd - frameInfoStart + 1);

		SpriteFrame frame;
		frame.x = ParseInt(frameInfo, "x");
		frame.y = ParseInt(frameInfo, "y");
		frame.w = ParseInt(frameInfo, "w");
		frame.h = ParseInt(frameInfo, "h");

		if (frameW != static_cast<float>(frame.w))
		{
			frameW = static_cast<float>(frame.w);
		}

		if (frameH != static_cast<float>(frame.h))
		{
			frameH = static_cast<float>(frame.h);
		}

		const size_t durationPos = jsonString.find("\"duration\":", frameInfoEnd);
		const size_t nextFramePos = jsonString.find("\"frame\":", frameInfoEnd);

		if (durationPos != std::string::npos && (nextFramePos == std::string::npos || durationPos < nextFramePos))
		{
			const int durationMs = ParseInt(jsonString.substr(durationPos, 30), "duration");
			frame.duration = static_cast<float>(durationMs) / 1000.0f;
		}
		else
		{
			frame.duration = 0.1f;
		}

		animInfo.push_back(frame);
		pos = frameInfoEnd;
	}

	const size_t metaPos = jsonString.find("\"meta\"", pos);
	const size_t sheetSizePos = jsonString.find("\"size\"", metaPos);
	if (sheetSizePos == std::string::npos)
	{
		return false;
	}
	const size_t sheetSizeEndPos = jsonString.find('}', sheetSizePos);
	if (sheetSizeEndPos == std::string::npos)
	{
		return false;
	}

	const std::string sheetSizeInfo = jsonString.substr(sheetSizePos, sheetSizeEndPos - sheetSizePos + 1);
	spriteSheetW = static_cast<float>(ParseInt(sheetSizeInfo, "w"));
	spriteSheetH = static_cast<float>(ParseInt(sheetSizeInfo, "h"));

	// find loop info
	const size_t loopInfoPos = jsonString.find("\"Loop\"", sheetSizeEndPos);
	if (loopInfoPos != std::string::npos)
	{
		const size_t loopFromPos = jsonString.find("\"from\"", loopInfoPos);
		loopStartFrame = ParseInt(jsonString.substr(loopFromPos, 30), "from");
		const size_t loopToPos = jsonString.find("\"to\"", loopFromPos);
		loopEndFrame = ParseInt(jsonString.substr(loopToPos, 30), "to");
	}

	return !animInfo.empty();
}

SpriteRendererComponent* SpriteAnimatorComponent::GetSpriteRenderer() const
{
	return srComp;
}

std::string SpriteAnimatorComponent::ReadJson(const std::wstring& jsonPath)
{
	HANDLE hFile = CreateFileW(
		jsonPath.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		throw std::runtime_error("Failed to open file");
	}

	LARGE_INTEGER fileSize;
	if (!GetFileSizeEx(hFile, &fileSize))
	{
		CloseHandle(hFile);
		throw std::runtime_error("Failed to get file size");
	}

	if (fileSize.QuadPart > MAXDWORD)
	{
		CloseHandle(hFile);
		throw std::runtime_error("File is too large");
	}

	std::string buffer;

	buffer.resize(static_cast<size_t>(fileSize.QuadPart));

	DWORD bytesRead = 0;
	if (!ReadFile(hFile, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr))
	{
		CloseHandle(hFile);
		throw std::runtime_error("Failed to read file");
	}

	CloseHandle(hFile);

	buffer.resize(bytesRead);
	return buffer;
}

int SpriteAnimatorComponent::ParseInt(const std::string& json, const std::string& key)
{
	const std::string searchKey = "\"" + key + "\"";
	size_t pos = json.find(searchKey);

	if (pos == std::string::npos)
	{
		return -1;
	}

	pos += searchKey.length();
	while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t'))
	{
		pos++;
	}

	if (pos < json.length() && json[pos] == ':')
	{
		pos++;
	}

	while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t'))
	{
		pos++;
	}

	const char* valFound = json.c_str() + pos;
	char* endPtr;
	errno = 0;

	const auto result = static_cast<int>(std::strtol(valFound, &endPtr, 10));

	if (endPtr == valFound)
	{
		std::cerr << "Failed to parse int: " << valFound << ", no digits found" << std::endl;
		return 0;
	}
	if (errno == ERANGE)
	{
		std::cerr << "Failed to parse int: " << valFound << ", value out of range for a long int" << std::endl;
		return 0;
	}

	return result;
}
