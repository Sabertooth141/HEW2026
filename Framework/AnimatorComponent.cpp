#include "AnimatorComponent.h"

#include <stdexcept>

#include "GameObject.h"
#include "PixelShader.h"
#include "SpriteRendererComponent.h"
#include "TextureCache.h"

AnimatorComponent::AnimatorComponent(Renderer& renderer) : renderer(renderer), currAnimation(nullptr)
{
}

void AnimatorComponent::AddAnimation(const std::string& animName, const std::wstring& spritePath,
                                     const std::wstring& spriteJson)
{
	auto it = animations.find(animName);
	// stop if animation name already exists
	if (it != animations.end())
	{
		throw std::runtime_error("Animation name [" + animName + "] already used");
	}

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv = TextureCache::Load(renderer, spritePath);
	auto spriteRenderer = std::make_unique<SpriteRendererComponent>(renderer, srv.Get());
	spriteRenderer->layer = renderLayer;
	spriteRenderer->sortOrder = sortOrder;

	spriteRenderer->owner = owner;
	auto spriteAnimator = std::make_unique<SpriteAnimatorComponent>(*spriteRenderer, spriteJson);
	spriteAnimator->owner = owner;

	owner->GetTransform()->SetScale({spriteAnimator->GetFrameSize().x, spriteAnimator->GetFrameSize().y, 1});
	animations[animName] = {animName, std::move(spriteRenderer), std::move(spriteAnimator)};
}

void AnimatorComponent::SetStatic(const std::wstring& spritePath)
{
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv = TextureCache::Load(renderer, spritePath);
	auto spriteRenderer = std::make_unique<SpriteRendererComponent>(renderer, srv.Get());
	spriteRenderer->layer = renderLayer;
	spriteRenderer->sortOrder = sortOrder;
	spriteRenderer->owner = owner;
	auto spriteAnimator = std::make_unique<SpriteAnimatorComponent>(*spriteRenderer, 1, 1, 1);
	spriteAnimator->owner = owner;

	owner->GetTransform()->SetScale({spriteAnimator->GetFrameSize().x, spriteAnimator->GetFrameSize().y, 1});
	animations["Static"] = {"Static", std::move(spriteRenderer), std::move(spriteAnimator)};

	SetCurrAnimation("Static");
}

AnimationEntry& AnimatorComponent::GetCurrAnimation() const
{
	return *currAnimation;
}

std::string& AnimatorComponent::GetCurrAnimName() const
{
	return currAnimation->animName;
}

bool AnimatorComponent::SetCurrAnimation(const std::string& animName)
{
	auto it = animations.find(animName);

	if (it == animations.end())
	{
		return false;
	}
	currAnimation = &it->second;
	return true;
}

void AnimatorComponent::Update(float deltaTime)
{
	currAnimation->spriteRenderer->Update(deltaTime);
	currAnimation->spriteAnimator->Update(deltaTime);
}

void AnimatorComponent::Render()
{
	if (currAnimation->spriteRenderer.get() == nullptr)
	{
		return;
	}

	currAnimation->spriteRenderer->Bind(renderer); // b1 UV, t0 texture, s0 sampler
	currAnimation->spriteRenderer->Render();
}

void AnimatorComponent::SetEnabled(bool inEnabled)
{
	enabled = inEnabled;
}

void AnimatorComponent::SetRenderLayer(const RenderLayer inLayer)
{
	renderLayer = inLayer;
	for (auto& entry : animations)
	{
		entry.second.spriteRenderer->layer = inLayer;
	}
}

void AnimatorComponent::SetSortOrder(const int inOrder)
{
	sortOrder = inOrder;
	for (auto& entry : animations)
	{
		entry.second.spriteRenderer->sortOrder = inOrder;
	}
}

void AnimatorComponent::SetFlipX(const bool inFlip) const
{
	for (auto& entry : animations)
	{
		entry.second.spriteRenderer->flipX = inFlip;
	}
}

void AnimatorComponent::SetFlipY(const bool inFlip) const
{
	for (auto& entry : animations)
	{
		entry.second.spriteRenderer->flipY = inFlip;
	}
}
