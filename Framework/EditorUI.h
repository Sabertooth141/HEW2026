#pragma once
#include <DirectXMath.h>
#include <cstdint>
#include <string>
#include <vector>

#include "RenderSystem.h"
class PhysicsSystem;
class GameObject;
class Scene;
class Camera2D;
class SceneManager;

class EditorUI
{
public:
	enum class Mode
	{
		EDIT,
		PLAY
	};

	void Draw(SceneManager& sceneManager, Camera2D& camera);
	void ToggleVisible();

	bool IsVisible() const
	{
		return isVisible;
	}

	bool IsPaused() const
	{
		return isPaused;
	}

	bool ShouldTick();

	Mode GetMode() const
	{
		return mode;
	}

	bool IsEditMode() const
	{
		return mode == Mode::EDIT;
	}

private:
	// Undo/Redo 用に保存するオブジェクトの状態
	struct ObjectSnapshot
	{
		std::string prefabName;
		std::string name;
		bool isLayoutObject = false;
		DirectX::XMFLOAT3 position = {};
		DirectX::XMFLOAT3 rotation = {};
		DirectX::XMFLOAT3 scale = { 1.f, 1.f, 1.f };
	};

	struct HistoryEntry
	{
		enum class Type
		{
			MODIFY, // 名前・Transform の変更
			CREATE,
			DESTROY
		};

		Type type = Type::MODIFY;
		std::uint64_t instanceId = 0;
		ObjectSnapshot before; // MODIFY：変更前 / DESTROY：削除前
		ObjectSnapshot after; // MODIFY：変更後 / CREATE：生成時
	};

	// UI widgets
	void DrawHierarchy(Scene& scene);
	void DrawInspector(Scene& scene);
	void DrawToolbar(SceneManager& sceneManager);
	void HandleViewportMouse(Scene& scene, Camera2D& camera);
	void DrawSelectionOutline(Camera2D& camera);
	void DrawLayoutSave(SceneManager& sceneManager);
	void DrawPrefabPalette(Scene& scene, const Camera2D& camera);
	void DrawDebug(PhysicsSystem& physics, RenderSystem& render);
	void HandleCamPan(Camera2D& camera);

	void ValidateSelection(Scene& scene);

	GameObject* Pick(Scene& scene, const Camera2D& camera, DirectX::XMFLOAT2 mousePosScreen);

	void EnterPlayMode(SceneManager& sceneManager);
	void EnterEditMode(SceneManager& sceneManager);

	bool CanEditViewport() const
	{
		return mode == Mode::EDIT || isPaused;
	}

	// グリッド・矩形ツール
	bool IsRectToolActive() const
	{
		return IsEditMode() && !rectPrefab.empty();
	}

	bool IsRectPrefab(const std::string& prefabName) const;
	void HandleRectTool(Scene& scene, const Camera2D& camera);
	void CancelRectTool();
	void DrawGrid(const Camera2D& camera);
	void DrawRectPreview(const Camera2D& camera);
	void DrawColliderOutlines(Scene& scene, const Camera2D& camera);
	DirectX::XMFLOAT2 SnapToGrid(DirectX::XMFLOAT2 pos, const DirectX::XMFLOAT3& scale) const;

	// Undo/Redo
	static ObjectSnapshot TakeSnapshot(GameObject& obj);
	static void ApplySnapshot(GameObject& obj, const ObjectSnapshot& snapshot);
	static bool IsSameState(const ObjectSnapshot& a, const ObjectSnapshot& b);
	GameObject* Respawn(Scene& scene, const ObjectSnapshot& snapshot);
	void RemapInstanceId(std::uint64_t oldId, std::uint64_t newId);

	void PushHistory(HistoryEntry entry);
	void RecordModify(GameObject& obj, const ObjectSnapshot& before);
	void RecordCreate(GameObject& obj);
	void RecordDestroy(GameObject& obj);
	void TrackInspectorEdit(GameObject& obj, const ObjectSnapshot& frameStart);

	void Undo(Scene& scene);
	void Redo(Scene& scene);
	void HandleHistoryShortcuts(Scene& scene);
	void ClearHistory();

private:
	bool isVisible = false;
	bool isPaused = false;

	bool stepRequested = false;

	Scene* lastScene = nullptr;
	GameObject* selectedObject = nullptr;

	bool isDragging = false;
	DirectX::XMFLOAT2 objDragOffset = {};
	ObjectSnapshot dragStartSnapshot;

	std::string saveMessage;
	bool saveValid = true;

	Mode mode = Mode::PLAY;
	std::string playModeStartScene;

	bool isPanning = false;

	// グリッド：1マスのサイズ（後で調整できるように変数にしておく）
	float gridSize = 64.f;
	bool isGridEnabled = true;

	// 矩形ツールで配置するプレハブ（ドラッグした範囲に 1 つだけ生成してスケールを合わせる）
	std::vector<std::string> rectPrefabNames = { "platform", "one_way_platform", "breakable_floor", "breakable_wall" };
	std::string rectPrefab;
	bool isRectDragging = false;
	DirectX::XMFLOAT2 rectDragStart = {};

	// Undo/Redo
	std::vector<HistoryEntry> undoHistory;
	std::vector<HistoryEntry> redoHistory;
	size_t historyLimit = 100;
	ObjectSnapshot inspectorEditStart;
};