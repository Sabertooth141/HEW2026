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

private:
	void Render2DPass()
	{
		renderer.Set2DMode();

		spriteSystem.Render();
	}

private:
	SpriteRenderSystem spriteSystem;
	Renderer& renderer;
};
