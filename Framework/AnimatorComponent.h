#pragma once
#include <memory>
#include <string>
#include <unordered_map>

#include "IComponent.h"
#include "RenderLayer.h"
#include "SpriteAnimatorComponent.h"
class GameObject;

struct AnimationEntry
{
	std::string animName;
	std::unique_ptr<SpriteRendererComponent> spriteRenderer;
	std::unique_ptr<SpriteAnimatorComponent> spriteAnimator;
};

class AnimatorComponent : public IComponent
{
public:
	AnimatorComponent(Renderer& renderer);
	void AddAnimation(const std::string& animName, const std::wstring& spritePath, const std::wstring& spriteJson);
	void SetStatic(const std::wstring& spritePath);
	AnimationEntry& GetCurrAnimation() const;
	std::string& GetCurrAnimName() const;
	bool SetCurrAnimation(const std::string& animName);
	void Update(float deltaTime) override;
	void Render();
	
	bool IsEnabled() const { return enabled; }
	void SetEnabled(bool inEnabled);

	void SetRenderLayer(RenderLayer inLayer);
	void SetSortOrder(int inOrder);

	void SetFlipX(bool inFlip) const;
	void SetFlipY(bool inFlip) const;

public:
	GameObject* parent = nullptr;

private:
	Renderer& renderer;
	
	std::unordered_map<std::string, AnimationEntry> animations;
	AnimationEntry* currAnimation;

	RenderLayer renderLayer = RenderLayer::Default;
	int sortOrder = 0;

	bool enabled = true;
};
