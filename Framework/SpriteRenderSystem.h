#pragma once
#include <algorithm>
#include <vector>

#include "MaterialComponent.h"
#include "SpriteRendererComponent.h"

class SpriteRenderSystem
{
public:
	void Register(AnimatorComponent* sprite)
	{
		renderers.push_back(sprite);
		isDirty = true;
	}

	void Unregister(GameObject* object)
	{
		std::erase_if(renderers, [object](const AnimatorComponent* a) { return a->owner == object; });
		isDirty = true;
	}

	void Render()
	{
		SortIfDirty();

		for (auto* sprite : renderers)
		{
			sprite->Render();
		}
	}

private:
	void SortIfDirty()
	{
		if (!isDirty)
		{
			return;
		}
		std::ranges::stable_sort(renderers,
		                         [](const AnimatorComponent* a, const AnimatorComponent* b)
		                         {
			                         if (a->GetCurrAnimation().spriteRenderer->layer != b->GetCurrAnimation().
				                         spriteRenderer->layer)
			                         {
				                         return static_cast<int>(a->GetCurrAnimation().spriteRenderer->layer) <
					                         static_cast<int>(b->GetCurrAnimation().spriteRenderer->layer);
			                         }
			                         return a->GetCurrAnimation().spriteRenderer->sortOrder < b->GetCurrAnimation().
				                         spriteRenderer->sortOrder;
		                         }
		);
		isDirty = false;
	}

private:
	std::vector<AnimatorComponent*> renderers;
	bool isDirty = true;
};
