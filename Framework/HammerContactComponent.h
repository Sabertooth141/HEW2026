#pragma once

#include <DirectXMath.h>
#include <atomic>
#include <cstdint>
#include <functional>
#include <utility>

#include "IComponent.h"

// Attack-system bridge. The enemy/attack owner controls swing lifetime and
// forwards the callback to its own HP/TakeDamage implementation.
class HammerContactComponent : public IComponent
{
public:
    using DamageCallback =
        std::function<void(int, const DirectX::XMFLOAT3&)>;

    explicit HammerContactComponent(int inAttackPower = 10)
        : attackPower(inAttackPower > 0 ? inAttackPower : 0),
          contactId(nextContactId.fetch_add(1, std::memory_order_relaxed))
    {
    }

    HammerContactComponent(const HammerContactComponent&) = delete;
    HammerContactComponent& operator=(const HammerContactComponent&) = delete;

    void SetHammerState(bool enabled)
    {
        if (hammerState == enabled) return;
        hammerState = enabled;
        if (enabled) ++hammerStateId;
        else EndSwing();
    }

    bool IsHammerState() const { return hammerState; }
    std::uint64_t GetContactId() const { return contactId; }
    std::uint64_t GetHammerStateId() const
    {
        return hammerState ? hammerStateId : 0;
    }

    std::uint64_t BeginSwing()
    {
        // Being held as a hammer is distinct from attacking with that hammer.
        // A held enemy can take ore contact damage while no swing is active.
        if (!hammerState) return 0;
        // One ID source across every hammer prevents IDs from different enemies
        // colliding. Stay events can detect a new swing without needing an Exit.
        attackId = nextAttackId.fetch_add(1, std::memory_order_relaxed);
        active = true;
        return attackId;
    }

    void EndSwing() { active = false; }
    bool IsActive() const { return hammerState && active; }
    std::uint64_t GetAttackId() const { return attackId; }
    int GetAttackPower() const { return attackPower > 0 ? attackPower : 0; }

    void SetDamageCallback(DamageCallback callback)
    {
        damageCallback = std::move(callback);
    }

    void ApplyContactDamage(int damage, const DirectX::XMFLOAT3& position) const
    {
        if (damage > 0 && damageCallback) damageCallback(damage, position);
    }

    int attackPower;

private:
    inline static std::atomic<std::uint64_t> nextContactId{ 1 };
    inline static std::atomic<std::uint64_t> nextAttackId{ 1 };
    const std::uint64_t contactId;
    std::uint64_t hammerStateId = 0;
    std::uint64_t attackId = 0;
    bool hammerState = false;
    bool active = false;
    DamageCallback damageCallback;
};
