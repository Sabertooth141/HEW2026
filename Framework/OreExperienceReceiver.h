#pragma once

#include <limits>

#include "IComponent.h"

// Explicit experience storage for a demo or another system without the native
// PlayerController experience API. It does not replace the player's level-up.
class OreExperienceReceiver : public IComponent
{
public:
    int GetTotal() const { return total; }

    void AddExperience(int amount)
    {
        if (amount <= 0) return;
        constexpr int maximum = (std::numeric_limits<int>::max)();
        total = total > maximum - amount ? maximum : total + amount;
    }

private:
    int total = 0;
};
