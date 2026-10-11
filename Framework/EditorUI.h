#pragma once
#include <DirectXMath.h>
class GameObject;
class Scene;
class Camera2D;
class SceneManager;

class EditorUI
{
public:
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

private:
	// UI widgets
	void DrawHierarchy(Scene& scene);
	void DrawInspector(Scene& scene);
	void DrawToolbar(SceneManager& sceneManager);
	void HandleViewportMouse(Scene& scene, Camera2D& camera);
	void DrawSelectionOutline(Camera2D& camera);

	void ValidateSelection(Scene& scene);

	GameObject* Pick(Scene& scene, const Camera2D& camera, DirectX::XMFLOAT2 mousePosScreen);

private:
	bool isVisible = false;
	bool isPaused = false;

	bool stepRequested = false;

	Scene* lastScene = nullptr;
	GameObject* selectedObject = nullptr;

	bool isDragging = false;
	DirectX::XMFLOAT2 objDragOffset = {};
};
