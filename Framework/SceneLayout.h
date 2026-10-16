#pragma once
#include <string>


class Scene;

namespace SceneLayout
{
	struct Result
	{
		bool valid = true; // 書き込み失敗：false
		bool fileFound = false; //　読み込み失敗：false
		int objectCount = 0; //　保存したobject数
		int skippedCount = 0; // 未登録プレハブ数
		std::string message; // debug output
	};

	std::string GetPath(const std::string& sceneName);

	Result Save(const Scene& scene, const std::string& sceneName);
	Result Load(Scene& scene, const std::string& sceneName);
}
