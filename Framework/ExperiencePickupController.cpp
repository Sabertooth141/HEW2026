#include "ExperiencePickupController.h"

#include <cmath>

#include "AnimatorComponent.h"
#include "BoxCollider2D.h"
#include "GameObject.h"
#include "OreGameplayAdapter.h"
#include "Scene.h"

ExperiencePickupController::ExperiencePickupController(int inValue)
{
    SetValue(inValue);
}

void ExperiencePickupController::SetValue(int inValue)
{
    if (!collected) value = inValue > 0 ? inValue : 0;
}

bool ExperiencePickupController::TryCollect(GameObject& recipient)
{
    if (collected || !owner || !owner->GetContext().gameScene) return false;
    if (recipient.GetContext().gameScene != owner->GetContext().gameScene)
        return false;

    collected = true; // Set before the recipient's level-up callbacks can run.
    if (!OreGameplayAdapter::TryAddExperience(recipient, value))
    {
        collected = false;
        return false;
    }

    if (auto* collider = owner->GetComponent<BoxCollider2D>())
        collider->SetColliderActive(false);
    if (auto* animator = owner->GetComponent<AnimatorComponent>())
        animator->SetEnabled(false);
    SetEnabled(false);
    DestroySelf();
    return true;
}

void ExperiencePickupController::OnCollisionEnter2D(const GameObject& other)
{
    // GameObject currently lacks const component access; collision recipients
    // are live mutable objects, and awarding experience changes their state.
    TryCollect(const_cast<GameObject&>(other));
}

void ExperiencePickupController::OnCollisionStay2D(const GameObject& other)
{
    TryCollect(const_cast<GameObject&>(other));
}

void ExperiencePickupController::Update(float)
{
    if (collected || !owner) return;
    auto* scene = owner->GetContext().gameScene;
    if (!scene) return;

    const auto position = owner->GetTransform()->GetPosition();
    for (const auto& object : scene->GetObjects())
    {
        if (object.get() == owner) continue;
        float radius = 0.0f;
        if (!OreGameplayAdapter::TryGetAbsorbRadius(*object, radius)) continue;
        const auto recipientPosition = object->GetTransform()->GetPosition();
        const double x = static_cast<double>(position.x) - recipientPosition.x;
        const double y = static_cast<double>(position.y) - recipientPosition.y;
        const double distanceSquared = x * x + y * y;
        const double radiusSquared = static_cast<double>(radius) * radius;
        if (distanceSquared <= radiusSquared && TryCollect(*object)) return;
    }
}
