#pragma once
#include "BoxCollider2D.h"
#include "CollisionDispatch.h"

std::optional<CollisionManifold> TestBoxVsBox(const OBB& a, const OBB& b);

void RegisterCollisionTests();
