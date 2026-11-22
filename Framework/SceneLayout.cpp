#include <nlohmann/json.hpp>
#include <DirectXMath.h>
#include <fstream>
#include <cmath>
#include <filesystem>

#include "SceneLayout.h"
#include "PrefabRegistry.h"
#include "Scene.h"
#include "PlayerController.h"
#include "SpeedBoostComponent.h"
#include "Rigidbody2DComponent.h"
#include "WarpComponent.h"

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
		return nlohmann::ordered_json::array({ Round3(v.x), Round3(v.y), Round3(v.z) });
	}

	nlohmann::ordered_json Float2ToJson(const DirectX::XMFLOAT2& v)
	{
		return nlohmann::ordered_json::array({ Round3(v.x), Round3(v.y) });
	}

	bool JsonToFloat2(const nlohmann::ordered_json& json, DirectX::XMFLOAT2& out)
	{
		if (!json.is_array() || json.size() != 2 || !json[0].is_number() || !json[1].is_number())
		{
			return false;
		}

		out = { json[0].get<float>(), json[1].get<float>() };
		return true;
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

		out = { json[0].get<float>(), json[1].get<float>(), json[2].get<float>() };
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


		// プレイヤーの設定を保存する
		if (const PlayerController* player =
			obj.GetComponent<PlayerController>())
		{
			nlohmann::ordered_json playerSettings;

			// 移動速度を保存
			playerSettings["moveSpeed"] = Round3(player->moveSpeed);

			// ジャンプ力を保存
			playerSettings["maxJumpHeight"] = Round3(player->maxJumpHeight);
			playerSettings["jumpToApexTime"] = Round3(player->jumpToApexTime);

			// 横慣性を保存
			playerSettings["horizontalInertia"] = Round3(player->horizontalInertia);

			// 加速能力の設定を保存
			if (const SpeedBoostComponent* speedBoost =
				obj.GetComponent<SpeedBoostComponent>())
			{
				const SpeedBoostComponent::BoostAbility abilityList[] =
				{
					SpeedBoostComponent::BoostAbility::WallClimb,
					SpeedBoostComponent::BoostAbility::BreakWall,
					SpeedBoostComponent::BoostAbility::Dive,
					SpeedBoostComponent::BoostAbility::AirRun
				};

				playerSettings["abilities"] =
					nlohmann::ordered_json::array();

				// 1～3スタックの設定を保存
				for (int stack = 1;
					stack <= SpeedBoostComponent::MaxAbilityStack;
					++stack)
				{
					nlohmann::ordered_json stackAbilities =
						nlohmann::ordered_json::array();

					// 各能力のON/OFFを保存
					for (const auto ability : abilityList)
					{
						stackAbilities.push_back(
							speedBoost->IsAbilityEnabledForStack(
								stack, ability));
					}

					playerSettings["abilities"].push_back(
						std::move(stackAbilities));
				}
			}

			entry["playerSettings"] = std::move(playerSettings);
		}

		entry["rotation"] = Round3(DirectX::XMConvertToDegrees(rot.z));
		entry["scale"] = Float3ToJson(transform->GetScale());

		// コライダーの形はプレハブから変更したときだけ保存する
		if (const BoxCollider2D* col = obj.GetComponent<BoxCollider2D>(); col && col->IsShapeModified())
		{
			nlohmann::ordered_json collider;
			collider["halfExtents"] = Float2ToJson(col->GetHalfExtents());
			collider["offset"] = Float2ToJson(col->GetOffset());
			entry["collider"] = std::move(collider);
		}

		// ワープ先をシーンデータへ保存する。
		if (const WarpComponent* warp =
			obj.GetComponent<WarpComponent>())
		{
			nlohmann::ordered_json warpSettings;
			warpSettings["targetPosition"] =
				Float3ToJson(warp->GetTargetPosition());

			entry["warpSettings"] = std::move(warpSettings);
		}

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

		DirectX::XMFLOAT3 pos = { 0.f, 0.f, 1.f };
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

		// コライダーの形（保存されているときだけ）
		if (const auto colliderIt = entry.find("collider"); colliderIt != entry.end() && colliderIt->is_object())
		{
			if (BoxCollider2D* col = obj->GetComponent<BoxCollider2D>())
			{
				DirectX::XMFLOAT2 value;
				if (const auto it = colliderIt->find("halfExtents"); it != colliderIt->end() && JsonToFloat2(*it, value))
				{
					col->SetHalfExtents(value);
				}

				if (const auto it = colliderIt->find("offset"); it != colliderIt->end() && JsonToFloat2(*it, value))
				{
					col->SetOffset(value);
				}

				// 慣性モーメントはコライダーの大きさから計算されるので更新しておく
				if (auto* rb = obj->GetComponent<Rigidbody2DComponent>())
				{
					rb->SetInertia(col->ComputeInertia(rb->GetMass()));
				}
			}
		}


		// 保存されているプレイヤー設定を読み込む
		const auto playerSettingsIt = entry.find("playerSettings");

		if (playerSettingsIt != entry.end() &&
			playerSettingsIt->is_object())
		{
			if (PlayerController* player =
				obj->GetComponent<PlayerController>())
			{
				// 移動速度を復元
				const auto speedIt =
					playerSettingsIt->find("moveSpeed");

				if (speedIt != playerSettingsIt->end() &&
					speedIt->is_number())
				{
					player->moveSpeed = speedIt->get<float>();
				}

				// 最大ジャンプ高度を復元
				const auto maxJumpHeightIt =
					playerSettingsIt->find("maxJumpHeight");

				if (maxJumpHeightIt != playerSettingsIt->end() &&
					maxJumpHeightIt->is_number())
				{
					player->maxJumpHeight =
						maxJumpHeightIt->get<float>();
				}

				// 最高地点までの時間を復元
				const auto jumpToApexTimeIt =
					playerSettingsIt->find("jumpToApexTime");

				if (jumpToApexTimeIt != playerSettingsIt->end() &&
					jumpToApexTimeIt->is_number())
				{
					player->jumpToApexTime =
						jumpToApexTimeIt->get<float>();
				}

				// 読み込んだ設定からジャンプ速度と重力を再計算
				player->UpdateJumpSettings();

				// 加速能力の設定を復元
				if (SpeedBoostComponent* speedBoost =
					obj->GetComponent<SpeedBoostComponent>())
				{
					const SpeedBoostComponent::BoostAbility abilityList[] =
					{
						SpeedBoostComponent::BoostAbility::WallClimb,
						SpeedBoostComponent::BoostAbility::BreakWall,
						SpeedBoostComponent::BoostAbility::Dive,
						SpeedBoostComponent::BoostAbility::AirRun
					};

					const auto abilitiesIt =
						playerSettingsIt->find("abilities");

					if (abilitiesIt != playerSettingsIt->end() &&
						abilitiesIt->is_array() &&
						abilitiesIt->size() >=
						SpeedBoostComponent::MaxAbilityStack)
					{
						// 1～3スタックの設定を復元
						for (int stackIndex = 0;
							stackIndex < SpeedBoostComponent::MaxAbilityStack;
							++stackIndex)
						{
							const auto& stackAbilities =
								(*abilitiesIt)[stackIndex];

							if (!stackAbilities.is_array() ||
								stackAbilities.size() < 4)
							{
								continue;
							}

							// 各能力のON/OFFを復元
							for (int abilityIndex = 0;
								abilityIndex < 4;
								++abilityIndex)
							{
								if (stackAbilities[abilityIndex].is_boolean())
								{
									speedBoost->SetAbilityEnabled(
										stackIndex + 1,
										abilityList[abilityIndex],
										stackAbilities[abilityIndex].get<bool>());
								}
							}
						}
					}

				}
				// 横慣性を復元
				const auto inertiaIt =
					playerSettingsIt->find("horizontalInertia");

				if (inertiaIt != playerSettingsIt->end() &&
					inertiaIt->is_number())
				{
					player->horizontalInertia =
						inertiaIt->get<float>();
				}
			}
		}

		// 保存されたワープ先を復元する。
		const auto warpSettingsIt = entry.find("warpSettings");

		if (warpSettingsIt != entry.end() &&
			warpSettingsIt->is_object())
		{
			if (WarpComponent* warp =
				obj->GetComponent<WarpComponent>())
			{
				const auto targetIt =
					warpSettingsIt->find("targetPosition");

				DirectX::XMFLOAT3 targetPosition;

				if (targetIt != warpSettingsIt->end() &&
					JsonToFloat3(*targetIt, targetPosition))
				{
					warp->SetTargetPosition(targetPosition);
				}
			}
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