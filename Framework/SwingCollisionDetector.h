#pragma once
#include "MonoBehavior.h"

class SwingCollisionDetector : public MonoBehavior
{
public:
    void OnCollisionEnter2D(const GameObject& other) override;
};