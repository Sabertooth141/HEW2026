#include "CameraController.h"

#include "Camera2D.h"
#include "GameObject.h"
#include "PlayerController.h"
#include "Scene.h"

void CameraController::Awake()
{
	MonoBehavior::Awake();
	camera = &owner->GetContext().camera;
	ownerRb = owner->GetComponent<Rigidbody2DComponent>();
    ownerController = owner->GetComponent<PlayerController>();
	stackTopY = groundTopY;
}

void CameraController::LateUpdate(float deltaTime)
{
    if (ownerController->GetHasSpawned())
    {
        return;
    }

    float highestY = groundTopY;
    bool  foundSettled = false;

    for (auto& obj : owner->GetContext().gameScene->GetObjects())
    {
        if (obj.get() == owner)                continue;
        if (obj->GetTag() != ObjectTag::Block) continue;

        const auto* rb = obj->GetComponent<Rigidbody2DComponent>();
        if (rb && !rb->IsSleeping() && std::abs(rb->GetVelocity().y) > settleSpeed)
        {
            continue;
        }

        if (const auto* collider = obj->GetComponent<BoxCollider2D>())
        {
            highestY = std::max(highestY, collider->GetWorldAABB().max.y);
            foundSettled = true;
        }
    }

    // whole tower airborne -> keep the last height instead of collapsing to the ground
    if (foundSettled)
    {
        stackTopY = highestY;
    }

    const float camY = camera->GetPosition().y;
    const float target = stackTopY + stackOffset;
    const float delta = target - camY;

    float lambda = 0.f;
    if (delta > deadZone)
    {
        lambda = riseLambda;
        fallTimer = 0.f;
    }
    else if (delta < -deadZone)
    {
        fallTimer += deltaTime;
        if (fallTimer >= fallDelay)
        {
            lambda = fallLambda;
        }
    }
    else
    {
        fallTimer = 0.f;
    }

    DirectX::XMFLOAT2 camPos = camera->GetPosition();
    if (lambda > 0.f)
    {
        camPos.y += delta * (1.f - std::exp(-lambda * deltaTime));
        camera->SetPosition(camPos);
    }

    DirectX::XMFLOAT3 playerPos = owner->GetTransform()->GetPosition();
    playerPos.y = camPos.y + playerOffset;
    owner->GetTransform()->SetPosition(playerPos);

    if (ownerRb)
    {
        ownerRb->SetVelocity({ ownerRb->GetVelocity().x, 0.f });
    }
}

float CameraController::GetStackTopY()
{
    return stackTopY;
}

float CameraController::GetGroundTop()
{
    return groundTopY;
}


