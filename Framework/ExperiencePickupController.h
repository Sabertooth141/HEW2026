#pragma once

#include "MonoBehavior.h"

class ExperiencePickupController : public MonoBehavior
{
public:
    explicit ExperiencePickupController(int value = 1);

    void SetValue(int value);
    int GetValue() const { return value; }
    bool IsCollected() const { return collected; }
    bool TryCollect(GameObject& recipient);

    void Update(float deltaTime) override;
    void OnCollisionEnter2D(const GameObject& other) override;
    void OnCollisionStay2D(const GameObject& other) override;

private:
    int value = 1;
    bool collected = false;
};
