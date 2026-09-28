#pragma once
#include <DirectXMath.h>

#include "IComponent.h"
class TransformComponent;

class Rigidbody2DComponent : public IComponent
{
public:
	Rigidbody2DComponent(TransformComponent& inTransformComp, float inMass, bool inIsStatic = false);

	void AddForce(DirectX::XMFLOAT2 force);
	void AddTorque(float inTorque);
	void ApplyImpulse(DirectX::XMFLOAT2 impulse, DirectX::XMFLOAT2 contactVector);

	void SetGravity(const float inGrav)
	{
		gravity = inGrav;
	}

	void SetVelocity(const DirectX::XMFLOAT2 vel)
	{
		velocity = vel;
	}

	void SetAngularVel(const float inAngVel)
	{
		angularVel = inAngVel;
	}

	void SetRestitution(const float inRes)
	{
		restitution = inRes;
	}

	void SetIsStatic(const bool inStatic)
	{
		isStatic = inStatic;
		if (isStatic)
		{
			velocity = {0, 0};
			angularVel = 0.f;
		}
		RefreshInv();
	}

	void SetFreezeRotation(const bool inFreezeRot)
	{
		freezeRotation = inFreezeRot;
		if (freezeRotation)
		{
			angularVel = 0.f;
		}
		RefreshInv();
	}

	void SetInertia(const float inInertia)
	{
		inertia = inInertia;
		RefreshInv();
	}

	void SetMass(const float inMass)
	{
		mass = inMass;
		RefreshInv();
	}

	DirectX::XMFLOAT2 GetVelocity() const
	{
		return velocity;
	}

	float GetAngularVel() const
	{
		return angularVel;
	}

	float GetInvMass() const
	{
		return invMass;
	}

	float GetRestitution() const
	{
		return restitution;
	}

	float GetMass() const
	{
		return mass;
	}

	float GetInvInertia() const
	{
		return invInertia;
	}

	float GetFriction() const
	{
		return friction;
	}

	DirectX::XMFLOAT2 GetVelAtPoint(const DirectX::XMFLOAT2 r) const
	{
		return { velocity.x - angularVel * r.y, velocity.y + angularVel * r.x };
	}

	bool IsStatic() const
	{
		return isStatic;
	}

	TransformComponent& GetTransformComp() const
	{
		return transformComp;
	}

	// physics sys
	void Integrate(float deltaTime);
	void ClearAccumulator();

	void RefreshInv();

	void Wake();
	void UpdateSleep(float deltaTime);
	bool IsSleeping() const { return isSleeping; }
	void MarkSupported() { hasSupport = true; }


private:
	TransformComponent& transformComp;

	// velocities
	DirectX::XMFLOAT2 velocity = {0, 0};
	float angularVel = 0.f;

	// forces
	DirectX::XMFLOAT2 accumulatedForce = {0, 0};
	float accumulatedTorque = 0.f;

	// properties
	float gravity = 200.f;
	float invMass = 1.f; // inverted mass for acceleration calculation F = ma
	bool isStatic = false;
	float mass = 1.f;
	float inertia = 1.f;
	float invInertia = 0.f;
	bool freezeRotation = false;
	float friction = 0.4f;
	bool isSleeping = false;
	bool hasSupport = false;
	float sleepTimer = 0.f;

	float restitution = 0.2f; // bounciness of body
	float linearDampening = 0.1f;
	float angularDampening = 0.1f;

	static constexpr float snapEpsilon = 0.05f;
	static constexpr float sleepLinearThreshold = 8.f;
	static constexpr float sleepAngularThreshold = 0.25f;
	static constexpr float sleepDelay = 0.5f;
};
