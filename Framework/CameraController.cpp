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
        if (obj->GetTag() != ObjectTag::BLOCK) continue;

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

    DirectX::XMFLOAT3 playerPos =
        owner->GetTransform()->GetPosition();

    // プレイヤーを画面中央にする目標座標
    const float targetCameraX = playerPos.x;
    const float targetCameraY = playerPos.y;

    const float followSpeed = 5.0f;

    // X座標（左右）を追従
    const float deltaX = targetCameraX - camPos.x;
    camPos.x += deltaX *
        (1.0f - std::exp(-followSpeed * deltaTime));

    // Y座標（上下）を追従
    const float deltaY = targetCameraY - camPos.y;
    camPos.y += deltaY *
        (1.0f - std::exp(-followSpeed * deltaTime));

    camera->SetPosition(camPos);


}

float CameraController::GetStackTopY()
{
    return stackTopY;
}

float CameraController::GetGroundTop()
{
    return groundTopY;
}


