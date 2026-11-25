#pragma once

#include "GameObject.h"
#include "MonoBehavior.h"
#include "ObjectTags.h"
#include "SceneManager.h"

//プレイヤーが触れたときタイトルへ戻すゴール用のコンポーネント
class GoalComponent :public MonoBehavior
{
public:
	void OnCollisionEnter2D(const GameObject& other)override
	{
		//プレイヤー以外との接触は無視
		if (other.GetTag() != ObjectTag::PLAYER || isActivated || owner == nullptr)
		{
			return;
		}

		//一度だけ遷移要求
		isActivated = true;

		ChangeScene("Title");
	}

private:
	bool isActivated = false;
};

