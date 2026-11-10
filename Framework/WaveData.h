#pragma once

#include <xaudio2.h>

#include <string>
#include <vector>

//WAVファイルから読み込んだ音声データを保持するクラス
//SoundManagerはこのクラスから音声形式と波形データを取得する

class WaveData
{
public:
	//指定のWAVファイルを読み込む 読み込み失敗時はfalseを返す
	bool Load(const std::wstring& filePath, std::string& errorMessage);

	//SoundManagerがSourceVoiceを作成するとき使う音声形式を返す
	const WAVEFORMATEX& GetFormat() const
	{
		return waveFormat;
	}

	//WAVの波形データを返す　再生が完了するまでこのデータを保持
	const std::vector<BYTE>& GetAudioData() const
	{
		return audioData;
	}

	//正しく読み込まれた音声データがあるか
	bool IsLoaded() const
	{
		return !audioData.empty();
	}

private:
	//チャンネル数、サンプリングレート、ビット数などの形式情報
	WAVEFORMATEX waveFormat{};

	//WAVファイルのdataチャンクに入っていた波形データ
	std::vector<BYTE> audioData;
};