#pragma once
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Collider2D.h"
#include "Rigidbody2DComponent.h"

class GameObject;

class PhysicsSystem
{
public:
	void Register(Rigidbody2DComponent* rigidbody, Collider2D* collider, GameObject* gameObject);

	void AttachRBToEntry(GameObject* object, Rigidbody2DComponent* rb);
	void AttachColliderToEntry(GameObject* object, Collider2D* col);

	void Unregister(GameObject* gameObject);

	void Update(float deltaTime);

	// tuning
	void SetVelocityIterations(const int inIterations)
	{
		velocityIterations = inIterations;
	}

	void SetRestitutionSlop(const float inSlop)
	{
		restitutionSlop = inSlop;
	}

private:
	// for spatial partitioning
	struct Entry
	{
		Rigidbody2DComponent* rigidbody = nullptr;
		Collider2D* collider = nullptr;
		GameObject* gameObject = nullptr;
	};

	// grid
	struct CellCoord
	{
		int x, y;

		bool operator==(const CellCoord& other) const
		{
			return x == other.x && y == other.y;
		}
	};

	struct CellCoordHash
	{
		size_t operator()(const CellCoord& cell) const
		{
			return std::hash<int>()(cell.x) ^ (std::hash<int>()(cell.y) << 1);
		}
	};

	// candidate pair for comparison
	struct EntryPair
	{
		Entry* a;
		Entry* b;

		bool operator==(const EntryPair& other) const
		{
			return a == other.a && b == other.b;
		}
	};

	struct EntryPairHash
	{
		size_t operator()(const EntryPair& pair) const
		{
			return std::hash<Entry*>()(pair.a) ^ (std::hash<Entry*>()(pair.b) << 1);
		}
	};

	// persistent pairs
	struct ColliderPair
	{
		GameObject* a;
		GameObject* b;

		bool operator==(const ColliderPair& other) const
		{
			return a == other.a && b == other.b;
		}
	};

	struct ColliderPairHash
	{
		size_t operator()(const ColliderPair& pair) const
		{
			return std::hash<GameObject*>()(pair.a) ^ (std::hash<GameObject*>()(pair.b) << 1);
		}
	};

	// One solvable contact pair. Built once per frame, then relaxed over several
	// iterations. The accumulated impulses are what make multi-point manifolds
	// stable: each iteration applies only the *delta* needed to reach the clamped
	// total, so the two contact points stop undoing each other's work.
	struct ContactConstraint
	{
		Rigidbody2DComponent* a = nullptr;
		Rigidbody2DComponent* b = nullptr;

		CollisionManifold manifold;

		// contact offsets from each body's center, cached (positions don't move
		// during velocity iterations)
		DirectX::XMFLOAT2 rA[2]{};
		DirectX::XMFLOAT2 rB[2]{};

		// effective masses along normal / tangent, cached
		float normalMass[2]{};
		float tangentMass[2]{};

		// accumulated impulses across iterations
		float normalImpulse[2]{};
		float tangentImpulse[2]{};

		float restitution = 0.f;
		float friction = 0.f;
	};

	// logic

	void IntegrateForces(float deltaTime);
	void RebuildGrid();
	std::vector<EntryPair> GenerateCandidatePairs();
	void ProcessPairs(const std::vector<EntryPair>& candidates);

	void BuildConstraint(Entry* entryA, Entry* entryB, const CollisionManifold& manifold);
	void SolveVelocityConstraint(ContactConstraint& constraint) const;
	void CorrectPositions(const ContactConstraint& constraint) const;

	// helpers

	CellCoord GetCellCoord(DirectX::XMFLOAT2 worldPos) const;

	static EntryPair MakeCanonicalEntryPair(Entry* a, Entry* b);
	static ColliderPair MakeCanonicalColliderPair(GameObject* a, GameObject* b);

private:
	Entry& GetOrCreate(GameObject* object);

private:
	std::vector<Entry> entries;

	std::unordered_map<CellCoord, std::vector<Entry*>, CellCoordHash> grid;
	float cellSize = 64.f;

	std::unordered_set<ColliderPair, ColliderPairHash> currentOverlaps;
	std::unordered_set<ColliderPair, ColliderPairHash> previousOverlaps;

	std::vector<ContactConstraint> contacts;

	// solver tuning
	int velocityIterations = 8;

	// Below this approach speed, restitution is forced to 0. Without this, the
	// -gravity * dt that integration injects every frame gets bounced back as
	// +restitution * gravity * dt and a resting body never settles.
	// Rule of thumb: ~2 * gravity * fixedDeltaTime.
	float restitutionSlop = 8.f;

	float positionSlop = 0.05f;
	float positionPercent = 0.8f;

	// normal.y beyond this counts as "supported" for sleep purposes (~45 degrees)
	float supportNormalThreshold = 0.7f;
};