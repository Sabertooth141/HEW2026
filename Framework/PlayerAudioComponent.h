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

private:
	//読み込んだジャンプ音保持
	std::shared_ptr<WaveData> jumpSound;
};
