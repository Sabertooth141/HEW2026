#pragma once

#include "GameObject.h"
#include "IComponent.h"
#include "ObjectTags.h"
#include "PlayerController.h"

class CheckpointComponent :public IComponent
{
public:
	void OnCollisionEnter2D(const GameObject& other) override
	{
		if (other.GetTag() != ObjectTag::PLAYER)
		{
			return;
		}

		PlayerController* playerController = const_cast<PlayerController*>(other.GetComponent<PlayerController>());
		if (playerController == nullptr)
		{
			return;
		}

		const DirectX::XMFLOAT3 checkpointPosition =
			owner->GetTransform()->GetPosition();

		playerController->SetCheckpoint(checkpointPosition, owner);
	}
};