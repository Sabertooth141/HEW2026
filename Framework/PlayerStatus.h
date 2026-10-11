
#pragma once
#include <vector>

class PlayerStatus
{
public:
    int GetLevel() const { return level; }

    float GetMaxHP() const { return maxHP; }
    float GetHP() const { return hp; }

    float GetMoveSpeed() const { return moveSpeed; }
    float GetSwingSpeed() const { return swingSpeed; }
    float GetSwingDamage() const { return swingDamage; }
    float GetViewRange() const { return viewRange; }
    float GetHarpoonInterval() const { return harpoonInterval; }
    float GetAbsorbRange() const { return absorbRange; }

    enum class StatType
    {
        MAX_HP,
        SWING_DAMAGE,
        SWING_SPEED,
        MOVE_SPEED,
        VIEW_RANGE,
        HARPOON_INTERVAL,
        ABSORB_RANGE
    };

    std::vector<StatType> GetLevelUpChoices() const;

    void ApplyLevelUp(StatType type);

    void AddMoveSpeed(float amount);
    void AddSwingSpeed(float amount);
    void AddSwingDamage(float amount);
    void AddViewRange(float amount);
    void ReduceHarpoonInterval(float amount);
    void AddAbsorbRange(float amount);


    void TakeDamage(float damage);
    bool IsDead() const;
    void AddMaxHP(float amount);
    void LevelUp();

    int maxHPLevel = 1;
    int swingDamageLevel = 1;
    int swingSpeedLevel = 1;
    int moveSpeedLevel = 1;
    int viewRangeLevel = 1;
    int harpoonIntervalLevel = 1;
    int absorbRangeLevel = 1;

    void AddExperience(float amount)
    {
        if (amount <= 0.0f)
        {
            return;
        }

        exp += amount;

        while (exp >= nextLevelExp)
        {
            exp -= nextLevelExp;
            LevelUp();
            nextLevelExp += 50.0f;
        }
    }
      
private:


    int level = 1;

    float exp = 0.0f;
    float nextLevelExp = 100.0f;



    float maxHP = 3.0f;
    float hp = 3.0f;

    float moveSpeed = 500.0f;
    float swingSpeed = 1.0f;
    float swingDamage = 1.0f;
    float viewRange = 1.0f;
    float harpoonInterval = 2.0f;
    float absorbRange = 1.0f;
};