#pragma once
#include <DirectXMath.h>
#include <string>
#include <vector>

#include "IComponent.h"

class SpriteRendererComponent;

struct SpriteFrame
{
	int w, h;
	int x, y;
	float duration;
};

class SpriteAnimatorComponent : public IComponent
{
public:
	SpriteAnimatorComponent(SpriteRendererComponent& sr, int cols, int rows, int totalFrames);
	SpriteAnimatorComponent(SpriteRendererComponent& sr, const std::wstring& jsonPath);
	void Update(float deltaTime) override;
	void SetAnimation(int inStartFrame, int inEndFrame, float fps, bool inLoop = true);
	bool ParseJson(const std::string& jsonString);
	SpriteRendererComponent* GetSpriteRenderer() const;

	DirectX::XMFLOAT2 GetFrameSize() const { return {frameW, frameH}; }

private:
	void AdvanceFrame();
	void PushUV();

	std::string ReadJson(const std::wstring& jsonPath);
	int ParseInt(const std::string& json, const std::string& key);

private:
	SpriteRendererComponent* srComp = nullptr;
	std::vector<SpriteFrame> animInfo;
	float spriteSheetW = 0, spriteSheetH = 0;
	float frameW = 0, frameH = 0;
	int cols, rows;
	int startFrame, endFrame, currFrame;
	int loopStartFrame, loopEndFrame;
	float frameDuration;
	float timer;
	bool loop;
};

