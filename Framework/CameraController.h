#pragma once
#include "MonoBehavior.h"
#include "WindowSettings.h"

class PlayerController;
class Rigidbody2DComponent;
class Camera2D;

class CameraController : public MonoBehavior
{
public:
	void Awake() override;
	void LateUpdate(float deltaTime) override;

    float GetStackTopY();

	void SetGroundTop(float y)
	{
		groundTopY = y;
	}

    float GetGroundTop();

private:
	Camera2D* camera = nullptr;
	Rigidbody2DComponent* ownerRb = nullptr;
    PlayerController* ownerController = nullptr;

    float groundTopY = -200.f;          // ground pos -300 + halfExtent 100
    float stackOffset = WIN_HEIGHT * 0.15f;
    float playerOffset = WIN_HEIGHT * 0.35f;
    float settleSpeed = 10.f;

    float riseLambda = 4.f;
    float fallLambda = 2.5f;              // descend noticeably slower than we climb
    float deadZone = 10.f;               // px of slack before the camera bothers moving
    float fallDelay = 0.4f;              // target must stay low this long before descending

    float stackTopY = 0.f;                // last known settled height
    float fallTimer = 0.f;
};

