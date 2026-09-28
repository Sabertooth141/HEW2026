#pragma once
#include <vector>

#include "AnimatorComponent.h"

class AnimationSystem
{
public:
	void Register(AnimatorComponent* animator)
	{
		animators.push_back(animator);
	}

	void Unregister(AnimatorComponent* animator)
	{
		std::erase(animators, animator);
	}

	void Update(float deltaTime)
	{
		for (auto* anim : animators)
		{
			if (anim->IsEnabled())
			{
				anim->Update(deltaTime);
			}
		}
	}

private:
	std::vector<AnimatorComponent*> animators;
};
