#include "EditorUI.h"

#include <imgui.h>

#include "Camera2D.h"
#include "GameObject.h"
#include "Scene.h"
#include "SceneManager.h"
#include "WindowSettings.h"

namespace
{
	bool IsObjectScreenSpace(GameObject& obj)
	{
		auto* anim = obj.GetComponent<AnimatorComponent>();
		return anim && anim->IsScreenSpaceEnabled();
	}

	// スクリーン座標（左上原点・+Y 下）→ ワールド座標（画面中央原点・+Y 上）
	DirectX::XMFLOAT2 ScreenToWorld(DirectX::XMFLOAT2 screen, DirectX::XMFLOAT2 cam, bool screenSpace)
	{
		const float x = screen.x - static_cast<float>(WIN_WIDTH) * 0.5f;
		const float y = static_cast<float>(WIN_HEIGHT) * 0.5f - screen.y;
		if (screenSpace)
		{
			return {x, y};
		}

		return {x + cam.x, y + cam.y};
	}

	ImVec2 WorldToScreen(DirectX::XMFLOAT2 world, DirectX::XMFLOAT2 cam, bool screenSpace)
	{
		if (!screenSpace)
		{
			world.x -= cam.x;
			world.y -= cam.y;
		}
		return {world.x + WIN_WIDTH * 0.5f, WIN_HEIGHT * 0.5f - world.y};
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

		const DirectX::XMFLOAT2 local[4] = {{-halfX, -halfY}, {halfX, -halfY}, {halfX, halfY}, {-halfX, halfY}};
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
				rb->SetVelocity({0.f, 0.f});
				rb->SetAngularVel(0.f);
			}

			rb->Wake();
		}
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
	ImGui::BeginDisabled(isTransitioning);
	DrawHierarchy(*scene);
	DrawInspector(*scene);
	ImGui::EndDisabled();

	if (!isTransitioning)
	{
		HandleViewportMouse(*scene, camera);
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
	}
}

bool EditorUI::ShouldTick()
{
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
}

void EditorUI::DrawHierarchy(Scene& scene)
{
	ImGui::SetNextWindowPos({10, 110.f}, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize({200.f, 300.f}, ImGuiCond_FirstUseEver);
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
	ImGui::SetNextWindowPos({540.f, 170.f}, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize({250.f, 420.f}, ImGuiCond_FirstUseEver);
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

	// name & tag
	char nameBuffer[128];
	strncpy_s(nameBuffer, selectedObjRef.GetName().c_str(), _TRUNCATE);
	if (ImGui::InputText("名前", nameBuffer, sizeof(nameBuffer)))
	{
		selectedObjRef.SetName(nameBuffer);
	}

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

		DirectX::XMFLOAT3 rot = transform->GetRotation();
		float deg = DirectX::XMConvertToDegrees(rot.z);
		if (ImGui::DragFloat("回転（度）", &deg, 0.5f))
		{
			rot.z = DirectX::XMConvertToRadians(deg);
			transform->SetRotation(rot);
			WakeObjRb(selectedObjRef, false);
		}

		DirectX::XMFLOAT3 scale = transform->GetScale();
		if (ImGui::DragFloat2("スケール", &scale.x, 0.5f))
		{
			transform->SetScale(scale);
			WakeObjRb(selectedObjRef, false);
		}
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
		scene.Destroy(selectedObject); // FlushPending で実際に削除される
		selectedObject = nullptr;
		isDragging = false;
	}

	ImGui::End();
}

void EditorUI::DrawToolbar(SceneManager& sceneManager)
{
	ImGui::SetNextWindowPos({540.f, 10.f}, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize({250.f, 150.f}, ImGuiCond_FirstUseEver);
	if (ImGui::Begin("エディター"))
	{
		ImGui::Text("%.1f fps", ImGui::GetIO().Framerate);

		if (ImGui::Button(isPaused ? "再開" : "一時停止", {90.f, 0.f,}))
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

		ImGui::BeginDisabled(sceneManager.IsTransitioning());
		const std::string currScene = sceneManager.GetCurrSceneName();
		if (ImGui::BeginCombo("シーン", currScene.c_str()))
		{
			for (const std::string& sceneName : sceneManager.GetSceneNames())
			{
				if (ImGui::Selectable(sceneName.c_str(), sceneName == currScene))
				{
					sceneManager.RequestChange(sceneName);
				}
			}
			ImGui::EndCombo();
		}

		if (ImGui::Button("リロード"))
		{
			sceneManager.RequestChange(currScene);
		}

		ImGui::EndDisabled();

		ImGui::TextDisabled("F1：表示切替");
		ImGui::TextDisabled("一時停止中：クリックで選択／ドラッグで移動");
	}

	ImGui::End();
}

void EditorUI::HandleViewportMouse(Scene& scene, Camera2D& camera)
{
	// 選択・ドラッグは一時停止中のみ（ゲームのクリック操作と競合させない）
	if (!isPaused)
	{
		isDragging = false;
		return;
	}

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
		}
	}

	if (!isDragging)
	{
		return;
	}

	if (!selectedObject || !ImGui::IsMouseDown(ImGuiMouseButton_Left))
	{
		isDragging = false;
		return;
	}

	TransformComponent* transform = selectedObject->GetTransform();
	const DirectX::XMFLOAT3 pos = transform->GetPosition();
	const DirectX::XMFLOAT2 worldPos = ScreenToWorld(mouse, cam, IsObjectScreenSpace(*selectedObject));
	transform->SetPosition({ worldPos.x + objDragOffset.x, worldPos.y + objDragOffset.y, pos.z });
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

GameObject* EditorUI::Pick(Scene& scene, const Camera2D& camera, const DirectX::XMFLOAT2 mousePosScreen)
{
	const DirectX::XMFLOAT2 camPos = camera.GetPosition();
	std::vector<GameObject*> candidates;
	for (auto& obj : scene.GetObjects())
	{
		candidates.push_back(obj.get());
	}

	// topmost first: higher layer first, then higher sort order
	std::ranges::stable_sort(candidates, [](GameObject* a, GameObject* b)
	{
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
