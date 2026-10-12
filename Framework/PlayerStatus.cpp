#include "PlayerStatus.h"
#include "Gameobject.h"
#include <algorithm>
#include <random>

void PlayerStatus::TakeDamage(float damage)
{
    hp -= damage;

    if (hp < 0.0f)
    {
        hp = 0.0f;
    }

}

bool PlayerStatus::IsDead() const
{
    return hp <= 0.0f;

}
void PlayerStatus::LevelUp()
{
    level++;
}
void PlayerStatus::AddMaxHP(float amount)
{
    maxHP += amount;
    hp += amount;
}

void PlayerStatus::AddMoveSpeed(float amount)
{
    moveSpeed += amount;
}

void PlayerStatus::AddSwingSpeed(float amount)
{
    swingSpeed += amount;
}

void PlayerStatus::AddSwingDamage(float amount)
{
    swingDamage += amount;
}

void PlayerStatus::AddViewRange(float amount)
{
    viewRange += amount;
}

void PlayerStatus::ReduceHarpoonInterval(float amount)
{
    harpoonInterval -= amount;

    if (harpoonInterval < 0.0f)
    {
        harpoonInterval = 0.0f;
    }
}

void PlayerStatus::AddAbsorbRange(float amount)
{
    absorbRange += amount;
}


void PlayerStatus::ApplyLevelUp(StatType type)
{
    switch (type)
    {
    case StatType::MAX_HP:
        if (maxHPLevel < 5)
        {
            AddMaxHP(1.0f);
            maxHPLevel++;
        }
        break;

    case StatType::SWING_DAMAGE:
        if (swingDamageLevel < 5)
        {
            AddSwingDamage(1.0f);
            swingDamageLevel++;
        }
        break;

    case StatType::SWING_SPEED:
        if (swingSpeedLevel < 5)
        {
            AddSwingSpeed(0.5f);
            swingSpeedLevel++;
        }
        break;

    case StatType::MOVE_SPEED:
        if (moveSpeedLevel < 5)
        {
            AddMoveSpeed(100.0f);
            moveSpeedLevel++;
        }
        break;

    case StatType::VIEW_RANGE:
        if (viewRangeLevel < 5)
        {
            AddViewRange(0.1f);
            viewRangeLevel++;
        }
        break;

    case StatType::HARPOON_INTERVAL:
        if (harpoonIntervalLevel < 5)
        {
            ReduceHarpoonInterval(0.1f);
            harpoonIntervalLevel++;
        }
        break;

    case StatType::ABSORB_RANGE:
        if (absorbRangeLevel < 5)
        {
            AddAbsorbRange(1.0f);
            absorbRangeLevel++;
        }
        break;
    }
}

std::vector<PlayerStatus::StatType> PlayerStatus::GetLevelUpChoices() const
{
    std::vector<StatType> availableChoices;

    if (maxHPLevel < 5)
        availableChoices.push_back(StatType::MAX_HP);

    if (swingDamageLevel < 5)
        availableChoices.push_back(StatType::SWING_DAMAGE);

    if (swingSpeedLevel < 5)
        availableChoices.push_back(StatType::SWING_SPEED);

    if (moveSpeedLevel < 5)
        availableChoices.push_back(StatType::MOVE_SPEED);

    if (viewRangeLevel < 5)
        availableChoices.push_back(StatType::VIEW_RANGE);

    if (harpoonIntervalLevel < 5)
        availableChoices.push_back(StatType::HARPOON_INTERVAL);

    if (absorbRangeLevel < 5)
        availableChoices.push_back(StatType::ABSORB_RANGE);

    static std::mt19937 rng(std::random_device{}());

    std::shuffle(
        availableChoices.begin(),
        availableChoices.end(),
        rng
    );

    if (availableChoices.size() > 3)
    {
        availableChoices.resize(3);
    }

    return availableChoices;
}

void PlayerStatus::Heal(float amount)
{
    hp += amount;
    if (hp > maxHP)
    {
        hp = maxHP;
    }
}