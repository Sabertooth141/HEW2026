#pragma once

class GameObject;

// Optional integration with the team's actual harpoon/player/enemy interfaces.
// The original master has none of those damage/experience methods; these
// helpers then return unavailable rather than inventing another player system.
namespace OreGameplayAdapter
{
    struct NativeHammerContact
    {
        bool captured = false;
        float damage = 0.0f;
        const GameObject* captureSource = nullptr;
    };

    NativeHammerContact QueryNativeHammerContact(GameObject& enemyObject);
    bool ApplyNativeContactDamage(GameObject& enemyObject, float damage);
    bool TryAddExperience(GameObject& recipient, int amount);
    bool TryGetAbsorbRadius(GameObject& recipient, float& radius);
}
