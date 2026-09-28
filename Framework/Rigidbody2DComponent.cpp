#include "Rigidbody2DComponent.h"

#include "TransformComponent.h"

Rigidbody2DComponent::Rigidbody2DComponent(TransformComponent& inTransformComp, float inMass, bool inIsStatic) :
	transformComp(inTransformComp),
	mass(inMass),
	isStatic(inIsStatic)
{
	RefreshInv();
}

void Rigidbody2DComponent::AddForce(DirectX::XMFLOAT2 force)
{
	if (isStatic)
	{
		return;
	}

	accumulatedForce.x += force.x;
	accumulatedForce.y += force.y;
}

void Rigidbody2DComponent::AddTorque(float inTorque)
{
	if (invInertia <= 0)
	{
		return;
	}

	accumulatedTorque += inTorque;
}

void Rigidbody2DComponent::ApplyImpulse(DirectX::XMFLOAT2 impulse, DirectX::XMFLOAT2 contactVector)
{
	if (isStatic)
	{
		return;
	}

	velocity.x += impulse.x * invMass;
	velocity.y += impulse.y * invMass;

	angularVel += (contactVector.x * impulse.y - contactVector.y * impulse.x) * invInertia;
}

void Rigidbody2DComponent::Integrate(const float deltaTime)
{
	if (isStatic || isSleeping)
	{
		return;
	}

	// integrate forces
	velocity.x += accumulatedForce.x * invMass * deltaTime;
	velocity.y += (accumulatedForce.y * invMass - gravity) * deltaTime;
	angularVel += accumulatedTorque * invInertia * deltaTime;

	// framerate-independent damping
	const float linDamp = 1.0f / (1.0f + linearDampening * deltaTime);
	const float angDamp = 1.0f / (1.0f + angularDampening * deltaTime);
	velocity.x *= linDamp;
	velocity.y *= linDamp;
	angularVel *= angDamp;

	// tiny snap, only to kill drift — NOT a sleep threshold
	if (std::abs(velocity.x) < snapEpsilon) velocity.x = 0.f;
	if (std::abs(velocity.y) < snapEpsilon) velocity.y = 0.f;
	if (std::abs(angularVel) < snapEpsilon) angularVel = 0.f;

	const DirectX::XMFLOAT3 currPos = transformComp.GetPosition();
	const DirectX::XMFLOAT3 currRot = transformComp.GetRotation();

	transformComp.SetPosition({
		currPos.x + velocity.x * deltaTime,
		currPos.y + velocity.y * deltaTime,
		currPos.z
		});

	transformComp.SetRotation({
		currRot.x,
		currRot.y,
		currRot.z + angularVel * deltaTime
		});
}

void Rigidbody2DComponent::UpdateSleep(const float deltaTime)
{
	if (isStatic)
	{
		return;
	}

	const float speedSq = velocity.x * velocity.x + velocity.y * velocity.y;

	const bool slow = speedSq < sleepLinearThreshold * sleepLinearThreshold
		&& std::abs(angularVel) < sleepAngularThreshold;

	if (slow && hasSupport)
	{
		sleepTimer += deltaTime;
		if (sleepTimer > sleepDelay)
		{
			isSleeping = true;
			velocity = { 0.f, 0.f };
			angularVel = 0.f;
		}
	}
	else
	{
		sleepTimer = 0.f;
	}

	hasSupport = false; // resolver re-sets this each frame
}

void Rigidbody2DComponent::Wake()
{
	isSleeping = false;
	sleepTimer = 0.f;
}

void Rigidbody2DComponent::ClearAccumulator()
{
	accumulatedForce = {0, 0};
	accumulatedTorque = 0.f;
}

void Rigidbody2DComponent::RefreshInv()
{
	if (isStatic)
	{
		invMass = 0;
	}
	else
	{
		invMass = 1.f / mass;
	}
	
	if (isStatic || freezeRotation || inertia <= 0.f)
	{
		invInertia = 0.f;
	}
	else
	{
		invInertia = 1.f / inertia;
	}
}
