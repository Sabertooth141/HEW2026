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
	struct DebugStats
	{
		// counts
		int bodies = 0;
		int gridCells = 0;
		int maxPerCell = 0;      // most bodies sharing one cell
		int rawPairChecks = 0;   // i/j iterations before dedup (the n^2 part)
		int candidatePairs = 0;  // after dedup
		int contacts = 0;        // pairs that really touched and became constraints
		int overlaps = 0;        // touching pairs including triggers

		// time of the last step, in milliseconds
		float integrateMs = 0.f;
		float broadPhaseMs = 0.f;
		float narrowPhaseMs = 0.f;
		float solveMs = 0.f;
		float positionMs = 0.f;
		float sleepAndEventsMs = 0.f;
		float totalMs = 0.f;
	};

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

	const DebugStats& GetDebugStats() const
	{
		return debugStats;
	}

	// how many times Update ran since the last call; call once per rendered frame
	int ConsumeStepCount()
	{
		const int steps = stepsSinceRead;
		stepsSinceRead = 0;
		return steps;
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

	// One solvable contact pair. Built once per frame
	struct ContactConstraint
	{
		Rigidbody2DComponent* a = nullptr;
		Rigidbody2DComponent* b = nullptr;

		CollisionManifold manifold;

		// contact offsets from each body's center
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

	float restitutionSlop = 8.f;

	float positionSlop = 0.05f;
	float positionPercent = 0.8f;

	// normal.y beyond this counts as "supported" for sleep purposes (~45 degrees)
	float supportNormalThreshold = 0.7f;

	DebugStats debugStats;
	int stepsSinceRead = 0;
};