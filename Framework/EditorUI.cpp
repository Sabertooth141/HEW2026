#include "EditorUI.h"

#include <algorithm>
#include <cmath>
#include <string>

#include <imgui.h>

#include "Camera2D.h"
#include "GameObject.h"
#include "PrefabRegistry.h"
#include "Scene.h"
#include "SceneManager.h"
#include "WindowSettings.h"
#include "PlayerController.h"
#include "SpeedBoostComponent.h"

namespace
{
	bool IsObjectScreenSpace(GameObject& obj)
	{
		auto* anim = obj.GetComponent<AnimatorComponent>();
		return anim && anim->IsScreenSpaceEnabled();
	}

	DirectX::XMFLOAT2 LogicPerClientPixel()
	{
		const ImVec2 display = ImGui::GetIO().DisplaySize;
		if (display.x <= 0.f || display.y <= 0.f)
		{
			return { 1.f, 1.f };
		}

		return { static_cast<float>(WIN_WIDTH) / display.x, static_cast<float>(WIN_HEIGHT) / display.y };
	}

	// スクリーン座標（左上原点・+Y 下）→ ワールド座標（画面中央原点・+Y 上）
	DirectX::XMFLOAT2 ScreenToWorld(DirectX::XMFLOAT2 screen, DirectX::XMFLOAT2 cam, bool screenSpace)
	{
		const DirectX::XMFLOAT2 ratio = LogicPerClientPixel();
		screen.x *= ratio.x;
		screen.y *= ratio.y;

		const float x = screen.x - static_cast<float>(WIN_WIDTH) * 0.5f;
		const float y = static_cast<float>(WIN_HEIGHT) * 0.5f - screen.y;
		if (screenSpace)
		{
			return { x, y };
		}

		return { x + cam.x, y + cam.y };
	}

	ImVec2 WorldToScreen(DirectX::XMFLOAT2 world, DirectX::XMFLOAT2 cam, bool screenSpace)
	{
		if (!screenSpace)
		{
			world.x -= cam.x;
			world.y -= cam.y;
		}

		const DirectX::XMFLOAT2 ratio = LogicPerClientPixel();
		return { (world.x + WIN_WIDTH * 0.5f) / ratio.x, (WIN_HEIGHT * 0.5f - world.y) / ratio.y };
	}

	// スプライトは -0.5〜0.5 の四角形を Transform でスケール・回転したもの
	void GetObjCorners(GameObject& obj, DirectX::XMFLOAT2 outCorners[4])
	{
		const TransformComponent* objTransform = obj.GetTransform();
		const DirectX::XMFLOAT3 objPos = objTransform->GetPosition();
		const DirectX::XMFLOAT3 objRot = objTransform->GetRotation();
		const DirectX::XMFLOAT3 objScale = objTransform->GetScale();

		const float halfX = objScale.x * 0.5f;
		const float halfY = objScale.y * 0.5f;
		const float rotCos = std::cos(objRot.z);
		const float rotSin = std::sin(objRot.z);

		const DirectX::XMFLOAT2 local[4] = { { -halfX, -halfY },{ halfX, -halfY },{ halfX, halfY },{ -halfX, halfY } };
		for (int i = 0; i < 4; ++i)
			outCorners[i] = {
				objPos.x + local[i].x * rotCos - local[i].y * rotSin,
				objPos.y + local[i].x * rotSin + local[i].y * rotCos
		};
	}

	bool ObjHitTest(GameObject& obj, DirectX::XMFLOAT2 cursorPos)
	{
		const TransformComponent* t = obj.GetTransform();
		const DirectX::XMFLOAT3 pos = t->GetPosition();
		const DirectX::XMFLOAT3 rot = t->GetRotation();
		const DirectX::XMFLOAT3 sc = t->GetScale();

		// クリック位置をオブジェクトのローカル空間へ
		const float dx = cursorPos.x - pos.x;
		const float dy = cursorPos.y - pos.y;
		const float c = std::cos(-rot.z);
		const float s = std::sin(-rot.z);
		const float lx = dx * c - dy * s;
		const float ly = dx * s + dy * c;

		return std::abs(lx) <= std::abs(sc.x) * 0.5f && std::abs(ly) <= std::abs(sc.y) * 0.5f;
	}

	// エディタから動かしたらスリープ中の剛体を起こす
	void WakeObjRb(GameObject& obj, bool stop)
	{
		if (auto* rb = obj.GetComponent<Rigidbody2DComponent>())
		{
			if (stop)
			{
				rb->SetVelocity({ 0.f, 0.f });
				rb->SetAngularVel(0.f);
			}

			rb->Wake();
		}
	}

	// 見た目がない（Animator なし・非表示）オブジェクト。コライダーだけの地形など
	bool IsEmptyObject(GameObject& obj)
	{
		const auto* anim = obj.GetComponent<AnimatorComponent>();
		return !anim || !anim->IsEnabled();
	}

	struct GridRect
	{
		DirectX::XMFLOAT2 min;
		DirectX::XMFLOAT2 max;
	};

	// 2 点が含まれるマスをすべて覆う矩形（クリックだけなら 1 マス）
	GridRect GetGridRect(const DirectX::XMFLOAT2 a, const DirectX::XMFLOAT2 b, const float cellSize)
	{
		// floor で切り捨てるので負の座標でも正しいマスになる
		const float minX = std::floor(std::min(a.x, b.x) / cellSize) * cellSize;
		const float minY = std::floor(std::min(a.y, b.y) / cellSize) * cellSize;
		const float maxX = (std::floor(std::max(a.x, b.x) / cellSize) + 1.f) * cellSize;
		const float maxY = (std::floor(std::max(a.y, b.y) / cellSize) + 1.f) * cellSize;

		return { { minX, minY },{ maxX, maxY } };
	}
}

void EditorUI::Draw(SceneManager& sceneManager, Camera2D& camera)
{
	if (!isVisible)
	{
		return;
	}

	DrawToolbar(sceneManager);

	Scene* scene = sceneManager.GetCurrScene();
	if (!scene)
	{
		return;
	}

	ValidateSelection(*scene);

	const bool isTransitioning = sceneManager.IsTransitioning();
	if (isTransitioning)
	{
		// シーンが作り直されるので履歴の対象はすべて無効になる
		ClearHistory();
		CancelRectTool();
	}

	ImGui::BeginDisabled(isTransitioning);
	DrawHierarchy(*scene);
	DrawInspector(*scene);
	DrawDebug(scene->GetPhysics(), scene->GetRender());

	DrawPrefabPalette(*scene, camera);
	ImGui::EndDisabled();

	DrawLayoutSave(sceneManager);

	if (!isTransitioning)
	{
		HandleHistoryShortcuts(*scene);
		HandleCamPan(camera);
		HandleViewportMouse(*scene, camera);

		DrawGrid(camera);
		DrawColliderOutlines(*scene, camera);
		DrawRectPreview(camera);
	}

	if (selectedObject)
	{
		DrawSelectionOutline(camera);
	}
}

void EditorUI::ToggleVisible()
{
	isVisible = !isVisible;
	if (!isVisible)
	{
		isPaused = false;
		isDragging = false;
		stepRequested = false;
		isPanning = false;
		CancelRectTool();
		return;
	}
	isPaused = true;
}

bool EditorUI::ShouldTick()
{
	if (mode == Mode::EDIT)
	{
		stepRequested = false;
		return false;
	}

	if (!isPaused)
	{
		return true;
	}

	if (stepRequested)
	{
		stepRequested = false;
		return true;
	}

	return false;
}

void EditorUI::ValidateSelection(Scene& scene)
{
	if (&scene != lastScene)
	{
		lastScene = &scene;
		selectedObject = nullptr;
		isDragging = false;
		isPanning = false;
		ClearHistory();
		CancelRectTool();
		return;
	}

	if (!selectedObject)
	{
		return;
	}

	const auto& objInScene = scene.GetObjects();
	const bool isObjAlive = std::ranges::any_of(objInScene, [this](const auto& obj)
		{
			return obj.get() == selectedObject;
		});

	if (!isObjAlive)
	{
		selectedObject = nullptr;
		isDragging = false;
	}
}

void EditorUI::DrawHierarchy(Scene& scene)
{
	ImGui::SetNextWindowPos({ 10, 110.f }, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize({ 200.f, 300.f }, ImGuiCond_FirstUseEver);
	if (ImGui::Begin("ヒエラルキー"))
	{
		ImGui::TextDisabled("%zu objects", scene.GetObjects().size());
		ImGui::Separator();

		for (auto& objPtr : scene.GetObjects())
		{
			GameObject* obj = objPtr.get();
			ImGui::PushID(obj);

			if (ImGui::Selectable(obj->GetName().c_str(), selectedObject == obj))
			{
				selectedObject = obj;
			}
			ImGui::PopID();
		}
	}

	ImGui::End();
}

void EditorUI::DrawInspector(Scene& scene)
{
	ImGui::SetNextWindowPos({ 540.f, 170.f }, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize({ 250.f, 420.f }, ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("インスペクター"))
	{
		ImGui::End();
		return;
	}

	if (!selectedObject)
	{
		ImGui::TextDisabled("オブジェクト未選択");
		ImGui::End();
		return;
	}

	GameObject& selectedObjRef = *selectedObject;
	ImGui::PushItemWidth(-90.f);

	// このフレームで変更される前の状態（Undo 用）
	const ObjectSnapshot frameStart = TakeSnapshot(selectedObjRef);

	// name & tag
	char nameBuffer[128];
	strncpy_s(nameBuffer, selectedObjRef.GetName().c_str(), _TRUNCATE);
	if (ImGui::InputText("名前", nameBuffer, sizeof(nameBuffer)))
	{
		selectedObjRef.SetName(nameBuffer);
	}
	TrackInspectorEdit(selectedObjRef, frameStart);

	int tag = static_cast<int>(selectedObjRef.GetTag());
	if (ImGui::Combo("タグ", &tag, objectTagNames, IM_ARRAYSIZE(objectTagNames)))
	{
		selectedObjRef.SetTag(static_cast<ObjectTag>(tag));
	}

	// transform
	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		TransformComponent* transform = selectedObjRef.GetTransform();

		DirectX::XMFLOAT3 pos = transform->GetPosition();
		if (ImGui::DragFloat2("位置", &pos.x, 1.f))
		{
			transform->SetPosition(pos);
			WakeObjRb(selectedObjRef, false);
		}
		TrackInspectorEdit(selectedObjRef, frameStart);

		DirectX::XMFLOAT3 rot = transform->GetRotation();
		float deg = DirectX::XMConvertToDegrees(rot.z);
		if (ImGui::DragFloat("回転（度）", &deg, 0.5f))
		{
			rot.z = DirectX::XMConvertToRadians(deg);
			transform->SetRotation(rot);
			WakeObjRb(selectedObjRef, false);
		}
		TrackInspectorEdit(selectedObjRef, frameStart);

		DirectX::XMFLOAT3 scale = transform->GetScale();
		if (ImGui::DragFloat2("スケール", &scale.x, 0.5f))
		{
			transform->SetScale(scale);
			WakeObjRb(selectedObjRef, false);
		}
		TrackInspectorEdit(selectedObjRef, frameStart);
	}

	// rigidbody
	if (auto* rb = selectedObjRef.GetComponent<Rigidbody2DComponent>())
	{
		if (ImGui::CollapsingHeader("Rigidbody2D", ImGuiTreeNodeFlags_DefaultOpen))
		{
			bool isStatic = rb->IsStatic();
			if (ImGui::Checkbox("Static", &isStatic))
			{
				rb->SetIsStatic(isStatic);
				rb->Wake();
			}

			bool isFreezeRot = rb->IsFreezeRotation();
			if (ImGui::Checkbox("回転を固定", &isFreezeRot))
			{
				rb->SetFreezeRotation(isFreezeRot);
			}

			DirectX::XMFLOAT2 vel = rb->GetVelocity();
			if (ImGui::DragFloat2("速度", &vel.x, 1.f))
			{
				rb->SetVelocity(vel);
				rb->Wake();
			}

			float angVel = rb->GetAngularVel();
			if (ImGui::DragFloat("角速度", &angVel, 0.05f))
			{
				rb->SetAngularVel(angVel);
				rb->Wake();
			}

			float mass = rb->GetMass();
			if (ImGui::DragFloat("質量", &mass, 0.05f, 0.01f, 10000.f))
			{
				mass = std::max(mass, 0.01f);
				rb->SetMass(mass);
				// 慣性モーメントは質量から計算されるので更新しておく
				if (auto* col = selectedObjRef.GetComponent<BoxCollider2D>())
				{
					rb->SetInertia(col->ComputeInertia(mass));
				}
			}

			float gravity = rb->GetGravity();
			if (ImGui::DragFloat("重力", &gravity, 5.f))
			{
				rb->SetGravity(gravity);
				rb->Wake();
			}

			float restitution = rb->GetRestitution();
			if (ImGui::SliderFloat("反発係数", &restitution, 0.f, 1.f))
			{
				rb->SetRestitution(restitution);
			}

			ImGui::Text("状態: %s", rb->IsSleeping() ? "スリープ" : "動作中");
			ImGui::SameLine();
			if (ImGui::SmallButton("Wake"))
			{
				rb->Wake();
			}
		}


	}

	// PlayerController
	if (auto* player = selectedObjRef.GetComponent<PlayerController>())
	{
		float speed = player->moveSpeed;

		if (ImGui::DragFloat("移動速度", &speed, 1.f))
		{
			player->moveSpeed = speed;
		}

		// ジャンプ力
		float jumpPower = player->jumpPower;

		if (ImGui::DragFloat("ジャンプ力", &jumpPower, 1.f))
		{
			player->jumpPower = jumpPower;
		}

		// スタック数ごとの能力設定
		if (auto* speedBoost = selectedObjRef.GetComponent<SpeedBoostComponent>())
		{
			if (ImGui::CollapsingHeader("加速能力設定", ImGuiTreeNodeFlags_DefaultOpen))
			{
				const char* abilityNames[] =
				{
					"壁のぼり",
					"壁破壊",
					"急降下",
					"空中走行"
				};

				const SpeedBoostComponent::BoostAbility abilities[] =
				{
					SpeedBoostComponent::BoostAbility::WallClimb,//壁のぼり
					SpeedBoostComponent::BoostAbility::BreakWall,//壁破壊
					SpeedBoostComponent::BoostAbility::Dive,//急降下
					SpeedBoostComponent::BoostAbility::AirRun
				};

				for (int stack = 1; stack <= SpeedBoostComponent::MaxAbilityStack; ++stack)
				{
					ImGui::Text("%dスタック", stack);//スタック数

					ImGui::Indent();

					for (int i = 0; i < IM_ARRAYSIZE(abilities); ++i)
					{
						ImGui::PushID(stack * 10 + i);

						bool enabled =
							speedBoost->IsAbilityEnabledForStack(stack, abilities[i]);

						if (ImGui::Checkbox(abilityNames[i], &enabled))//チェックが変更されたら保存
						{
							speedBoost->SetAbilityEnabled(stack, abilities[i], enabled);
						}

						ImGui::PopID();
					}

					ImGui::Unindent();
					ImGui::Separator();
				}
			}
		}
	}

	// box collider
	if (auto* col = selectedObjRef.GetComponent<BoxCollider2D>())
	{
		if (ImGui::CollapsingHeader("BoxCollider2D", ImGuiTreeNodeFlags_DefaultOpen))
		{
			bool trigger = col->IsTrigger();
			if (ImGui::Checkbox("Trigger", &trigger))
			{
				col->SetTrigger(trigger);
			}

			bool active = col->IsColliderActive();
			if (ImGui::Checkbox("判定を有効", &active))
			{
				col->SetColliderActive(active);
			}

			// サイズ・オフセットはスケールに対する比率（サイズ 1 = オブジェクトと同じ大きさ）
			bool isShapeChanged = false;

			const DirectX::XMFLOAT2 halfExtents = col->GetHalfExtents();
			DirectX::XMFLOAT2 size = { halfExtents.x * 2.f, halfExtents.y * 2.f };
			if (ImGui::DragFloat2("サイズ", &size.x, 0.01f, 0.01f, 100.f))
			{
				size.x = std::max(size.x, 0.01f);
				size.y = std::max(size.y, 0.01f);
				col->SetHalfExtents({ size.x * 0.5f, size.y * 0.5f });
				isShapeChanged = true;
			}

			DirectX::XMFLOAT2 offset = col->GetOffset();
			if (ImGui::DragFloat2("オフセット", &offset.x, 0.01f))
			{
				col->SetOffset(offset);
				isShapeChanged = true;
			}

			if (ImGui::SmallButton("リセット"))
			{
				col->ResetShape();
				isShapeChanged = true;
			}

			// 実際の大きさ（ピクセル）
			const OBB obb = col->GetWorldOBB();
			ImGui::TextDisabled("実サイズ: %.1f x %.1f", obb.halfExtents.x * 2.f, obb.halfExtents.y * 2.f);

			if (isShapeChanged)
			{
				// 慣性モーメントはコライダーの大きさから計算されるので更新しておく
				if (auto* rb = selectedObjRef.GetComponent<Rigidbody2DComponent>())
				{
					rb->SetInertia(col->ComputeInertia(rb->GetMass()));
					rb->Wake();
				}
			}
		}
	}

	// Animator
	if (auto* anim = selectedObjRef.GetComponent<AnimatorComponent>())
	{
		if (ImGui::CollapsingHeader("Animator", ImGuiTreeNodeFlags_DefaultOpen))
		{
			bool enabled = anim->IsEnabled();
			if (ImGui::Checkbox("表示", &enabled))
			{
				anim->SetEnabled(enabled);
			}
			ImGui::Text("スクリーン空間: %s", anim->IsScreenSpaceEnabled() ? "ON" : "OFF");
		}
	}

	ImGui::PopItemWidth();

	ImGui::Separator();
	if (ImGui::Button("削除"))
	{
		RecordDestroy(*selectedObject);
		scene.Destroy(selectedObject); // FlushPending で実際に削除される
		selectedObject = nullptr;
		isDragging = false;
	}

	ImGui::End();
}

void EditorUI::DrawToolbar(SceneManager& sceneManager)
{
	ImGui::SetNextWindowPos({ 540.f, 10.f }, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize({ 250.f, 150.f }, ImGuiCond_FirstUseEver);
	if (ImGui::Begin("エディター"))
	{
		ImGui::Text("%.1f fps", ImGui::GetIO().Framerate);

		const bool isTransitioning = sceneManager.IsTransitioning();

		ImGui::BeginDisabled(isTransitioning);
		if (mode == Mode::EDIT)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, { 0.2f, 0.55f, 0.25f, 1.f });
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.2f, 0.65f, 0.3f, 1.f });
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.15f, 0.45f, 0.2f, 1.f });

			const bool isPlayPressed = ImGui::Button("プレイ", { 90.f, 0.f });
			ImGui::PopStyleColor(3);

			if (isPlayPressed)
			{
				EnterPlayMode(sceneManager);
			}

			ImGui::SameLine();
			ImGui::TextColored({ 0.6f, 0.85f, 1.f, 1.f }, "編集モード中");
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_Button, { 0.6f, 0.25f, 0.2f, 1.f });
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.7f, 0.3f, 0.25f, 1.f });
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.5f, 0.2f, 0.15f, 1.f });

			const bool isStopPressed = ImGui::Button("編集モード", { 90.f, 0.f });
			ImGui::PopStyleColor(3);

			if (isStopPressed)
			{
				EnterEditMode(sceneManager);
			}

			ImGui::SameLine();
			if (isPaused)
			{
				ImGui::TextColored({ 1.f, 0.7f, 0.3f, 1.f }, "プレイ中（一時停止）");
			}
			else
			{
				ImGui::TextColored({ 1.f, 0.7f, 0.3f, 1.f }, "プレイ中");
			}
		}
		ImGui::EndDisabled();

		ImGui::BeginDisabled(mode != Mode::PLAY);
		if (ImGui::Button(isPaused ? "再開" : "一時停止", { 90.f, 0.f, }))
		{
			isPaused = !isPaused;
			isDragging = false;
		}

		ImGui::SameLine();
		ImGui::BeginDisabled(!isPaused);
		if (ImGui::Button("1フレーム進める"))
		{
			stepRequested = true;
		}
		ImGui::EndDisabled();
		ImGui::EndDisabled();

		// Undo/Redo（編集モードのみ）
		Scene* scene = sceneManager.GetCurrScene();
		ImGui::BeginDisabled(isTransitioning || !scene || mode != Mode::EDIT);

		ImGui::BeginDisabled(undoHistory.empty());
		if (ImGui::Button("元に戻す", { 90.f, 0.f }))
		{
			Undo(*scene);
		}
		ImGui::EndDisabled();

		ImGui::SameLine();
		ImGui::BeginDisabled(redoHistory.empty());
		if (ImGui::Button("やり直す", { 90.f, 0.f }))
		{
			Redo(*scene);
		}
		ImGui::EndDisabled();

		ImGui::EndDisabled();

		ImGui::BeginDisabled(isTransitioning);
		const std::string currScene = sceneManager.GetCurrSceneName();
		if (ImGui::BeginCombo("シーン", currScene.c_str()))
		{
			for (const std::string& sceneName : sceneManager.GetSceneNames())
			{
				if (ImGui::Selectable(sceneName.c_str(), sceneName == currScene))
				{
					ClearHistory();
					CancelRectTool();
					sceneManager.RequestChange(sceneName);
				}
			}
			ImGui::EndCombo();
		}

		if (ImGui::Button("リロード"))
		{
			ClearHistory();
			CancelRectTool();
			sceneManager.RequestChange(currScene);
		}

		if (mode == Mode::EDIT && ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("保存していない変更は破棄されます");
		}

		ImGui::EndDisabled();

		ImGui::TextDisabled("F1：表示切替");
		if (mode == Mode::EDIT)
		{
			ImGui::TextDisabled("クリックで選択／ドラッグで移動");
			ImGui::TextDisabled("Alt+ドラッグでスナップなし");
			ImGui::TextDisabled("右ドラッグでカメラ移動");
			ImGui::TextDisabled("Ctrl+Z：元に戻す");
			ImGui::TextDisabled("Ctrl+Y / Ctrl+Shift+Z：やり直す");
		}
		else if (isPaused)
		{
			ImGui::TextDisabled("一時停止中：クリックで選択／ドラッグで移動");
		}
	}

	ImGui::End();
}

void EditorUI::HandleViewportMouse(Scene& scene, Camera2D& camera)
{
	// 選択・ドラッグは一時停止中のみ（ゲームのクリック操作と競合させない）
	if (!CanEditViewport())
	{
		isDragging = false;
		isRectDragging = false;
		return;
	}

	// 矩形ツール選択中は、選択・ドラッグの代わりに範囲を指定して配置する
	if (IsRectToolActive())
	{
		isDragging = false;
		HandleRectTool(scene, camera);
		return;
	}

	isRectDragging = false;

	const ImGuiIO& io = ImGui::GetIO();
	const DirectX::XMFLOAT2 mouse = { io.MousePos.x, io.MousePos.y };
	const DirectX::XMFLOAT2 cam = camera.GetPosition();

	if (!io.WantCaptureMouse && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		selectedObject = Pick(scene, camera, mouse);
		isDragging = selectedObject != nullptr;
		if (isDragging)
		{
			const DirectX::XMFLOAT3 pos = selectedObject->GetTransform()->GetPosition();
			const DirectX::XMFLOAT2 worldPos = ScreenToWorld(mouse, cam, IsObjectScreenSpace(*selectedObject));
			objDragOffset = { pos.x - worldPos.x, pos.y - worldPos.y };
			dragStartSnapshot = TakeSnapshot(*selectedObject);
		}
	}

	if (!isDragging)
	{
		return;
	}

	if (!selectedObject || !ImGui::IsMouseDown(ImGuiMouseButton_Left))
	{
		// ドラッグ終了時に 1 回だけ履歴に積む（動いていなければ積まない）
		if (selectedObject)
		{
			RecordModify(*selectedObject, dragStartSnapshot);
		}

		isDragging = false;
		return;
	}

	TransformComponent* transform = selectedObject->GetTransform();
	const DirectX::XMFLOAT3 pos = transform->GetPosition();
	const bool isScreenSpace = IsObjectScreenSpace(*selectedObject);
	const DirectX::XMFLOAT2 worldPos = ScreenToWorld(mouse, cam, isScreenSpace);

	DirectX::XMFLOAT2 newPos = { worldPos.x + objDragOffset.x, worldPos.y + objDragOffset.y };

	// Alt を押している間はスナップしない
	if (isGridEnabled && !io.KeyAlt && !isScreenSpace)
	{
		newPos = SnapToGrid(newPos, transform->GetScale());
	}

	transform->SetPosition({ newPos.x, newPos.y, pos.z });
	WakeObjRb(*selectedObject, true);
}

void EditorUI::DrawSelectionOutline(Camera2D& camera)
{
	DirectX::XMFLOAT2 corners[4];
	GetObjCorners(*selectedObject, corners);

	const DirectX::XMFLOAT2 cam = camera.GetPosition();
	const bool isScreenSpace = IsObjectScreenSpace(*selectedObject);

	ImDrawList* drawList = ImGui::GetForegroundDrawList();
	drawList->AddQuad(WorldToScreen(corners[0], cam, isScreenSpace), WorldToScreen(corners[1], cam, isScreenSpace),
		WorldToScreen(corners[2], cam, isScreenSpace), WorldToScreen(corners[3], cam, isScreenSpace),
		IM_COL32(255, 200, 0, 255), 2.f);
}

void EditorUI::DrawLayoutSave(SceneManager& sceneManager)
{
	ImGui::Begin("シーンセーブ");

	Scene* scene = sceneManager.GetCurrScene();

	ImGui::BeginDisabled(!scene || mode != Mode::EDIT || sceneManager.IsTransitioning());
	if (ImGui::Button("シーンを保存"))
	{
		const SceneLayout::Result result = SceneLayout::Save(*scene, sceneManager.GetCurrSceneName());
		saveValid = result.valid;
		saveMessage = result.message;
	}

	ImGui::EndDisabled();
	if (mode != Mode::EDIT)
	{
		ImGui::SameLine();
		ImGui::TextDisabled("編集モードのみ");
	}

	const auto drawStatus = [](bool isValid, const std::string& logMessage, bool highlight = false)
		{
			ImVec4 color = { 1.f, 0.4f, 0.4f, 1.f };

			if (isValid)
			{
				if (highlight)
				{
					color = { 0.4f, 0.9f, 0.4f, 1.f };
				}
				else
				{
					color = ImGui::GetStyle().Colors[ImGuiCol_TextDisabled];
				}
			}

			ImGui::PushStyleColor(ImGuiCol_Text, color);
			ImGui::TextWrapped("%s", logMessage.c_str());
			ImGui::PopStyleColor();
		};

	const SceneLayout::Result& load = sceneManager.GetLastLayoutLoad();
	if (!load.message.empty())
	{
		drawStatus(load.valid && load.skippedCount == 0, load.message);
	}

	if (!saveMessage.empty())
	{
		drawStatus(saveValid, saveMessage, true);
	}

	ImGui::End();
}

void EditorUI::DrawPrefabPalette(Scene& scene, const Camera2D& camera)
{
	ImGui::SetNextWindowSize({ 200.f, 300.f }, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos({ 10.f, 420.f }, ImGuiCond_FirstUseEver);

	if (ImGui::Begin("プレハブ"))
	{
		const bool canPlace = mode == Mode::EDIT;

		ImGui::Checkbox("グリッドスナップ", &isGridEnabled);
		ImGui::SameLine();
		ImGui::TextDisabled("%.0fpx", gridSize);

		ImGui::Separator();

		if (!canPlace)
		{
			ImGui::TextDisabled("編集モードのみ配置できます");
		}
		else if (IsRectToolActive())
		{
			ImGui::TextDisabled("ドラッグで範囲を指定して配置");
			ImGui::TextDisabled("Esc で解除");
		}
		else
		{
			ImGui::TextDisabled("クリックで画面中央に配置");
			ImGui::TextDisabled("[矩形] はドラッグで範囲配置");
		}

		ImGui::Separator();

		ImGui::BeginDisabled(!canPlace);
		for (const std::string& prefabName : PrefabRegistry::Instance().GetPrefabNames())
		{
			// 地形用：選ぶと矩形ツールになる（もう一度クリックで解除）
			if (IsRectPrefab(prefabName))
			{
				const std::string label = "[矩形] " + prefabName;
				const bool isSelected = rectPrefab == prefabName;

				if (ImGui::Selectable(label.c_str(), isSelected))
				{
					rectPrefab = isSelected ? std::string{} : prefabName;
					isRectDragging = false;
				}

				continue;
			}

			if (ImGui::Selectable(prefabName.c_str()))
			{
				CancelRectTool();

				const DirectX::XMFLOAT2 cam = camera.GetPosition();

				// 次フレームの FlushPending でシーンに追加される（一時停止中も Flush は行われる）
				if (GameObject* obj = scene.InstantiateAsLayout(prefabName, { cam.x, cam.y, 1.f }))
				{
					if (isGridEnabled)
					{
						TransformComponent* transform = obj->GetTransform();
						const DirectX::XMFLOAT3 pos = transform->GetPosition();
						const DirectX::XMFLOAT2 snapped = SnapToGrid({ pos.x, pos.y }, transform->GetScale());
						transform->SetPosition({ snapped.x, snapped.y, pos.z });
					}

					selectedObject = obj;
					isDragging = false;
					RecordCreate(*obj);
				}
			}
		}
		ImGui::EndDisabled();
	}

	ImGui::End();
}

void EditorUI::DrawDebug(PhysicsSystem& physics, RenderSystem& render)
{
	const PhysicsSystem::DebugStats& s = physics.GetDebugStats();
	const int steps = physics.ConsumeStepCount();

	ImGui::Begin("Physics Stats");

	ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
	ImGui::Text("Physics steps this frame: %d", steps);

	ImGui::SeparatorText("Counts");
	ImGui::Text("Bodies:          %d", s.bodies);
	ImGui::Text("Grid cells:      %d", s.gridCells);
	ImGui::Text("Max per cell:    %d", s.maxPerCell);
	ImGui::Text("Raw pair checks: %d", s.rawPairChecks);
	ImGui::Text("Candidate pairs: %d", s.candidatePairs);
	ImGui::Text("Overlaps:        %d", s.overlaps);
	ImGui::Text("Contacts:        %d", s.contacts);

	ImGui::SeparatorText("Time per step (ms)");
	ImGui::Text("Integrate:       %.3f", s.integrateMs);
	ImGui::Text("Broad phase:     %.3f", s.broadPhaseMs);
	ImGui::Text("Narrow phase:    %.3f", s.narrowPhaseMs);
	ImGui::Text("Solve:           %.3f", s.solveMs);
	ImGui::Text("Positions:       %.3f", s.positionMs);
	ImGui::Text("Sleep + events:  %.3f", s.sleepAndEventsMs);
	ImGui::Text("Total:           %.3f", s.totalMs);

	ImGui::SeparatorText("Culling");
	ImGui::Text("Drawn: %d", render.GetDrawnSpriteCount());
	ImGui::Text("Culled: %d", render.GetCulledSpriteCount());

	ImGui::End();
}

void EditorUI::HandleCamPan(Camera2D& camera)
{
	if (!IsEditMode())
	{
		isPanning = false;
		return;
	}

	const ImGuiIO& io = ImGui::GetIO();

	if (!io.WantCaptureMouse && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
	{
		isPanning = true;
	}

	if (!isPanning)
	{
		return;
	}

	if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
	{
		isPanning = false;
		return;
	}

	camera.Move({ -io.MouseDelta.x, io.MouseDelta.y });
}

GameObject* EditorUI::Pick(Scene& scene, const Camera2D& camera, const DirectX::XMFLOAT2 mousePosScreen)
{
	const DirectX::XMFLOAT2 camPos = camera.GetPosition();
	std::vector<GameObject*> candidates;
	for (auto& obj : scene.GetObjects())
	{
		candidates.push_back(obj.get());
	}

	// 見た目のないオブジェクト（コライダーだけの地形など）は常に最優先。
	// その中では小さいものを優先して、大きいコライダーの中にある小さいものも選べるようにする
	// それ以外は手前のもの優先：レイヤーが上 → ソート順が上
	std::ranges::stable_sort(candidates, [](GameObject* a, GameObject* b)
		{
			const bool isEmptyA = IsEmptyObject(*a);
			const bool isEmptyB = IsEmptyObject(*b);

			if (isEmptyA != isEmptyB)
			{
				return isEmptyA;
			}

			if (isEmptyA)
			{
				const DirectX::XMFLOAT3 scaleA = a->GetTransform()->GetScale();
				const DirectX::XMFLOAT3 scaleB = b->GetTransform()->GetScale();
				return std::abs(scaleA.x * scaleA.y) < std::abs(scaleB.x * scaleB.y);
			}

			auto* animA = a->GetComponent<AnimatorComponent>();
			auto* animB = b->GetComponent<AnimatorComponent>();

			const int layerA = animA ? static_cast<int>(animA->GetRenderLayer()) : 0;
			const int layerB = animB ? static_cast<int>(animB->GetRenderLayer()) : 0;

			if (layerA != layerB)
			{
				return layerA > layerB;
			}

			const int orderA = animA ? animA->GetSortOrder() : 0;
			const int orderB = animB ? animB->GetSortOrder() : 0;
			return orderA > orderB;
		});

	for (int pass = 0; pass < 2; pass++)
	{
		const bool checkScreenSpace = pass == 0;

		for (const auto obj : candidates)
		{
			const bool isScreenSpace = IsObjectScreenSpace(*obj);
			if (isScreenSpace != checkScreenSpace)
			{
				continue;
			}

			if (ObjHitTest(*obj, ScreenToWorld(mousePosScreen, camPos, isScreenSpace)))
			{
				return obj;
			}
		}
	}

	return nullptr;
}

void EditorUI::EnterPlayMode(SceneManager& sceneManager)
{
	Scene* scene = sceneManager.GetCurrScene();
	if (!scene || sceneManager.IsTransitioning())
	{
		return;
	}

	const std::string sceneName = sceneManager.GetCurrSceneName();
	const SceneLayout::Result& load = sceneManager.GetLastLayoutLoad();

	const bool hasLayoutObj = std::ranges::any_of(scene->GetObjects(), [scene](const auto& obj)
		{
			return obj->IsLayoutObject() && !scene->IsPendingDestroy(obj.get());
		});

	if (!load.valid || load.skippedCount > 0)
	{
		saveValid = false;
		saveMessage = "自動保存をスキップ（" + sceneName + "）：読み込みエラー／スキップがあるため、ファイルを上書きしません";
	}
	else if (load.fileFound || hasLayoutObj)
	{
		const SceneLayout::Result result = SceneLayout::Save(*scene, sceneName);
		saveValid = result.valid;
		saveMessage = result.message;

		if (!result.valid)
		{
			return;
		}
	}

	ClearHistory();
	CancelRectTool();

	playModeStartScene = sceneName;
	mode = Mode::PLAY;
	isPaused = false;
	stepRequested = false;
	isDragging = false;
	isPanning = false;
	selectedObject = nullptr;

	sceneManager.RequestChange(sceneName);
}

void EditorUI::EnterEditMode(SceneManager& sceneManager)
{
	if (sceneManager.IsTransitioning())
	{
		return;
	}

	ClearHistory();
	CancelRectTool();

	mode = Mode::EDIT;
	isPaused = false;
	stepRequested = false;
	isDragging = false;
	isPanning = false;
	selectedObject = nullptr;

	std::string tgtScene;

	if (sceneManager.IsRegistered(playModeStartScene))
	{
		tgtScene = playModeStartScene;
	}
	else
	{
		tgtScene = sceneManager.GetCurrSceneName();
	}

	sceneManager.RequestChange(tgtScene);
}

// ===== グリッド・矩形ツール =====

bool EditorUI::IsRectPrefab(const std::string& prefabName) const
{
	return std::ranges::find(rectPrefabNames, prefabName) != rectPrefabNames.end();
}

void EditorUI::HandleRectTool(Scene& scene, const Camera2D& camera)
{
	const ImGuiIO& io = ImGui::GetIO();

	// Esc：ドラッグ中ならキャンセル、そうでなければツール解除
	if (!io.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_Escape, false))
	{
		if (isRectDragging)
		{
			isRectDragging = false;
		}
		else
		{
			rectPrefab.clear();
		}

		return;
	}

	const DirectX::XMFLOAT2 mouseWorld = ScreenToWorld({ io.MousePos.x, io.MousePos.y }, camera.GetPosition(), false);

	if (!io.WantCaptureMouse && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		isRectDragging = true;
		rectDragStart = mouseWorld;
		return;
	}

	if (!isRectDragging || ImGui::IsMouseDown(ImGuiMouseButton_Left))
	{
		return;
	}

	// ボタンを離したら、ドラッグした範囲に 1 つだけ生成してスケールで大きさを合わせる
	isRectDragging = false;

	const GridRect rect = GetGridRect(rectDragStart, mouseWorld, gridSize);
	const DirectX::XMFLOAT2 center = { (rect.min.x + rect.max.x) * 0.5f, (rect.min.y + rect.max.y) * 0.5f };
	const DirectX::XMFLOAT2 size = { rect.max.x - rect.min.x, rect.max.y - rect.min.y };

	GameObject* obj = scene.InstantiateAsLayout(rectPrefab, { center.x, center.y, 1.f });
	if (!obj)
	{
		return;
	}

	TransformComponent* transform = obj->GetTransform();
	const DirectX::XMFLOAT3 scale = transform->GetScale();
	transform->SetScale({ size.x, size.y, scale.z });
	WakeObjRb(*obj, true);

	selectedObject = obj;
	RecordCreate(*obj);
}

void EditorUI::CancelRectTool()
{
	rectPrefab.clear();
	isRectDragging = false;
}

void EditorUI::DrawGrid(const Camera2D& camera)
{
	if (!IsEditMode() || (!isGridEnabled && !IsRectToolActive()))
	{
		return;
	}

	const ImGuiIO& io = ImGui::GetIO();
	const DirectX::XMFLOAT2 cam = camera.GetPosition();

	// 画面の四隅をワールドに変換して、見えている範囲だけ線を引く
	const DirectX::XMFLOAT2 topLeft = ScreenToWorld({ 0.f, 0.f }, cam, false);
	const DirectX::XMFLOAT2 bottomRight = ScreenToWorld({ io.DisplaySize.x, io.DisplaySize.y }, cam, false);

	const int firstX = static_cast<int>(std::floor(topLeft.x / gridSize));
	const int lastX = static_cast<int>(std::ceil(bottomRight.x / gridSize));
	const int firstY = static_cast<int>(std::floor(bottomRight.y / gridSize));
	const int lastY = static_cast<int>(std::ceil(topLeft.y / gridSize));

	// 線が多すぎるとき（gridSize を小さくした・将来ズームアウトした場合）は描かない
	if ((lastX - firstX) + (lastY - firstY) > 600)
	{
		return;
	}

	// 背景描画リスト：ゲーム画面の上、ImGui ウィンドウの下に描かれる
	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	const ImU32 lineColor = IM_COL32(255, 255, 255, 35);
	const ImU32 axisColor = IM_COL32(255, 255, 255, 100);

	for (int x = firstX; x <= lastX; ++x)
	{
		const float worldX = static_cast<float>(x) * gridSize;
		drawList->AddLine(WorldToScreen({ worldX, topLeft.y }, cam, false),
			WorldToScreen({ worldX, bottomRight.y }, cam, false),
			x == 0 ? axisColor : lineColor);
	}

	for (int y = firstY; y <= lastY; ++y)
	{
		const float worldY = static_cast<float>(y) * gridSize;
		drawList->AddLine(WorldToScreen({ topLeft.x, worldY }, cam, false),
			WorldToScreen({ bottomRight.x, worldY }, cam, false),
			y == 0 ? axisColor : lineColor);
	}
}

void EditorUI::DrawRectPreview(const Camera2D& camera)
{
	if (!IsRectToolActive())
	{
		return;
	}

	const ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse && !isRectDragging)
	{
		return;
	}

	const DirectX::XMFLOAT2 cam = camera.GetPosition();
	const DirectX::XMFLOAT2 mouseWorld = ScreenToWorld({ io.MousePos.x, io.MousePos.y }, cam, false);

	// ドラッグ前はカーソル位置の 1 マスを表示
	const GridRect rect = GetGridRect(isRectDragging ? rectDragStart : mouseWorld, mouseWorld, gridSize);

	// 画面上では +Y が下なので、左上 = (min.x, max.y)、右下 = (max.x, min.y)
	const ImVec2 screenMin = WorldToScreen({ rect.min.x, rect.max.y }, cam, false);
	const ImVec2 screenMax = WorldToScreen({ rect.max.x, rect.min.y }, cam, false);

	ImDrawList* drawList = ImGui::GetForegroundDrawList();
	drawList->AddRectFilled(screenMin, screenMax, IM_COL32(80, 200, 255, isRectDragging ? 60 : 30));
	drawList->AddRect(screenMin, screenMax, IM_COL32(80, 200, 255, 230), 0.f, 0, 2.f);

	const int cellsX = static_cast<int>(std::lround((rect.max.x - rect.min.x) / gridSize));
	const int cellsY = static_cast<int>(std::lround((rect.max.y - rect.min.y) / gridSize));
	const std::string label = rectPrefab + "  " + std::to_string(cellsX) + " x " + std::to_string(cellsY);
	drawList->AddText({ screenMin.x, screenMin.y - ImGui::GetTextLineHeight() - 2.f }, IM_COL32(80, 200, 255, 255),
		label.c_str());
}

void EditorUI::DrawColliderOutlines(Scene& scene, const Camera2D& camera)
{
	// 見た目のない地形（platform など）も見えるように、編集モードでは全コライダーの枠を描く
	if (!IsEditMode())
	{
		return;
	}

	const DirectX::XMFLOAT2 cam = camera.GetPosition();
	ImDrawList* drawList = ImGui::GetBackgroundDrawList();

	for (const auto& objPtr : scene.GetObjects())
	{
		GameObject& obj = *objPtr;
		const BoxCollider2D* col = obj.GetComponent<BoxCollider2D>();
		if (!col || IsObjectScreenSpace(obj) || scene.IsPendingDestroy(&obj))
		{
			continue;
		}

		ImU32 color = IM_COL32(90, 255, 120, 200); // 通常
		if (!col->IsColliderActive())
		{
			color = IM_COL32(150, 150, 150, 150); // 無効
		}
		else if (col->IsTrigger())
		{
			color = IM_COL32(255, 220, 80, 200); // トリガー
		}

		const std::array<DirectX::XMFLOAT2, 4> corners = col->GetWorldOBB().GetCorners();
		drawList->AddQuad(WorldToScreen(corners[0], cam, false), WorldToScreen(corners[1], cam, false),
			WorldToScreen(corners[2], cam, false), WorldToScreen(corners[3], cam, false),
			color, 1.5f);
	}
}

DirectX::XMFLOAT2 EditorUI::SnapToGrid(const DirectX::XMFLOAT2 pos, const DirectX::XMFLOAT3& scale) const
{
	// オブジェクトが何マス分か（1 マスより小さいものは 1 マス扱い＝マスの中央に置く）
	const float cellsX = std::max(1.f, std::round(std::abs(scale.x) / gridSize));
	const float cellsY = std::max(1.f, std::round(std::abs(scale.y) / gridSize));
	const float halfX = cellsX * gridSize * 0.5f;
	const float halfY = cellsY * gridSize * 0.5f;

	// 左下の角をいちばん近いグリッド線に合わせる
	return {
		std::round((pos.x - halfX) / gridSize) * gridSize + halfX,
		std::round((pos.y - halfY) / gridSize) * gridSize + halfY
	};
}

// ===== Undo/Redo =====

EditorUI::ObjectSnapshot EditorUI::TakeSnapshot(GameObject& obj)
{
	const TransformComponent* transform = obj.GetTransform();

	ObjectSnapshot snapshot;
	snapshot.prefabName = obj.GetPrefabName();
	snapshot.name = obj.GetName();
	snapshot.isLayoutObject = obj.IsLayoutObject();
	snapshot.position = transform->GetPosition();
	snapshot.rotation = transform->GetRotation();
	snapshot.scale = transform->GetScale();
	return snapshot;
}

void EditorUI::ApplySnapshot(GameObject& obj, const ObjectSnapshot& snapshot)
{
	TransformComponent* transform = obj.GetTransform();

	obj.SetName(snapshot.name);
	transform->SetPosition(snapshot.position);
	transform->SetRotation(snapshot.rotation);
	transform->SetScale(snapshot.scale);
	WakeObjRb(obj, true);
}

bool EditorUI::IsSameState(const ObjectSnapshot& a, const ObjectSnapshot& b)
{
	const auto isSameFloat3 = [](const DirectX::XMFLOAT3& l, const DirectX::XMFLOAT3& r)
		{
			return l.x == r.x && l.y == r.y && l.z == r.z;
		};

	return a.name == b.name &&
		isSameFloat3(a.position, b.position) &&
		isSameFloat3(a.rotation, b.rotation) &&
		isSameFloat3(a.scale, b.scale);
}

GameObject* EditorUI::Respawn(Scene& scene, const ObjectSnapshot& snapshot)
{
	if (snapshot.prefabName.empty() || !PrefabRegistry::Instance().Find(snapshot.prefabName))
	{
		return nullptr;
	}

	GameObject* obj = snapshot.isLayoutObject
		? scene.InstantiateAsLayout(snapshot.prefabName, snapshot.position)
		: scene.Instantiate(snapshot.prefabName, snapshot.position);

	if (!obj)
	{
		return nullptr;
	}

	ApplySnapshot(*obj, snapshot);
	return obj;
}

void EditorUI::RemapInstanceId(const std::uint64_t oldId, const std::uint64_t newId)
{
	// 作り直したオブジェクトは ID が変わるので、履歴の中の古い ID を差し替える
	for (HistoryEntry& entry : undoHistory)
	{
		if (entry.instanceId == oldId)
		{
			entry.instanceId = newId;
		}
	}

	for (HistoryEntry& entry : redoHistory)
	{
		if (entry.instanceId == oldId)
		{
			entry.instanceId = newId;
		}
	}
}

void EditorUI::PushHistory(HistoryEntry entry)
{
	// 新しい操作をしたら、やり直しの履歴は捨てる
	redoHistory.clear();
	undoHistory.push_back(std::move(entry));

	if (undoHistory.size() > historyLimit)
	{
		undoHistory.erase(undoHistory.begin());
	}
}

void EditorUI::RecordModify(GameObject& obj, const ObjectSnapshot& before)
{
	if (!IsEditMode())
	{
		return;
	}

	ObjectSnapshot after = TakeSnapshot(obj);
	if (IsSameState(before, after))
	{
		return;
	}

	HistoryEntry entry;
	entry.type = HistoryEntry::Type::MODIFY;
	entry.instanceId = obj.GetInstanceId();
	entry.before = before;
	entry.after = std::move(after);
	PushHistory(std::move(entry));
}

void EditorUI::RecordCreate(GameObject& obj)
{
	if (!IsEditMode())
	{
		return;
	}

	HistoryEntry entry;
	entry.type = HistoryEntry::Type::CREATE;
	entry.instanceId = obj.GetInstanceId();
	entry.after = TakeSnapshot(obj);
	PushHistory(std::move(entry));
}

void EditorUI::RecordDestroy(GameObject& obj)
{
	// プレハブ名がないもの（コードで直接作ったもの）は作り直せないので記録しない
	if (!IsEditMode() || obj.GetPrefabName().empty())
	{
		return;
	}

	HistoryEntry entry;
	entry.type = HistoryEntry::Type::DESTROY;
	entry.instanceId = obj.GetInstanceId();
	entry.before = TakeSnapshot(obj);
	PushHistory(std::move(entry));
}

void EditorUI::TrackInspectorEdit(GameObject& obj, const ObjectSnapshot& frameStart)
{
	// 直前のウィジェットの操作開始〜終了を 1 回の変更として記録する
	if (ImGui::IsItemActivated())
	{
		inspectorEditStart = frameStart;
	}

	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		RecordModify(obj, inspectorEditStart);
	}
}

void EditorUI::Undo(Scene& scene)
{
	if (undoHistory.empty())
	{
		return;
	}

	HistoryEntry entry = std::move(undoHistory.back());
	undoHistory.pop_back();
	isDragging = false;

	switch (entry.type)
	{
	case HistoryEntry::Type::MODIFY:
		if (GameObject* obj = scene.FindByInstanceId(entry.instanceId))
		{
			ApplySnapshot(*obj, entry.before);
			selectedObject = obj;
		}
		break;

	case HistoryEntry::Type::CREATE:
		if (GameObject* obj = scene.FindByInstanceId(entry.instanceId))
		{
			if (obj == selectedObject)
			{
				selectedObject = nullptr;
			}
			scene.Destroy(obj);
		}
		break;

	case HistoryEntry::Type::DESTROY:
		if (GameObject* obj = Respawn(scene, entry.before))
		{
			RemapInstanceId(entry.instanceId, obj->GetInstanceId());
			entry.instanceId = obj->GetInstanceId();
			selectedObject = obj;
		}
		break;
	}

	redoHistory.push_back(std::move(entry));
}

void EditorUI::Redo(Scene& scene)
{
	if (redoHistory.empty())
	{
		return;
	}

	HistoryEntry entry = std::move(redoHistory.back());
	redoHistory.pop_back();
	isDragging = false;

	switch (entry.type)
	{
	case HistoryEntry::Type::MODIFY:
		if (GameObject* obj = scene.FindByInstanceId(entry.instanceId))
		{
			ApplySnapshot(*obj, entry.after);
			selectedObject = obj;
		}
		break;

	case HistoryEntry::Type::CREATE:
		if (GameObject* obj = Respawn(scene, entry.after))
		{
			RemapInstanceId(entry.instanceId, obj->GetInstanceId());
			entry.instanceId = obj->GetInstanceId();
			selectedObject = obj;
		}
		break;

	case HistoryEntry::Type::DESTROY:
		if (GameObject* obj = scene.FindByInstanceId(entry.instanceId))
		{
			if (obj == selectedObject)
			{
				selectedObject = nullptr;
			}
			scene.Destroy(obj);
		}
		break;
	}

	// PushHistory は redo を消してしまうので、ここでは直接積む
	undoHistory.push_back(std::move(entry));
}

void EditorUI::HandleHistoryShortcuts(Scene& scene)
{
	if (!IsEditMode() || isDragging || isRectDragging)
	{
		return;
	}

	const ImGuiIO& io = ImGui::GetIO();

	// テキスト入力中・ウィジェット操作中は ImGui 側の操作を優先する
	if (!io.KeyCtrl || io.WantTextInput || ImGui::IsAnyItemActive())
	{
		return;
	}

	if (ImGui::IsKeyPressed(ImGuiKey_Z))
	{
		// Ctrl+Shift+Z はやり直し
		if (io.KeyShift)
		{
			Redo(scene);
		}
		else
		{
			Undo(scene);
		}
	}
	else if (ImGui::IsKeyPressed(ImGuiKey_Y))
	{
		Redo(scene);
	}
}

void EditorUI::ClearHistory()
{
	undoHistory.clear();
	redoHistory.clear();
}