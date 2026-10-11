#include "MonoBehavior.h"

#include "GameObject.h"
#include "Scene.h"
#include "SceneManager.h"

void MonoBehavior::ChangeScene(const std::string& sceneName, const float exitTime, const float enterTime) const
{
	owner->GetContext().sceneManager->RequestChange(sceneName, exitTime, enterTime);
}

void MonoBehavior::SetInput(Keyboard& inKeyboard, Mouse& inMouse)
{
	keyboard = &inKeyboard;
	mouse = &inMouse;
}

GameObject* MonoBehavior::Instantiate(const std::string& prefab, const DirectX::XMFLOAT3& pos) const
{
	return owner->GetContext().gameScene->Instantiate(prefab, pos);
}

void MonoBehavior::Destroy(GameObject* object) const
{
	owner->GetContext().gameScene->Destroy(object);
}

void MonoBehavior::DestroySelf() const
{
	Destroy(owner);
}
