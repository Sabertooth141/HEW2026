#pragma once
#include "Renderer.h"
#include "SpriteRenderSystem.h"

class RenderSystem
{
public:
	RenderSystem(Renderer& renderer) : renderer(renderer)
	{
	}

	void Register(AnimatorComponent* sprite)
	{
		spriteSystem.Register(sprite);
	}

	void Unregister(GameObject* object)
	{
		spriteSystem.Unregister(object);
	}

	void Render()
	{
		Render2DPass();
	}

	void SetCullingEnabled(const bool inEnabled)
	{
		spriteSystem.SetCullingEnabled(inEnabled);
	}

	bool IsCullingEnabled() const
	{
		return spriteSystem.IsCullingEnabled();
	}

	int GetDrawnSpriteCount() const
	{
		return spriteSystem.GetDrawnCount();
	}

	int GetCulledSpriteCount() const
	{
		return spriteSystem.GetCulledCount();
	}

private:
	void Render2DPass()
	{
		renderer.Set2DMode();

		spriteSystem.Render(renderer.GetViewRect2D());
	}

private:
	SpriteRenderSystem spriteSystem;
	Renderer& renderer;
};