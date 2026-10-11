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

    // プレイヤーの位置を取得
    DirectX::XMFLOAT3 playerPos =
        owner->GetTransform()->GetPosition();

    // カメラの現在位置を取得
    DirectX::XMFLOAT2 camPos = camera->GetPosition();

    // プレイヤーの位置を目標にする
    const float targetX = playerPos.x;
    const float targetY = playerPos.y;
    const float lambda = riseLambda;

    // 左右・上下ともに滑らかに追従
    const float t = 1.f - std::exp(-lambda * deltaTime);

    camPos.x += (targetX - camPos.x) * t;
    camPos.y += (targetY - camPos.y) * t;

    camera->SetPosition(camPos);
}



  //  DirectX::XMFLOAT3 playerPos = owner->GetTransform()->GetPosition();
   // playerPos.y = camPos.y + playerOffset;
    //owner->GetTransform()->SetPosition(playerPos);

//    if (ownerRb)
  //  {
    //    ownerRb->SetVelocity({ ownerRb->GetVelocity().x, 0.f });
    //}


float CameraController::GetStackTopY()
{
    return stackTopY;
}

float CameraController::GetGroundTop()
{
    return groundTopY;
}


