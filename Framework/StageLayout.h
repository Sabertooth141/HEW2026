#pragma once

#include <cstddef>

namespace StageLayout
{
	struct ImageData
	{
		const wchar_t* texturePath;
		const char* objectName;

		// 画像の中心位置と表示サイズ
		float centerX;
		float centerY;
		float width;
		float height;

		// 画像と画像の間に意図的な隙間がある場合、
		// カメラ範囲だけを隙間側へ広げる値。
		float cameraLeftExtension = 0.0f;
		float cameraRightExtension = 0.0f;
		float cameraBottomExtension = 0.0f;
		float cameraTopExtension = 0.0f;
	};

	inline constexpr ImageData images[] =
	{
		// 既存ステージ
		{ L"../../assets/mapdesign_s/1_s.png", "StageBackground1",    0.0f,     0.0f, 1920.0f, 1080.0f },
		{ L"../../assets/mapdesign_s/2_s.png", "StageBackground2", 1920.0f,     0.0f, 1920.0f, 1080.0f },
		{ L"../../assets/mapdesign_s/3_s.png", "StageBackground3", 3840.0f,     0.0f, 1920.0f, 1080.0f },
		{ L"../../assets/mapdesign_s/4_s.png", "StageBackground4", 3840.0f, -1080.0f, 1920.0f, 1080.0f },
		{ L"../../assets/mapdesign_s/5_s.png", "StageBackground5", 5760.0f, -1080.0f, 1920.0f, 1080.0f },
		{ L"../../assets/mapdesign_s/6_s.png", "StageBackground6", 5760.0f,     0.0f, 1920.0f, 1080.0f },
		{ L"../../assets/mapdesign_s/7_s.png", "StageBackground7", 7680.0f,     0.0f, 1920.0f, 1080.0f },
		{ L"../../assets/mapdesign_s/8_s.png", "StageBackground8", 7680.0f, -1080.0f, 1920.0f, 1080.0f },

		// mapdesign2の下段：7 → 1 → 2 → 3 → 4 → 5 → 10
		// 下段の画像同士は隙間なく接続。
		// 既存ステージ右端の8640から、最初の画像までだけ500空ける。
		{ L"../../assets/mapdesign2/7.png",  "MapDesign2_Stage7",   9754.5f, -1080.0f, 1229.0f, 1080.0f, 500.0f },
		{ L"../../assets/mapdesign2/1.png",  "MapDesign2_Stage1",  11336.5f, -1080.0f, 1935.0f, 1080.0f },
		{ L"../../assets/mapdesign2/2.png",  "MapDesign2_Stage2",  13410.5f, -1080.0f, 2213.0f, 1080.0f },
		{ L"../../assets/mapdesign2/3.png",  "MapDesign2_Stage3",  15775.5f, -1080.0f, 2517.0f, 1080.0f },
		{ L"../../assets/mapdesign2/4.png",  "MapDesign2_Stage4",  18291.5f, -1080.0f, 2515.0f, 1080.0f },
		{ L"../../assets/mapdesign2/5.png",  "MapDesign2_Stage5",  20019.5f, -1080.0f,  941.0f, 1080.0f },
		{ L"../../assets/mapdesign2/10.png", "MapDesign2_Stage10", 22090.0f, -1080.0f, 3200.0f, 1080.0f },

		// mapdesign2の上段：8と9は横につなげる。6は添付図の位置。
		// 上段Y=0、下段Y=-1080なので、上下の画像端もY=-540で接する。
		{ L"../../assets/mapdesign2/8.png",  "MapDesign2_Stage8",  13428.5f, 0.0f, 2249.0f, 1080.0f },
		{ L"../../assets/mapdesign2/9.png",  "MapDesign2_Stage9",  15361.0f, 0.0f, 1616.0f, 1080.0f },
		{ L"../../assets/mapdesign2/6.png", "MapDesign2_Stage6",21149.5f, 0.0f, 3200.0f, 1080.0f },
	};

	inline constexpr int imageCount =
		static_cast<int>(sizeof(images) / sizeof(images[0]));
}