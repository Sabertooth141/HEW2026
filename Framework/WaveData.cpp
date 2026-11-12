#include "WaveData.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <vector>

namespace
{
	//RIFF/WAVの構造で数値はリトルエンディアン
	//バイト列を符号なし16ビットの整数として読み取る
	std::uint16_t ReadUInt16LE(const BYTE* bytes)
	{
		return static_cast<std::uint16_t>(bytes[0]) | (static_cast<std::uint16_t>(bytes[1] << 8));
	}

	//バイト列を符号なし32ビット整数として読み取る
	std::uint32_t ReadUInt32LE(const BYTE* bytes)
	{
		return static_cast<std::uint32_t>(bytes[0]) |
			(static_cast<std::uint32_t>(bytes[1]) << 8) |
			(static_cast<std::uint32_t>(bytes[2]) << 16 |
				(static_cast<std::uint32_t>(bytes[3]) << 24));
	}

	//エラー内容を設定して、失敗理由を返す関数
	bool SetError(std::string& errorMessage, const char* message)
	{
		errorMessage = message;
		return false;
	}
}
	
bool WaveData::Load(const std::wstring& filePath, std::string& errorMessage)
{
	errorMessage.clear();

	//読み込み成功までローカル変数にデータを入れる
	WAVEFORMATEX loadedFormat{};
	std::vector<BYTE> loadedAudioData;

	std::ifstream
		file(std::filesystem::path(filePath),
			std::ios::binary);

	if (!file.is_open())
	{
		return SetError(errorMessage, "WAVファイルを開けませんでした");
	}

	//RIFFヘッダーは「RIFF」「サイズ」「WAVE」の12バイト
	std::array<BYTE, 12> riffHeader{};
	file.read(reinterpret_cast<char*>(riffHeader.data()),
		static_cast<std::streamsize>(riffHeader.size()));

	if (!file || std::memcmp(riffHeader.data(), "RIFF", 4) != 0)
	{
		return SetError(errorMessage, "RIFF形式のファイルではありません");
	}

	if (std::memcmp(riffHeader.data() + 8, "WAVE", 4) != 0)
	{
		return SetError(errorMessage, "WAVE形式のファイルではありません");
	}

	//RIFFサイズは「WAVE」以降のチャンク領域のサイズ
	const std::uint32_t riffSize = ReadUInt32LE(riffHeader.data() + 4);

	if (riffSize < 4)
	{
		return SetError(errorMessage, "RIFFサイズが不正です");
	}

	//RIFFサイズには、既に読み込んだ「WAVE」の4バイトも含まれる
	std::uint64_t remainingBytes = riffSize - 4;

	bool foundFormat = false;
	bool foundAudioData = false;

	//fmtチャンクとdataチャンクを探す
	//未知のチャンクは読み飛ばす
	while (remainingBytes >= 8)
	{
		std::array<BYTE, 8> chunkHeader{};
		file.read(reinterpret_cast<char*>(chunkHeader.data()),
			static_cast<std::streamsize>(chunkHeader.size()));

		if (!file)
		{
			return SetError(errorMessage, "WAVのチャンクヘッダーを読み込めませんでした");
		}

		const std::uint32_t chunkSize = ReadUInt32LE(chunkHeader.data() + 4);
		remainingBytes -= 8;

		if (chunkSize > remainingBytes)
		{
			return SetError(errorMessage, "WAVのチャンクサイズが不正です");
		}

		//音声形式を示すfmtチャンクを読む
		if (std::memcmp(chunkHeader.data(), "fmt ", 4)==0)
		{
			//標準的なWAVEFORMATEXの基本部分は16バイト
			if (chunkSize < 16 || chunkSize>1024)
			{
				return SetError(errorMessage, "fmtチャンクのサイズが対応範囲外です");
			}

			std::vector<BYTE>formatBytes(chunkSize);
			file.read(reinterpret_cast<char*>(formatBytes.data()),
				static_cast<std::streamsize>(formatBytes.size()));

			if (!file)
			{
				return SetError(errorMessage, "fmtチャンクを読み込めませんでした");
			}

			loadedFormat.wFormatTag = ReadUInt16LE(formatBytes.data());
			loadedFormat.nChannels = ReadUInt16LE(formatBytes.data() + 2);
			loadedFormat.nSamplesPerSec = ReadUInt32LE(formatBytes.data() + 4);
			loadedFormat.nAvgBytesPerSec = ReadUInt32LE(formatBytes.data() + 8);
			loadedFormat.nBlockAlign = ReadUInt16LE(formatBytes.data() + 12);
			loadedFormat.wBitsPerSample = ReadUInt16LE(formatBytes.data() + 14);

			//今回は標準PCM形式だけに反応
			//圧縮形式やWAVE＿EXTENSIBLEはこの読み込み処理の対象外

			loadedFormat.cbSize = 0;

			foundFormat = true;
		}


		else if (std::memcmp(chunkHeader.data(), "data", 4) == 0)
		{
			loadedAudioData.resize(chunkSize);
			file.read(reinterpret_cast<char*>(loadedAudioData.data()),
				static_cast<std::streamsize>(loadedAudioData.size()));

			if (!file)
			{
				return SetError(errorMessage, "音声データを読み込めませんでした");
			}
			foundAudioData = true;
		}

		//fmtやデータ以外のチャンクはこの再生では使わない
		else
		{
			file.seekg(static_cast<std::streamoff>(chunkSize), std::ios::cur);

			if (!file)
			{
				return SetError(errorMessage, "未対応チャンクを読み飛ばせなかった");
			}
		}

		remainingBytes -= chunkSize;

		//RIFFではチャンクのサイズが奇数なら末尾に調整用の1バイトが入る
		if ((chunkSize & 1) != 0)
		{
			if(remainingBytes == 0)
			{
				return SetError(errorMessage, "WAVチャンクの調整バイトがありません");
			}

			file.seekg(1, std::ios::cur);
			if (!file)
			{
				return SetError(errorMessage, "WAVチャンクの調整バイトを読み飛ばせませんでした");
			}
			--remainingBytes;
		}
	}

	if (!foundFormat)
	{
		return SetError(errorMessage, "fmtチャンネルタグが見つからない");

	}

	if (!foundAudioData || loadedAudioData.empty())
	{
		return SetError(errorMessage, "再生する音声データがありません");
	}

	//圧縮形式ではなく、標準PCM形式か確認
	if (loadedFormat.wFormatTag != WAVE_FORMAT_PCM)
	{
		return SetError(errorMessage, "PCM形式以外のWAVには対応していません");
	}

	//モノラルとステレオに対応する
	if (loadedFormat.nChannels < 1 || loadedFormat.nChannels>2)
	{
		return SetError(errorMessage,"モノラルまたはステレオのWAVを指定してください");
	}

	if (loadedFormat.nSamplesPerSec == 0)
	{
		return SetError(errorMessage, "サンプリングレートが不正です");
	}

	//8,16,24,32ビットPCMを対象にする
	if (loadedFormat.wBitsPerSample != 8
		&& loadedFormat.wBitsPerSample != 16
		&& loadedFormat.wBitsPerSample != 24
		&& loadedFormat.wBitsPerSample != 32)
	{
		return SetError(errorMessage, "対応していないビット数のWAVです");
	}

	//1サンプル分のバイト数から、1フレーム文のサイズを計算する
	const std::uint32_t bytesPerSample = loadedFormat.wBitsPerSample / 8;
	const std::uint32_t expectedBlockAlign = static_cast<std::uint32_t>(loadedFormat.nChannels) * bytesPerSample;

	if (loadedFormat.nBlockAlign != expectedBlockAlign)
	{
		return SetError(errorMessage, "WAVのブロックサイズが不正です");
	}

	const std::uint64_t expectedAverageBytes = static_cast<std::uint64_t>(loadedFormat.nSamplesPerSec) * loadedFormat.nBlockAlign;

	if (expectedAverageBytes > MAXUINT32|| loadedFormat.nAvgBytesPerSec != expectedAverageBytes)
	{
		return SetError(errorMessage, "WAVのデータ速度が不正です");
	}

	//dataチャンクの末尾がサンプル途中で切れていないか確認する
	if (loadedAudioData.size() % loadedFormat.nBlockAlign != 0)
	{
		return SetError(errorMessage, "音声データのサイズが不正です");
	}

	//すべて通ったら、読み込んだ内容をこのオブジェクトに保存する
	waveFormat = loadedFormat;
	audioData = std::move(loadedAudioData);

	return true;
}


