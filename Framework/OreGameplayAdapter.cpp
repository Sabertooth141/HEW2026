#include "OreGameplayAdapter.h"

#include <cmath>

#include "EnemyController.h"
#include "GameObject.h"
#include "OreExperienceReceiver.h"
#include "PlayerController.h"
#include "Scene.h"

#if __has_include("HarpoonController.h")
#include "HarpoonController.h"
#define ORE_HAS_NATIVE_HARPOON 1
#else
#define ORE_HAS_NATIVE_HARPOON 0
#endif

namespace
{
    template <class T>
    bool IsLivingDamageableEnemy(T& enemy)
    {
        if constexpr (requires { enemy.TakeDamage(1.0f); })
        {
            if constexpr (requires { static_cast<float>(enemy.hp); })
            {
                const float hp = static_cast<float>(enemy.hp);
                return std::isfinite(hp) && hp > 0.0f;
            }
            else if constexpr (requires { static_cast<float>(enemy.GetHP()); })
            {
                const float hp = static_cast<float>(enemy.GetHP());
                return std::isfinite(hp) && hp > 0.0f;
            }
            else return false;
        }
        else return false;
    }

    template <class T>
    bool ApplyDamage(T& enemy, float amount)
    {
        if constexpr (requires { enemy.TakeDamage(amount); })
        {
            if (!IsLivingDamageableEnemy(enemy)) return false;
            enemy.TakeDamage(amount);
            // Native HarpoonController stores its captured enemy as a raw
            // pointer. Ownership/death handling remains with that system.
            return true;
        }
        else return false;
    }

    template <class T>
    bool ReadSwingDamage(T& player, float& damage)
    {
        if constexpr (requires {
            static_cast<float>(player.GetStatus().GetSwingDamage());
        })
        {
            const float value = static_cast<float>(
                player.GetStatus().GetSwingDamage());
            damage = std::isfinite(value) && value > 0.0f ? value : 0.0f;
            return true;
        }
        else return false;
    }

    template <class T>
    bool GiveNativeExperience(T& player, int amount)
    {
        if constexpr (requires { player.AddExperience(1.0f); })
        {
            player.AddExperience(static_cast<float>(amount));
            return true;
        }
        else return false;
    }

    template <class T>
    bool ReadAbsorbRadius(T& player, float& radius)
    {
        if constexpr (requires {
            player.AddExperience(1.0f);
            static_cast<float>(player.GetStatus().GetAbsorbRange());
        })
        {
            const float range = static_cast<float>(
                player.GetStatus().GetAbsorbRange());
            if (!std::isfinite(range) || range < 0.0f) return false;
            // Match the player's existing ExperienceOrb world-unit conversion.
            // Collection itself uses this component's one-time pickup path.
            radius = range * 10.0f + 40.0f;
            return std::isfinite(radius);
        }
        else return false;
    }

#if ORE_HAS_NATIVE_HARPOON
    template <class T>
    bool CapturesObject(T& harpoon, const GameObject* object)
    {
        if constexpr (requires {
            static_cast<bool>(harpoon.IsStuck());
            harpoon.GetStuckObject() == object;
        })
        {
            // Only compare the stored pointer. A harpoon can retain an obsolete
            // target pointer; the caller's target is a live collision object.
            return harpoon.IsStuck() && harpoon.GetStuckObject() == object;
        }
        else return false;
    }
#endif
}

OreGameplayAdapter::NativeHammerContact
OreGameplayAdapter::QueryNativeHammerContact(GameObject& enemyObject)
{
    NativeHammerContact result;
#if ORE_HAS_NATIVE_HARPOON
    auto* enemy = enemyObject.GetComponent<EnemyController>();
    auto* scene = enemyObject.GetContext().gameScene;
    if (!enemy || !scene || !IsLivingDamageableEnemy(*enemy)) return result;

    for (const auto& object : scene->GetObjects())
    {
        auto* harpoon = object->GetComponent<HarpoonController>();
        if (!harpoon || !CapturesObject(*harpoon, &enemyObject)) continue;

        for (const auto& playerObject : scene->GetObjects())
        {
            auto* player = playerObject->GetComponent<PlayerController>();
            if (!player || !ReadSwingDamage(*player, result.damage)) continue;
            result.captured = true;
            result.captureSource = object.get();
            return result;
        }
    }
#else
    (void)enemyObject;
#endif
    return result;
}

bool OreGameplayAdapter::ApplyNativeContactDamage(
    GameObject& enemyObject, float damage)
{
    if (!std::isfinite(damage) || damage <= 0.0f) return false;
    auto* enemy = enemyObject.GetComponent<EnemyController>();
    return enemy && ApplyDamage(*enemy, damage);
}

bool OreGameplayAdapter::TryAddExperience(GameObject& recipient, int amount)
{
    if (amount < 0) return false;
    if (auto* receiver = recipient.GetComponent<OreExperienceReceiver>())
    {
        receiver->AddExperience(amount);
        return true;
    }
    auto* player = recipient.GetComponent<PlayerController>();
    return player && GiveNativeExperience(*player, amount);
}

bool OreGameplayAdapter::TryGetAbsorbRadius(GameObject& recipient, float& radius)
{
    // A receiver is a collision-only demo component, not a second native player.
    if (recipient.GetComponent<OreExperienceReceiver>()) return false;
    auto* player = recipient.GetComponent<PlayerController>();
    return player && ReadAbsorbRadius(*player, radius);
}
