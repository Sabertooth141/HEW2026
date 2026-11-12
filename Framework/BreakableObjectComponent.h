
#pragma once

#include "MonoBehavior.h"

class BreakableObjectComponent : public MonoBehavior
{
public:
    // 壊れる条件の種類
    enum class BreakType
    {
        Dive,   // 急降下で壊す
        Boost   // 加速で壊す
    };

    void SetBreakType(BreakType type)
    {
        breakType = type;
    }

    BreakType GetBreakType() const
    {
        return breakType;
    }

    void Break() const
    {
        if (isBroken)
        {
            return;
        }

        isBroken = true;
        DestroySelf();
    }

private:
    BreakType breakType = BreakType::Dive;
    mutable bool isBroken = false;
};
