#pragma once

class Camera2D;
class Renderer;
class PhysicsSystem;
class ScriptSystem;
class AnimationSystem;
class RenderSystem;
class Scene;

struct GameContext
{
    Renderer& renderer;
    PhysicsSystem& physicsSys;
    ScriptSystem& scriptSys;
    AnimationSystem& animationSys;
    RenderSystem& renderSys;
    Camera2D& camera;

    Scene* gameScene = nullptr;
};