#pragma once

#include <memory>

#include "MonoBehavior.h"

class WaveData;

//プレイヤー専用の音を読み込み、再生するコンポーネント
class PlayerAudioComponent :public MonoBehavior
{
public:
	//プレイヤーの初期化時に読み込む
	void Awake() override;

	//ジャンプ音
	void PlayJump();

	//着地音
	void PlayLand();

	//歩行音
	void UpdateWalk(float deltaTime, bool isWalking);

private:
	//読み込んだジャンプ音保持
	std::shared_ptr<WaveData> jumpSound;
	std::shared_ptr<WaveData> landSound;
	std::shared_ptr<WaveData> walkSound;

	//次の足音までの経過時間
	float walkSoundTimer = 0.0f;

	//足音の間隔
	float walkSoundInterval = 0.35f;
};
