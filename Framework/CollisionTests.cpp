#include "CollisionTests.h"

namespace
{
	float Dot(const DirectX::XMFLOAT2& a, const DirectX::XMFLOAT2& b)
	{
		return a.x * b.x + a.y * b.y;
	}

	DirectX::XMFLOAT2 Sub(const DirectX::XMFLOAT2& a, const DirectX::XMFLOAT2& b)
	{
		return {a.x - b.x, a.y - b.y};
	}

	DirectX::XMFLOAT2 Normalize(const DirectX::XMFLOAT2& v)
	{
		const float len = std::sqrt(v.x * v.x + v.y * v.y);
		return len > 1e-6f ? DirectX::XMFLOAT2{v.x / len, v.y / len} : DirectX::XMFLOAT2{0, 0};
	}

	struct Face
	{
		DirectX::XMFLOAT2 start, end, normal;
	};

	// GetCorners() returns CCW winding, so the outward normal of edge (s -> e) is (dir.y, -dir.x)
	Face GetFace(const std::array<DirectX::XMFLOAT2, 4>& corners, const int i)
	{
		const DirectX::XMFLOAT2 s = corners[i];
		const DirectX::XMFLOAT2 e = corners[(i + 1) % 4];
		const DirectX::XMFLOAT2 dir = Sub(e, s);

		return Face{s, e, Normalize({dir.y, -dir.x})};
	}

	// the face whose outward normal is most aligned with dir
	int GetBestFaceIndex(const std::array<DirectX::XMFLOAT2, 4>& corners, const DirectX::XMFLOAT2 dir)
	{
		int best = 0;
		float bestDot = -FLT_MAX;

		for (int i = 0; i < 4; i++)
		{
			const float d = Dot(GetFace(corners, i).normal, dir);
			if (d > bestDot)
			{
				bestDot = d;
				best = i;
			}
		}

		return best;
	}

	// keeps the portion of the segment where Dot(n, p) - offset <= 0, returns false if nothing survives
	bool ClipSegment(DirectX::XMFLOAT2& p0, DirectX::XMFLOAT2& p1,
	                 const DirectX::XMFLOAT2 n, const float offset)
	{
		const float d0 = Dot(n, p0) - offset;
		const float d1 = Dot(n, p1) - offset;

		if (d0 <= 0.f && d1 <= 0.f) return true;
		if (d0 > 0.f && d1 > 0.f) return false;

		const float t = d0 / (d0 - d1);
		const DirectX::XMFLOAT2 hit = {p0.x + (p1.x - p0.x) * t, p0.y + (p1.y - p0.y) * t};

		if (d0 > 0.f)
		{
			p0 = hit;
		}
		else
		{
			p1 = hit;
		}

		return true;
	}

	/**
	 * clips the incident box's closest face against the reference box's face
	 *
	 * @param refNormalDir points from the reference box toward the incident box
	 */
	void BuildContacts(const OBB& ref, const OBB& inc,
	                   const DirectX::XMFLOAT2 refNormalDir, CollisionManifold& manifold)
	{
		const auto refCorners = ref.GetCorners();
		const auto incCorners = inc.GetCorners();

		const Face refFace = GetFace(refCorners, GetBestFaceIndex(refCorners, refNormalDir));
		const Face incFace = GetFace(incCorners,
		                             GetBestFaceIndex(incCorners, {-refNormalDir.x, -refNormalDir.y}));

		DirectX::XMFLOAT2 p0 = incFace.start;
		DirectX::XMFLOAT2 p1 = incFace.end;

		// clip against the reference face's two side planes
		const DirectX::XMFLOAT2 tangent = Normalize(Sub(refFace.end, refFace.start));
		const DirectX::XMFLOAT2 negTangent = {-tangent.x, -tangent.y};

		if (!ClipSegment(p0, p1, negTangent, Dot(negTangent, refFace.start))) return;
		if (!ClipSegment(p0, p1, tangent, Dot(tangent, refFace.end))) return;

		// keep only what lies behind the reference face plane
		const float refOffset = Dot(refFace.normal, refFace.start);

		manifold.contactCount = 0;
		for (const DirectX::XMFLOAT2& p : {p0, p1})
		{
			if (refOffset - Dot(refFace.normal, p) >= 0.f)
			{
				manifold.contacts[manifold.contactCount++] = p;
			}
		}

		// degenerate case: never drop the contact entirely or the bodies pass through
		if (manifold.contactCount == 0)
		{
			manifold.contacts[0] = {(p0.x + p1.x) * 0.5f, (p0.y + p1.y) * 0.5f};
			manifold.contactCount = 1;
		}
	}
}

std::optional<CollisionManifold> TestBoxVsBox(const OBB& a, const OBB& b)
{
	const std::array<DirectX::XMFLOAT2, 2> axesA = a.GetAxes();
	const std::array<DirectX::XMFLOAT2, 2> axesB = b.GetAxes();
	const std::array<DirectX::XMFLOAT2, 4> axes =
	{
		axesA[0], axesA[1],
		axesB[0], axesB[1]
	};

	const auto cornersA = a.GetCorners();
	const auto cornersB = b.GetCorners();

	float minOverlap = FLT_MAX;
	DirectX::XMFLOAT2 smallestAxis = {0, 0};

	bool ownerAngled = false;

	// projections
	for (int i = 0; i < 4; i++)
	{
		const DirectX::XMFLOAT2& axis = axes[i];

		float minA = FLT_MAX;
		float maxA = -FLT_MAX;
		for (const auto& corner : cornersA)
		{
			// take dot product of corners projected onto normal
			const float proj = corner.x * axis.x + corner.y * axis.y;
			minA = std::min(minA, proj);
			maxA = std::max(maxA, proj);
		}

		float minB = FLT_MAX;
		float maxB = -FLT_MAX;
		for (const auto& corner : cornersB)
		{
			// take dot product of corners projected onto normal
			const float proj = corner.x * axis.x + corner.y * axis.y;
			minB = std::min(minB, proj);
			maxB = std::max(maxB, proj);
		}

		// test overlap, if no overlap on one axis -> no collision
		const float overlap = std::min(maxA, maxB) - std::max(minA, minB);
		if (overlap <= 0)
		{
			return std::nullopt;
		}

		// find the tightest overlap for physics 
		if (overlap < minOverlap)
		{
			minOverlap = overlap;
			smallestAxis = axis;
			ownerAngled = i < 2;
		}
	}

	CollisionManifold manifold{.normal = smallestAxis, .penetration = minOverlap};

	const DirectX::XMFLOAT2 centerToCenter = {b.center.x - a.center.x, b.center.y - a.center.y};
	if (centerToCenter.x * manifold.normal.x + centerToCenter.y * manifold.normal.y <= 0.0f)
	{
		manifold.normal.x = -manifold.normal.x;
		manifold.normal.y = -manifold.normal.y;
	}

	if (ownerAngled)
	{
		BuildContacts(a, b, manifold.normal, manifold);
	}
	else
	{
		BuildContacts(b, a, { -manifold.normal.x, -manifold.normal.y }, manifold);
	}

	return manifold;
}

void RegisterCollisionTests()
{
	CollisionDispatch::GetInstance().Register(ColliderType::Box, ColliderType::Box,
	                                          [](const Collider2D& a,
	                                             const Collider2D& b) -> std::optional<CollisionManifold>
	                                          {
		                                          const auto& boxA = dynamic_cast<const BoxCollider2D&>(a);
		                                          const auto& boxB = dynamic_cast<const BoxCollider2D&>(b);
		                                          return TestBoxVsBox(boxA.GetWorldOBB(), boxB.GetWorldOBB());
	                                          });
}
