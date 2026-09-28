#include "PhysicsTest.h"

#include <DirectXMath.h>

#include "GameObject.h"
#include "WindowSettings.h"

void PhysicsTest::Update(float deltaTime)
{
	MonoBehavior::Update(deltaTime);

	const float halfSprite = owner->GetTransform()->GetScale().x * 0.5f;
	DirectX::XMFLOAT3 pos = owner->GetTransform()->GetPosition();
	//if (pos.x + halfSprite < -WIN_WIDTH / 2.f)
	//{
	//	pos.x = halfSprite + WIN_WIDTH / 2.f;
	//}
	//else if (pos.x - halfSprite > WIN_WIDTH / 2.f)
	//{
	//	pos.x = -halfSprite - WIN_WIDTH / 2.f;
	//}
	//else
	//{
	//	return;
	//}

	owner->GetTransform()->SetPosition(pos);

	if (owner->GetTransform()->GetPosition().y < -800.f)
	{
		DestroySelf();
	}
}
