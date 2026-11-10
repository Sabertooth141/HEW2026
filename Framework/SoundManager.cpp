#include "SoundManager.h"

#include "WaveData.h"

#include <algorithm>
#include <objbase.h>

//XAudio2 APIとCOM初期化APIをリンクする
#pragma comment(lib,"xaudio2.lib")
#pragma comment(lib,"ole32.lib")

SoundManager::~SoundManager()
{
	Shutdown();
}

HRESULT SoundManager::Initialize()
{
	//すでに初期化済みなら、二重にvoiceを作らない
	if (xAudio != nullptr && masteringVoice != nullptr)
	{
		return S_FALSE;
	}

	//XAudio2を使うので、COMを初期化
	const HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	//初期化に失敗したら呼び出しもとへ
	if (FAILED(comResult))
	{
		return comResult;
	}

	//CoInitializeExがS_OKまたはS_FALSEなら、
	//対応するCoUninitializeをあとで呼ぶ必要あり。
	isComInitialized = true;

	//XAudio2の音声処理エンジンの作成
	HRESULT result = XAudio2Create(&xAudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
	
	//失敗したら確保済みリソースを解放して終了
	if (FAILED(result))
	{
		Shutdown();
		return result;
	}
	
	//音声を最終的に出力先へ送るMastering Voiceを作成
	result = xAudio->CreateMasteringVoice(&masteringVoice);

	if (FAILED(result))
	{
		Shutdown();
		return result;
	}

	return S_OK;
}

void SoundManager::Update()
{
	//再生が終わったSEのSource Voice破棄
	//要素削除したとき未確認の要素番号のずれが発生しないよう後ろから走査
	for (std::size_t i = playingSEs.size();i > 0;--i)
	{
		PlayingSE& playingSound = playingSEs[i - 1];

		XAUDIO2_VOICE_STATE voiceState{};
		playingSound.voice->GetState(&voiceState, 0);

		//キューに残ったバッファがなければ再生完了
		if (voiceState.BuffersQueued == 0)
		{
			playingSound.voice->DestroyVoice();
			playingSEs.erase(playingSEs.begin() + (i - 1));
		}
	}
}

bool SoundManager::PlaySE(const std::shared_ptr<const WaveData>& sound, float volume)
{
	//初期化前、または空の音声データなら再生しない
	if (xAudio == nullptr || !sound || !sound->IsLoaded())
	{
		return false;
	}

	IXAudio2SourceVoice* sourceVoice = nullptr;

	//WAVの形式に合わせたSource Voiceを作成
	HRESULT result = xAudio->CreateSourceVoice(&sourceVoice, &sound->GetFormat());

	if (FAILED(result))
	{
		return false;
	}

	//再生ごとに音量を0.0から1.0の範囲に制限
	const float clampedVolume = std::clamp(volume, 0.0f, 1.0f);
	result = sourceVoice->SetVolume(clampedVolume);

	if (FAILED(result))
	{
		sourceVoice->DestroyVoice();
		return false;
	}

	//XAudio2へ渡す再生バッファの設定 音声データそのものはsoundが保持
	XAUDIO2_BUFFER buffer{};
	buffer.AudioBytes = static_cast<UINT32>(sound->GetAudioData().size());
	buffer.pAudioData = sound->GetAudioData().data();

	//再生する音声データをSource Voiceに登録
	result = sourceVoice->SubmitSourceBuffer(&buffer);

	if (FAILED(result))
	{
		sourceVoice->DestroyVoice();
		return false;
	}

	//登録した音声の再生開始
	result = sourceVoice->Start();
	if (FAILED(result))
	{
		sourceVoice->DestroyVoice();
		return false;
	}

	//再生が終わるまでWaveDataを保持
	playingSEs.push_back({ sourceVoice,sound });

	return true;
}

void SoundManager::Shutdown()
{
	StopBGM();

	//SEのSourceVoiceをすべて破棄
	for (PlayingSE& playingSound : playingSEs)
	{
		if (playingSound.voice != nullptr)
		{
			playingSound.voice->DestroyVoice();
			playingSound.voice = nullptr;
		}
	}

	//再生中データへのshared_ptrもここで解放する
	playingSEs.clear();

	if (masteringVoice != nullptr)
	{
		masteringVoice->DestroyVoice();
		masteringVoice = nullptr;
	}

	if (xAudio != nullptr)
	{
		xAudio->Release();
		xAudio = nullptr;
	}

	if (isComInitialized)
	{
		CoUninitialize();
		isComInitialized = false;
	}
}

bool SoundManager::PlayBGM(
	const std::shared_ptr<const WaveData>& sound,
	float volume)
{
	// XAudio2の初期化前、またはWAVの読み込み失敗時は再生しない
	if (xAudio == nullptr || !sound || !sound->IsLoaded())
	{
		return false;
	}

	// すでにBGMが鳴っていたら、先に停止する
	StopBGM();

	IXAudio2SourceVoice* sourceVoice = nullptr;

	// WAVの音声形式に合ったSource Voiceを作る
	HRESULT result =
		xAudio->CreateSourceVoice(&sourceVoice, &sound->GetFormat());

	if (FAILED(result))
	{
		return false;
	}

	// 音量を0.0～1.0の範囲に収める
	const float clampedVolume = std::clamp(volume, 0.0f, 1.0f);

	result = sourceVoice->SetVolume(clampedVolume);
	if (FAILED(result))
	{
		sourceVoice->DestroyVoice();
		return false;
	}

	// XAudio2へ渡す音声バッファを設定する
	XAUDIO2_BUFFER buffer{};
	buffer.Flags = XAUDIO2_END_OF_STREAM;
	buffer.AudioBytes =
		static_cast<UINT32>(sound->GetAudioData().size());
	buffer.pAudioData = sound->GetAudioData().data();

	// 音声データ全体を無限にループする
	buffer.LoopCount = XAUDIO2_LOOP_INFINITE;

	result = sourceVoice->SubmitSourceBuffer(&buffer);
	if (FAILED(result))
	{
		sourceVoice->DestroyVoice();
		return false;
	}

	result = sourceVoice->Start();
	if (FAILED(result))
	{
		sourceVoice->DestroyVoice();
		return false;
	}

	// BGMが鳴っている間、VoiceとWAVデータを保持する
	playingBGM.voice = sourceVoice;
	playingBGM.sound= sound;

	return true;
}


void SoundManager::StopBGM()
{
	if (playingBGM.voice != nullptr)
	{
		playingBGM.voice->Stop();
		playingBGM.voice->FlushSourceBuffers();
		playingBGM.voice->DestroyVoice();
		playingBGM.voice = nullptr;
	}

	playingBGM.sound.reset();
}