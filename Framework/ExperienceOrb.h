
#pragma once

#include "MonoBehavior.h"

class ExperienceOrb : public MonoBehavior
{
public:
    void OnCollisionEnter2D(const GameObject& other) override;
    void Update(float deltaTime) override;

private:
    float experienceAmount = 10.0f;
};

