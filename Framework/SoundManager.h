#pragma once

#include <xaudio2.h>

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

class WaveData;

//XAudio2エンジンと音声の出力先を管理するクラス
class SoundManager
{
public:
	//COM,XAudio2エンジン,Mastering Voiceの初期化
	HRESULT Initialize();

	//再生中のSEを確認し、再生が終わったVoiceを破棄
	void Update();

	//WAVデータを一回再生
	//ボリュームは0.0が無音、1.0が標準音量
	bool PlaySE(const std::shared_ptr<const WaveData>& sound, float volume = 1.0f);

	//Mastering Voice,XAudio2エンジン,COMの順に終了
	void Shutdown();

	//管理中の音声リソースの解放
	~SoundManager();

private:
	struct PlayingSound
	{
		IXAudio2SourceVoice* voice = nullptr;
		std::shared_ptr<const WaveData> sound;
	};

	//Source Voiceなどの作成に使う
	IXAudio2* xAudio = nullptr;

	//音声を最終的に出力デバイスへ送るvoice
	IXAudio2MasteringVoice* masteringVoice = nullptr;

	//現在再生中のSE
	std::vector<PlayingSound> playingSounds;

	//COMを初期化したか
	bool isComInitialized = false;
};