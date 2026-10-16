#include "PhysicsSystem.h"

#include <algorithm>
#include <chrono>
#include <cmath>

#include "CollisionDispatch.h"
#include "GameObject.h"

namespace
{
	using Clock = std::chrono::steady_clock;

	// returns ms since `mark`, then moves `mark` to now so phases can be chained
	float Lap(Clock::time_point& mark)
	{
		const Clock::time_point now = Clock::now();
		const float ms = std::chrono::duration<float, std::milli>(now - mark).count();
		mark = now;
		return ms;
	}
}

void PhysicsSystem::Register(Rigidbody2DComponent* rigidbody, Collider2D* collider, GameObject* gameObject)
{
	rigidbody->SetInertia(collider->ComputeInertia(rigidbody->GetMass()));
	entries.emplace_back(rigidbody, collider, gameObject);
}

void PhysicsSystem::AttachRBToEntry(GameObject* object, Rigidbody2DComponent* rb)
{
	Entry& entry = GetOrCreate(object);
	entry.rigidbody = rb;

	if (entry.collider)
	{
		rb->SetInertia(entry.collider->ComputeInertia(rb->GetMass()));
	}
}

void PhysicsSystem::AttachColliderToEntry(GameObject* object, Collider2D* col)
{
	Entry& entry = GetOrCreate(object);
	entry.collider = col;

	if (entry.rigidbody)
	{
		entry.rigidbody->SetInertia(col->ComputeInertia(entry.rigidbody->GetMass()));
	}
}

void PhysicsSystem::Unregister(GameObject* gameObject)
{
	std::erase_if(entries, [gameObject](const Entry& entry)
		{
			return entry.gameObject == gameObject;
		});

	const auto matchesPair = [gameObject](const ColliderPair& pair)
		{
			return pair.a == gameObject || pair.b == gameObject;
		};

	std::erase_if(previousOverlaps, matchesPair);
	std::erase_if(currentOverlaps, matchesPair);

	// entries reallocated, so every Entry* the grid holds is now dangling
	grid.clear();
	contacts.clear();
}

void PhysicsSystem::Update(const float deltaTime)
{
	stepsSinceRead += 1;

	const Clock::time_point start = Clock::now();
	Clock::time_point mark = start;

	// 1. integrate
	IntegrateForces(deltaTime);
	debugStats.integrateMs = Lap(mark);

	// 2. broad phase
	RebuildGrid();
	const std::vector<EntryPair> candidatePairs = GenerateCandidatePairs();
	debugStats.broadPhaseMs = Lap(mark);

	// 3. narrow phase -> builds constraints and fires overlap events
	currentOverlaps.clear();
	contacts.clear();
	ProcessPairs(candidatePairs);
	debugStats.narrowPhaseMs = Lap(mark);

	// 4. relax the whole contact set repeatedly.
	for (int iteration = 0; iteration < velocityIterations; iteration++)
	{
		for (ContactConstraint& constraint : contacts)
		{
			SolveVelocityConstraint(constraint);
		}
	}
	debugStats.solveMs = Lap(mark);

	// 5. push bodies out of penetration once, after velocities have settled
	for (const ContactConstraint& constraint : contacts)
	{
		CorrectPositions(constraint);
	}
	debugStats.positionMs = Lap(mark);

	// 6. sleep is evaluated AFTER resolution.
	for (Entry& entry : entries)
	{
		if (entry.rigidbody)
		{
			entry.rigidbody->UpdateSleep(deltaTime);
		}
	}

	// 7. exit events
	for (const ColliderPair& pair : previousOverlaps)
	{
		if (!currentOverlaps.contains(pair))
		{
			pair.a->NotifyColliderLeave2D(*pair.b);
			pair.b->NotifyColliderLeave2D(*pair.a);
		}
	}

	previousOverlaps = currentOverlaps;

	for (Entry& entry : entries)
	{
		if (entry.rigidbody)
		{
			entry.rigidbody->ClearAccumulator();
		}
	}
	debugStats.sleepAndEventsMs = Lap(mark);

	debugStats.totalMs = std::chrono::duration<float, std::milli>(Clock::now() - start).count();

	debugStats.bodies = static_cast<int>(entries.size());
	debugStats.gridCells = static_cast<int>(grid.size());
	debugStats.candidatePairs = static_cast<int>(candidatePairs.size());
	debugStats.contacts = static_cast<int>(contacts.size());
	debugStats.overlaps = static_cast<int>(currentOverlaps.size());
}

void PhysicsSystem::IntegrateForces(const float deltaTime)
{
	for (Entry& entry : entries)
	{
		if (!entry.rigidbody)
		{
			continue;
		}

		entry.rigidbody->Integrate(deltaTime);
	}
}

void PhysicsSystem::RebuildGrid()
{
	grid.clear();

	for (Entry& entry : entries)
	{
		if (!entry.collider || !entry.rigidbody)
		{
			continue;
		}

		if (!entry.collider->IsColliderActive())
		{
			continue;
		}

		const AABB box = entry.collider->GetWorldAABB();

		const CellCoord minCell = GetCellCoord(box.min);
		const CellCoord maxCell = GetCellCoord(box.max);

		for (int cx = minCell.x; cx <= maxCell.x; cx++)
		{
			for (int cy = minCell.y; cy <= maxCell.y; cy++)
			{
				grid[CellCoord{ cx, cy }].push_back(&entry);
			}
		}
	}
}

std::vector<PhysicsSystem::EntryPair> PhysicsSystem::GenerateCandidatePairs()
{
	std::unordered_set<EntryPair, EntryPairHash> dedup;

	debugStats.rawPairChecks = 0;
	debugStats.maxPerCell = 0;

	for (const auto& [cell, entriesInCell] : grid)
	{
		debugStats.maxPerCell = std::max(debugStats.maxPerCell, static_cast<int>(entriesInCell.size()));

		for (size_t i = 0; i < entriesInCell.size(); i++)
		{
			for (size_t j = i + 1; j < entriesInCell.size(); j++)
			{
				debugStats.rawPairChecks += 1;

				Entry* a = entriesInCell[i];
				Entry* b = entriesInCell[j];

				// a pair needs at least one body capable of moving
				// 静止物体同士だけを衝突候補から除外する
				if (a->rigidbody->IsStatic() &&
					b->rigidbody->IsStatic())
				{
					continue;
				}
				dedup.insert(MakeCanonicalEntryPair(a, b));
			}
		}
	}

	return std::vector<EntryPair>(dedup.begin(), dedup.end());
}

void PhysicsSystem::ProcessPairs(const std::vector<EntryPair>& candidates)
{
	for (const EntryPair& pair : candidates)
	{
		// if not close enough for SAT checks skip
		if (!pair.a->collider->GetWorldAABB().IsOverlap(pair.b->collider->GetWorldAABB()))
		{
			continue;
		}

		const std::optional<CollisionManifold> manifold = CollisionDispatch::GetInstance().Test(
			*pair.a->collider, *pair.b->collider);

		// if not touching
		if (!manifold)
		{
			continue;
		}

		const ColliderPair colliderPair = MakeCanonicalColliderPair(pair.a->gameObject, pair.b->gameObject);
		currentOverlaps.insert(colliderPair);

		// event handling
		if (!previousOverlaps.contains(colliderPair))
		{
			pair.a->gameObject->NotifyColliderEnter2D(*pair.b->gameObject);
			pair.b->gameObject->NotifyColliderEnter2D(*pair.a->gameObject);
		}
		else
		{
			pair.a->gameObject->NotifyColliderStay2D(*pair.b->gameObject);
			pair.b->gameObject->NotifyColliderStay2D(*pair.a->gameObject);
		}

		// if either collider is trigger -> skips physics
		if (pair.a->collider->IsTrigger() || pair.b->collider->IsTrigger())
		{
			continue;
		}

		BuildConstraint(pair.a, pair.b, *manifold);
	}
}

void PhysicsSystem::BuildConstraint(Entry* entryA, Entry* entryB, const CollisionManifold& manifold)
{
	Rigidbody2DComponent* a = entryA->rigidbody;
	Rigidbody2DComponent* b = entryB->rigidbody;

	if (manifold.contactCount <= 0)
	{
		return;
	}

	const float invMassSum = a->GetInvMass() + b->GetInvMass();
	if (invMassSum <= 0.f)
	{
		return;
	}

	// wake handling: a sleeper touched by something that can actually move gets
	// woken here, before the solver runs, so the solver never has to care
	const bool aMovable = !a->IsStatic() && !a->IsSleeping();
	const bool bMovable = !b->IsStatic() && !b->IsSleeping();

	if (!aMovable && !bMovable)
	{
		return;
	}

	if (a->IsSleeping() && bMovable)
	{
		a->Wake();
	}

	if (b->IsSleeping() && aMovable)
	{
		b->Wake();
	}

	// manifold normal points A -> B, so a normal pointing "up" means B is
	// resting on A
	if (manifold.normal.y > supportNormalThreshold)
	{
		b->MarkSupported();
	}
	else if (manifold.normal.y < -supportNormalThreshold)
	{
		a->MarkSupported();
	}

	ContactConstraint constraint;
	constraint.a = a;
	constraint.b = b;
	constraint.manifold = manifold;
	constraint.friction = std::sqrt(a->GetFriction() * b->GetFriction());
	constraint.restitution = std::min(a->GetRestitution(), b->GetRestitution());

	const DirectX::XMFLOAT3 posA = a->GetTransformComp().GetPosition();
	const DirectX::XMFLOAT3 posB = b->GetTransformComp().GetPosition();

	const DirectX::XMFLOAT2 normal = manifold.normal;

	// tangent is derived from the NORMAL, not from relative velocity.
	const DirectX::XMFLOAT2 tangent = { -normal.y, normal.x };

	float deepestApproach = 0.f;

	for (int i = 0; i < manifold.contactCount; i++)
	{
		const DirectX::XMFLOAT2 point = manifold.contacts[i];

		constraint.rA[i] = { point.x - posA.x, point.y - posA.y };
		constraint.rB[i] = { point.x - posB.x, point.y - posB.y };

		// effective mass along the normal, including the angular term (r x n)^2 * invI
		const float rACrossN = constraint.rA[i].x * normal.y - constraint.rA[i].y * normal.x;
		const float rBCrossN = constraint.rB[i].x * normal.y - constraint.rB[i].y * normal.x;

		constraint.normalMass[i] = invMassSum
			+ rACrossN * rACrossN * a->GetInvInertia()
			+ rBCrossN * rBCrossN * b->GetInvInertia();

		const float rACrossT = constraint.rA[i].x * tangent.y - constraint.rA[i].y * tangent.x;
		const float rBCrossT = constraint.rB[i].x * tangent.y - constraint.rB[i].y * tangent.x;

		constraint.tangentMass[i] = invMassSum
			+ rACrossT * rACrossT * a->GetInvInertia()
			+ rBCrossT * rBCrossT * b->GetInvInertia();

		const DirectX::XMFLOAT2 velA = a->GetVelAtPoint(constraint.rA[i]);
		const DirectX::XMFLOAT2 velB = b->GetVelAtPoint(constraint.rB[i]);

		const float velAlongNormal = (velB.x - velA.x) * normal.x + (velB.y - velA.y) * normal.y;
		deepestApproach = std::min(deepestApproach, velAlongNormal);
	}

	// a body only "lands" if it arrives fast enough to be worth bouncing
	if (std::abs(deepestApproach) < restitutionSlop)
	{
		constraint.restitution = 0.f;
	}

	contacts.push_back(constraint);
}

void PhysicsSystem::SolveVelocityConstraint(ContactConstraint& constraint) const
{
	Rigidbody2DComponent* a = constraint.a;
	Rigidbody2DComponent* b = constraint.b;

	const DirectX::XMFLOAT2 normal = constraint.manifold.normal;
	const DirectX::XMFLOAT2 tangent = { -normal.y, normal.x };

	for (int i = 0; i < constraint.manifold.contactCount; i++)
	{
		// --- normal impulse ---
		if (constraint.normalMass[i] > 0.f)
		{
			const DirectX::XMFLOAT2 velA = a->GetVelAtPoint(constraint.rA[i]);
			const DirectX::XMFLOAT2 velB = b->GetVelAtPoint(constraint.rB[i]);

			const float velAlongNormal = (velB.x - velA.x) * normal.x + (velB.y - velA.y) * normal.y;

			float lambda = -(1.0f + constraint.restitution) * velAlongNormal / constraint.normalMass[i];

			const float oldImpulse = constraint.normalImpulse[i];
			constraint.normalImpulse[i] = std::max(oldImpulse + lambda, 0.f);
			lambda = constraint.normalImpulse[i] - oldImpulse;

			a->ApplyImpulse({ -normal.x * lambda, -normal.y * lambda }, constraint.rA[i]);
			b->ApplyImpulse({ normal.x * lambda, normal.y * lambda }, constraint.rB[i]);
		}

		// --- friction impulse ---
		if (constraint.tangentMass[i] > 0.f)
		{
			const DirectX::XMFLOAT2 velA = a->GetVelAtPoint(constraint.rA[i]);
			const DirectX::XMFLOAT2 velB = b->GetVelAtPoint(constraint.rB[i]);

			const float velAlongTangent = (velB.x - velA.x) * tangent.x + (velB.y - velA.y) * tangent.y;

			float lambda = -velAlongTangent / constraint.tangentMass[i];

			const float maxFriction = constraint.friction * constraint.normalImpulse[i];

			const float oldImpulse = constraint.tangentImpulse[i];
			constraint.tangentImpulse[i] = std::clamp(oldImpulse + lambda, -maxFriction, maxFriction);
			lambda = constraint.tangentImpulse[i] - oldImpulse;

			a->ApplyImpulse({ -tangent.x * lambda, -tangent.y * lambda }, constraint.rA[i]);
			b->ApplyImpulse({ tangent.x * lambda, tangent.y * lambda }, constraint.rB[i]);
		}
	}
}

void PhysicsSystem::CorrectPositions(const ContactConstraint& constraint) const
{
	Rigidbody2DComponent* a = constraint.a;
	Rigidbody2DComponent* b = constraint.b;

	const float invMassSum = a->GetInvMass() + b->GetInvMass();
	if (invMassSum <= 0.f)
	{
		return;
	}

	const float depth = std::max(constraint.manifold.penetration - positionSlop, 0.f);
	if (depth <= 0.f)
	{
		return;
	}

	const float correction = depth / invMassSum * positionPercent;
	const DirectX::XMFLOAT2 normal = constraint.manifold.normal;

	const DirectX::XMFLOAT3 posA = a->GetTransformComp().GetPosition();
	const DirectX::XMFLOAT3 posB = b->GetTransformComp().GetPosition();

	a->GetTransformComp().SetPosition({
		posA.x - normal.x * correction * a->GetInvMass(),
		posA.y - normal.y * correction * a->GetInvMass(),
		posA.z
		});

	b->GetTransformComp().SetPosition({
		posB.x + normal.x * correction * b->GetInvMass(),
		posB.y + normal.y * correction * b->GetInvMass(),
		posB.z
		});
}

PhysicsSystem::CellCoord PhysicsSystem::GetCellCoord(const DirectX::XMFLOAT2 worldPos) const
{
	return CellCoord{
		.x = static_cast<int>(std::floor(worldPos.x / cellSize)),
		.y = static_cast<int>(std::floor(worldPos.y / cellSize))
	};
}

PhysicsSystem::EntryPair PhysicsSystem::MakeCanonicalEntryPair(Entry* a, Entry* b)
{
	if (a < b)
	{
		return EntryPair{ .a = a, .b = b };
	}

	return EntryPair{ .a = b, .b = a };
}

PhysicsSystem::ColliderPair PhysicsSystem::MakeCanonicalColliderPair(GameObject* a, GameObject* b)
{
	if (a < b)
	{
		return ColliderPair{ .a = a, .b = b };
	}

	return ColliderPair{ .a = b, .b = a };
}

PhysicsSystem::Entry& PhysicsSystem::GetOrCreate(GameObject* object)
{
	for (auto& entry : entries)
	{
		if (entry.gameObject == object)
		{
			return entry;
		}
	}

	entries.push_back(Entry{ nullptr, nullptr, object });

	// push_back may have reallocated, invalidating every Entry* in the grid
	grid.clear();

	return entries.back();
}