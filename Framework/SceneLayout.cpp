#include <nlohmann/json.hpp>
#include <DirectXMath.h>
#include <fstream>
#include <cmath>
#include <filesystem>

#include "SceneLayout.h"
#include "PrefabRegistry.h"
#include "Scene.h"

namespace
{
	constexpr int LAYOUT_VERSION = 1;
	constexpr auto SCENE_DIR = "../../assets/scenes/";

	// 小数点以下 3 桁に丸めて保存する（0.1f が 0.10000000149011612 にならないように。Git の差分も読みやすくなる
	double Round3(float v)
	{
		return std::round(static_cast<double>(v) * 1000.0) / 1000.0;
	}

	nlohmann::ordered_json Float3ToJson(const DirectX::XMFLOAT3& v)
	{
		return nlohmann::ordered_json::array({Round3(v.x), Round3(v.y), Round3(v.z)});
	}

	bool JsonToFloat3(const nlohmann::ordered_json& json, DirectX::XMFLOAT3& out)
	{
		if (!json.is_array() || json.size() != 3)
		{
			return false;
		}

		for (const nlohmann::ordered_json element : json)
		{
			if (!element.is_number())
			{
				return false;
			}
		}

		out = {json[0].get<float>(), json[1].get<float>(), json[2].get<float>()};
		return true;
	}
}

std::string SceneLayout::GetPath(const std::string& sceneName)
{
	return std::string(SCENE_DIR) + sceneName + ".json";
}

SceneLayout::Result SceneLayout::Save(const Scene& scene, const std::string& sceneName)
{
	Result result;

	nlohmann::ordered_json objects = nlohmann::ordered_json::array();
	for (const auto& objPtr : scene.GetObjects())
	{
		const GameObject& obj = *objPtr;

		if (!obj.IsLayoutObject() || scene.IsPendingDestroy(&obj))
		{
			continue;
		}

		const TransformComponent* transform = obj.GetTransform();
		const DirectX::XMFLOAT3 rot = transform->GetRotation();

		nlohmann::ordered_json entry;
		entry["prefab"] = obj.GetPrefabName();
		entry["name"] = obj.GetName();
		entry["position"] = Float3ToJson(transform->GetPosition());
		entry["rotation"] = Round3(DirectX::XMConvertToDegrees(rot.z));
		entry["scale"] = Float3ToJson(transform->GetScale());
		objects.push_back(std::move(entry));
	}

	std::string text = "{\n";
	text += " \"version\": " + std::to_string(LAYOUT_VERSION) + ",\n";
	text += " \"scene\": " + nlohmann::ordered_json(sceneName).dump() + ",\n";
	text += " \"objects\": [\n";
	for (size_t i = 0; i < objects.size(); i++)
	{
		text += "  " + objects[i].dump();
		if (i + 1 < objects.size())
		{
			text += ",\n";
		}
		else
		{
			text += "\n";
		}
	}
	text += "  ]\n}\n";

	const std::filesystem::path path = GetPath(sceneName);
	std::error_code errCode;
	std::filesystem::create_directories(path.parent_path(), errCode);

	std::filesystem::path tempPath = path;
	tempPath += ".tmp";
	{
		std::ofstream out(tempPath, std::ios::binary | std::ios::trunc);
		if (!out)
		{
			result.valid = false;
			result.message = "保存失敗：" + path.string() + "を開けません";
			return result;
		}
		out << text;
		if (!out)
		{
			result.valid = false;
			result.message = "保存失敗：書き込みエラー";
			return result;
		}
	}
	std::filesystem::rename(tempPath, path, errCode);
	if (errCode)
	{
		std::filesystem::remove(tempPath, errCode);
		result.valid = false;
		result.message = "保存失敗：" + path.string() + " を置き換えられません";
		return result;
	}

	result.fileFound = true;
	result.objectCount = static_cast<int>(objects.size());
	result.message = "保存しました：" + std::to_string(result.objectCount) + " 個 → " + path.string();
	return result;
}

SceneLayout::Result SceneLayout::Load(Scene& scene, const std::string& sceneName)
{
	Result result;
	const std::filesystem::path path = GetPath(sceneName);

	std::error_code errCode;
	if (!std::filesystem::exists(path, errCode))
	{
		result.message = "シーンJSONなし(" + path.string() + ")";
		return result;
	}

	result.fileFound = true;
	std::ifstream in(path, std::ios::binary);
	const nlohmann::ordered_json root = nlohmann::ordered_json::parse(in, nullptr, false, true);
	if (root.is_discarded() || !root.is_object())
	{
		result.valid = false;
		result.message = "読み込み失敗：JSON の形式が不正です（" + path.string() + "）";
		return result;
	}

	const auto objectsIt = root.find("objects"); // iterator
	if (objectsIt == root.end() || !objectsIt->is_array())
	{
		result.valid = false;
		result.message = "読み込み失敗：\"objects\" 配列がありません";
		return result;
	}

	const PrefabRegistry& registry = PrefabRegistry::Instance();
	for (const nlohmann::ordered_json& entry : *objectsIt)
	{
		if (!entry.is_object())
		{
			result.skippedCount += 1;
			continue;
		}

		const auto prefabIt = entry.find("prefab");
		if (prefabIt == entry.end() || !prefabIt->is_string())
		{
			result.skippedCount += 1;
			continue;
		}

		// 未登録のプレハブ（名前変更・削除された等）は assert させずに飛ばす
		const std::string prefab = prefabIt->get<std::string>();
		if (!registry.Find(prefab))
		{
			result.skippedCount += 1;
			continue;
		}

		DirectX::XMFLOAT3 pos = {0.f, 0.f, 1.f};
		if (const auto it = entry.find("position"); it != entry.end())
		{
			JsonToFloat3(*it, pos);
		}

		GameObject* obj = scene.InstantiateAsLayout(prefab, pos);
		if (!obj)
		{
			result.skippedCount += 1;
			continue;
		}

		if (const auto it = entry.find("name"); it != entry.end() && it->is_string())
		{
			obj->SetName(it->get<std::string>());
		}

		TransformComponent* transform = obj->GetTransform();
		if (const auto it = entry.find("rotation"); it != entry.end() && it->is_number())
		{
			DirectX::XMFLOAT3 rot = transform->GetRotation();
			rot.z = DirectX::XMConvertToRadians(it->get<float>());
			transform->SetRotation(rot);
		}

		DirectX::XMFLOAT3 scale;
		if (const auto it = entry.find("scale"); it != entry.end() && JsonToFloat3(*it, scale))
		{
			transform->SetScale(scale);
		}

		result.objectCount += 1;
	}

	result.message = "読み込み：" + std::to_string(result.objectCount) + "　個";
	if (result.skippedCount > 0)
	{
		result.message += "（" + std::to_string(result.skippedCount) + "　個スキップ：未登録プレハブ）";
	}
	return result;
}
