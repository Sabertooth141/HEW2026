
#include "ExperienceOrb.h"
#include "GameObject.h"
#include "PlayerController.h"
#include "Scene.h"
#include <Windows.h>
#include <cstdio>
void ExperienceOrb::Update(float deltaTime)
{
    printf("ExperienceOrb Update called!\n");
    if (owner == nullptr)
    {
        return;
    }

    auto* scene = owner->GetContext().gameScene;
    if (scene == nullptr)
    {
        OutputDebugStringA("ExperienceOrb: scene is null\n");
        return;
    }

    const auto orbPos = owner->GetTransform()->GetPosition();

    for (const auto& object : scene->GetObjects())
    {
        if (!object || object.get() == owner)
        {
            continue;
        }

     

        auto* player = object->GetComponent<PlayerController>();
        if (player == nullptr)
        {
            OutputDebugStringA("ExperienceOrb: PlayerController not found\n");
            continue;
        }

        const auto playerPos = object->GetTransform()->GetPosition();
        const float dx = orbPos.x - playerPos.x;
        const float dy = orbPos.y - playerPos.y;

        const float absorbRange =
            player->GetStatus().GetAbsorbRange() * 10.0f + 40.0f;

        if (dx * dx + dy * dy <= absorbRange * absorbRange)
        {
            OutputDebugStringA("EXP: In pickup range!\n");

            player->AddExperience(10.0f);
            OutputDebugStringA("EXP: AddExperience reached!\n");
            return;
        }
    }
}

void ExperienceOrb::OnCollisionEnter2D(const GameObject& other)
{
}