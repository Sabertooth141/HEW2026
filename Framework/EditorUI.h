#pragma once
#include <DirectXMath.h>
#include <string>
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
	// UI widgets
	void DrawHierarchy(Scene& scene);
	void DrawInspector(Scene& scene);
	void DrawToolbar(SceneManager& sceneManager);
	void HandleViewportMouse(Scene& scene, Camera2D& camera);
	void DrawSelectionOutline(Camera2D& camera);
	void DrawLayoutSave(SceneManager& sceneManager);
	void DrawPrefabPalette(Scene& scene, const Camera2D& camera);
	void DrawPhysicsDebug(PhysicsSystem& physics);

	void ValidateSelection(Scene& scene);

	GameObject* Pick(Scene& scene, const Camera2D& camera, DirectX::XMFLOAT2 mousePosScreen);

	void EnterPlayMode(SceneManager& sceneManager);
	void EnterEditMode(SceneManager& sceneManager);

	bool CanEditViewport() const
	{
		return mode == Mode::EDIT || isPaused;
	}

private:
	bool isVisible = false;
	bool isPaused = false;

	bool stepRequested = false;

	Scene* lastScene = nullptr;
	GameObject* selectedObject = nullptr;

	bool isDragging = false;
	DirectX::XMFLOAT2 objDragOffset = {};

	std::string saveMessage;
	bool saveValid = true;

	Mode mode = Mode::PLAY;
	std::string playModeStartScene;
};
