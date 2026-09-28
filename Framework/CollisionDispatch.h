#pragma once
#include <functional>
#include <optional>

#include "Collider2D.h"

using NarrowPhaseTest = std::function<std::optional<CollisionManifold>(const Collider2D&, const Collider2D&)>;

class CollisionDispatch
{
public:
	static CollisionDispatch& GetInstance()
	{
		static CollisionDispatch instance;
		return instance;
	}

	/**
	 * registers the collision test between two collider types
	 * 
	 * @param typeA collider type for A
	 * @param typeB collider type for B
	 * @param test test to be run between the two colliders at narrow phase collision test
	 */
	void Register(const ColliderType typeA, const ColliderType typeB, const NarrowPhaseTest& test)
	{
		table[{typeA, typeB}] = test;

		if (typeA != typeB)
		{
			table[{typeB, typeA}] = [test](const Collider2D& a, const Collider2D& b) -> std::optional<CollisionManifold>
			{
				auto result = test(b, a);
				if (result)
				{
					result->normal.x = -result->normal.x;
					result->normal.y = -result->normal.y;
				}
				return result;
			};
		}
	}

	std::optional<CollisionManifold> Test(const Collider2D& a, const Collider2D& b)
	{
		const auto function = table.find({ a.GetType(), b.GetType() });

		if (function == table.end())
		{
			return std::nullopt;
		}

		return function->second(a, b);
	}

private:
	struct TypePair
	{
		ColliderType a, b;

		bool operator==(const TypePair& other) const
		{
			return a == other.a && b == other.b;
		}
	};

	struct TypePairHash
	{
		size_t operator()(const TypePair& p) const
		{
			size_t h1 = std::hash<int>()(static_cast<int>(p.a));
			size_t h2 = std::hash<int>()(static_cast<int>(p.b));
			// Golden ratio constant prevents bit loss and scatters entropy evenly
			return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
		}
	};

	std::unordered_map<TypePair, NarrowPhaseTest, TypePairHash> table;
};
