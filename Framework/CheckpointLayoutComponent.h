#pragma once

#include <array>

#include "GameObject.h"
#include "MonoBehavior.h"

class CheckpointLayoutComponent :public MonoBehavior
{
public:
	void Start() override
	{
		for (std::size_t checkpointIndex = 0;checkpointIndex < checkpointPositions.size();++checkpointIndex)
		{
			GameObject* checkpointObj = Instantiate("checkpoint", checkpointPositions[checkpointIndex]);

			if (checkpointObj != nullptr)
			{
				checkpointObj->SetName("Checkpoint" + std::to_string(checkpointIndex + 1));
			}
		}
	}

private:
	const std::array<DirectX::XMFLOAT3, 0> checkpointPositions =
	{
	
	};
};
