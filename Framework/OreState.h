#pragma once

#include <cstdint>
#include <unordered_map>
#include <unordered_set>

enum class OreType : std::uint8_t
{
    Gray,
    LightBlue,
    Purple
};

struct OreStats
{
    int maxHp;
    int collisionDamage;
    int experience;
};

// Temporary test values, not final planner-approved balance values.
// Pass custom OreStats to OreController when the final values are decided.
constexpr OreStats GetDefaultOreStats(OreType type)
{
    switch (type)
    {
    case OreType::LightBlue: return { 60, 10, 10 };
    case OreType::Purple: return { 100, 20, 20 };
    default: return { 30, 5, 5 };
    }
}

// Rendering-independent state, so damage and per-swing contact rules can be tested
// without creating a DirectX window or loading any assets.
class OreState
{
public:
    explicit OreState(OreType type = OreType::Gray)
        : OreState(type, GetDefaultOreStats(type))
    {
    }

    OreState(OreType type, OreStats inStats)
        : oreType(type),
          stats{
              inStats.maxHp > 0 ? inStats.maxHp : 1,
              inStats.collisionDamage > 0 ? inStats.collisionDamage : 0,
              inStats.experience > 0 ? inStats.experience : 0
          },
          hp(stats.maxHp)
    {
    }

    bool TakeDamage(int damage)
    {
        if (damage <= 0 || IsDestroyed()) return false;
        // The comparison also avoids underflow for very large damage values.
        hp = damage >= hp ? 0 : hp - damage;
        return true;
    }

    bool TryRegisterHammerHit(std::uint64_t attackId)
    {
        if (attackId == 0 || IsDestroyed()) return false;
        // Keep all IDs for this ore's lifetime: exiting/re-entering contact or
        // another hammer hitting in between must not repeat the same swing.
        return handledAttackIds.insert(attackId).second;
    }

    // Contact damage belongs to hammer state, not to ore attack damage. Zero
    // activeAttackId means the enemy is held without performing a swing.
    bool TryRegisterHammerContactDamage(
        std::uint64_t contactId,
        std::uint64_t hammerStateId,
        std::uint64_t activeAttackId,
        std::uint64_t frameId)
    {
        if (contactId == 0 || IsDestroyed()) return false;
        if (!hasContactFrame || currentContactFrame != frameId)
        {
            currentContactFrame = frameId;
            hasContactFrame = true;
            reflectedContactsThisFrame.clear();
        }

        if (hammerStateId == 0)
        {
            hammerContacts.erase(contactId);
            return false; // Ordinary enemies do not take ore contact damage.
        }

        auto& contact = hammerContacts[contactId];
        const bool newHammerState = contact.hammerStateId != hammerStateId;
        const bool newSwing = activeAttackId != 0 &&
            contact.attackId != activeAttackId;
        if (!newHammerState && !newSwing) return false;

        // Merge first contact, a state change and a new swing into one cost.
        // Keep this frame guard even across Exit/re-entry. If a second event
        // arrives in the same frame, leave its tokens unconsumed for next Stay.
        if (!reflectedContactsThisFrame.insert(contactId).second) return false;
        contact.hammerStateId = hammerStateId;
        if (activeAttackId != 0) contact.attackId = activeAttackId;
        return true;
    }

    void EndHammerContact(std::uint64_t contactId)
    {
        hammerContacts.erase(contactId);
    }

    int GetHP() const { return hp; }
    int GetMaxHP() const { return stats.maxHp; }
    OreType GetOreType() const { return oreType; }
    const OreStats& GetStats() const { return stats; }
    bool IsDestroyed() const { return hp == 0; }

private:
    struct HammerContactState
    {
        std::uint64_t hammerStateId = 0;
        std::uint64_t attackId = 0;
    };

    OreType oreType;
    OreStats stats;
    int hp;
    std::unordered_set<std::uint64_t> handledAttackIds;
    std::unordered_map<std::uint64_t, HammerContactState> hammerContacts;
    std::unordered_set<std::uint64_t> reflectedContactsThisFrame;
    std::uint64_t currentContactFrame = 0;
    bool hasContactFrame = false;
};
