#pragma once

#include "AnimatorComponent.h"
#include "GameObject.h"
#include "IComponent.h"
#include "ObjectTags.h"
#include "PlayerController.h"

class CheckpointComponent : public IComponent
{
public:
	// このチェックポイントの表示状態を切り替える。
	void SetActivated(bool activated)
	{
		// 状態が変わらないなら、アニメーションを再スタートしない。
		if (isActivated == activated)
		{
			return;
		}

		isActivated = activated;

		AnimatorComponent* animator =
			owner->GetComponent<AnimatorComponent>();

		if (animator == nullptr)
		{
			return;
		}

		// 最後に触れた旗はアニメーション、
		// それ以外の旗は静止画像にする。
		if (isActivated)
		{
			animator->SetCurrAnimation("Activated");
		}
		else
		{
			animator->SetCurrAnimation("Static");
		}
	}

	void OnCollisionEnter2D(const GameObject& other) override
	{
		// プレイヤー以外との接触は無視する。
		if (other.GetTag() != ObjectTag::PLAYER)
		{
			return;
		}

		// 接触したプレイヤーの操作コンポーネントを取得する。
		PlayerController* playerController =
			const_cast<PlayerController*>(
				other.GetComponent<PlayerController>());

		if (playerController == nullptr)
		{
			return;
		}

		// プレイヤーに、この旗を最後に触れた旗として登録させる。
		const DirectX::XMFLOAT3 checkpointPosition =
			owner->GetTransform()->GetPosition();

		playerController->SetCheckpoint(checkpointPosition, owner);

		//触れたときのスタミナ回復
		playerController->RecoverStamina();
	}

private:
	// この旗が最後に触れたチェックポイントかどうか。
	bool isActivated = false;
};