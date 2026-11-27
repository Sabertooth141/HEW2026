#pragma once

#include "GameObject.h"
#include "IComponent.h"
#include "ObjectTags.h"
#include "PlayerController.h"

//加速装置
class AccelerationDeviceComponent :public IComponent
{
public:
	void OnCollisionEnter2D(const GameObject& other)override
	{
		TryActivete(other);
	}

	//装置に触れたままダッシュを始めたときも判定
	void OnCollisionStay2D(const GameObject& other)override
	{
		TryActivete(other);
	}

private:
	void TryActivete(const GameObject& other)
	{
		//プレイヤー以外は反応しない
		if (other.GetTag() != ObjectTag::PLAYER)
		{
			return;
		}


		//プレイヤーならPlayercontrollerを探す
		const PlayerController* playerController = other.GetComponent<PlayerController>();


		//プレイヤー以外か取得できていない
		if (playerController == nullptr)
		{
			return;
		}

		//const_castはポインタの方からconstを外して関数を呼ぶ
		const_cast<PlayerController*>(playerController)->TryActivateAccelerationDevice(owner);
	}
};