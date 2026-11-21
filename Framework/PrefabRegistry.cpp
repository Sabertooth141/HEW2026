#include "PrefabRegistry.h"

#include "GameObject.h"
#include "PhysicsTest.h"

#include "BoxCollider2D.h"
#include "CameraController.h"
#include "PlayerController.h"
#include "Rigidbody2DComponent.h"
#include "TextLabel.h"
#include "PlatformLayoutComponent.h"
#include "AnimatorComponent.h"
#include "CheckpointComponent.h"
#include "CheckpointLayoutComponent.h"
#include "RetryTransitionComponent.h"
#include "SpeedBoostComponent.h"
#include "BreakableObjectComponent.h"
#include "BreakableLayoutComponent.h"
#include "OneWayPlatformLayoutComponent.h"
#include "PlayerAudioComponent.h"

void RegisterPrefabs()
{
	// player
	PrefabRegistry::Instance().Register("player", [](GameObject& object)
	{
		object.SetTag(ObjectTag::PLAYER);

		// physics
		auto& playerRb = object.AddComponent<Rigidbody2DComponent>(*object.GetTransform(), 1.f);
		playerRb.SetFreezeRotation(true);
		playerRb.SetGravity(800.f);
		object.AddComponent<BoxCollider2D>(
			DirectX::XMFLOAT2(0.3f, 0.4f), DirectX::XMFLOAT2(0.f, 0.f), false, *object.GetTransform());

		// animation
		auto& anim = object.AddComponent<AnimatorComponent>(object.GetRenderer());
		anim.SetRenderLayer(RenderLayer::Player);
		anim.SetSortOrder(0);
		//待機中に表示する
		anim.AddAnimation("StaticRight",L"../../assets/sprite_walk/sprite/spritesheet_right/spritesheet.png",L"../../assets/sprite_walk/sprite/spritesheet_right/spritesheet.json");

		anim.AddAnimation("StaticLeft",L"../../assets/sprite_walk/sprite/spritesheet_left/spritesheet.png",L"../../assets/sprite_walk/sprite/spritesheet_left/spritesheet.json");
		// 歩き。先頭の立ち姿を除き、歩きフレームをループする。
		anim.AddAnimation("WalkRight",L"../../assets/sprite_walk/sprite/spritesheet_right/spritesheet.png",L"../../assets/sprite_walk/sprite/spritesheet_right/spritesheet.json");

		anim.AddAnimation("WalkLeft",L"../../assets/sprite_walk/sprite/spritesheet_left/spritesheet.png",L"../../assets/sprite_walk/sprite/spritesheet_left/spritesheet.json");

		// ジャンプ。左右のシートをそれぞれ一回再生する。
		anim.AddAnimation("JumpRight",L"../../assets/sprite_jump/sprite_jump/spritesheet (3)/spritesheet.png",L"../../assets/sprite_jump/sprite_jump/spritesheet (3)/spritesheet.json");

		anim.AddAnimation("JumpLeft",L"../../assets/sprite_jump/sprite_jump/spritesheet (2)/spritesheet.png",L"../../assets/sprite_jump/sprite_jump/spritesheet (2)/spritesheet.json");

		// 静止絵は0番のまま止める。
		anim.SetAnimationRange("StaticRight", 0, 0, 1.0f, false);
		anim.SetAnimationRange("StaticLeft", 0, 0, 1.0f, false);

		// 歩きは1〜30番を12 FPSでループする。
		anim.SetAnimationRange("WalkRight", 1, 30, 12.0f, true);
		anim.SetAnimationRange("WalkLeft", 1, 30, 12.0f, true);

		// ジャンプは0〜29番を15 FPSで一度再生し、最後の絵で止める。
		anim.SetAnimationRange("JumpRight", 0, 29, 15.0f, false);
		anim.SetAnimationRange("JumpLeft", 0, 29, 15.0f, false);

		// 最初に表示するアニメーションを決める。
		anim.SetCurrAnimation("StaticRight");

		// AddAnimation() が画像のフレーム寸法をScaleへ設定するため、
		// 全アニメーションの登録後にプレイヤーの表示サイズを設定する。
		object.GetTransform()->SetScale({ 80.0f, 80.0f, 1.0f });

		//加速
		object.AddComponent<SpeedBoostComponent>();

		//プレイヤーのSE管理
		object.AddComponent<PlayerAudioComponent>();

		// script
		auto& controller = object.AddComponent<PlayerController>();
		controller.SetInput(object.GetContext().keyboard, object.GetContext().mouse);
		object.AddComponent<CameraController>();

		//トランジション
		object.AddComponent<RetryTransitionComponent>();

		auto& label = object.AddComponent<TextLabel>();
		label.posOffset = { 10.f, 0.f };
		label.isDrawingScreen = true;
	});

	// block (physics sample)
	PrefabRegistry::Instance().Register("block", [](GameObject& object)
	{
		object.SetTag(ObjectTag::BLOCK);

		auto& anim = object.AddComponent<AnimatorComponent>(object.GetRenderer());
		anim.SetRenderLayer(RenderLayer::Default);
		anim.SetSortOrder(0);
		anim.SetStatic(L"../../assets/jinx.jpg");

		object.GetTransform()->SetScale(0.05f);

		auto& rb = object.AddComponent<Rigidbody2DComponent>(*object.GetTransform(), 1.0f);
		rb.SetGravity(500.f);

		object.AddComponent<BoxCollider2D>(
			DirectX::XMFLOAT2(0.5f, 0.5f),
			DirectX::XMFLOAT2(0.f, 0.f),
			false,
			*object.GetTransform());

		object.AddComponent<PhysicsTest>();
	});

	//プラットフォーム
	PrefabRegistry::Instance().Register("platform_layout", [](GameObject& object)
		{
			//object.AddComponent<PlatformLayoutComponent>();
		});
	//編集モードで個別配置する地面
	PrefabRegistry::Instance().Register("platform", [](GameObject& object)
		{
			object.SetTag(ObjectTag::GROUND);

			////地面の表示画像
			//auto& anim =
			//	object.AddComponent<AnimatorComponent>(object.GetRenderer());
			//anim.SetRenderLayer(RenderLayer::Default);
			//anim.SetSortOrder(0);
			////anim.SetStatic(L"../../assets/Ground.png");
			//// 初期サイズ。編集モードのインスペクターから変更できる

			object.GetTransform()->SetScale({ 256.0f, 40.0f, 1.0f });

			// 動かない地面として設定
			auto& platformRb =
				object.AddComponent<Rigidbody2DComponent>(
					*object.GetTransform(),
					1.0f);
			platformRb.SetGravity(0.0f);
			platformRb.SetIsStatic(true);

			// 通常の当たり判定。triggerはfalse
			object.AddComponent<BoxCollider2D>(
				DirectX::XMFLOAT2(0.5f, 0.5f),
				DirectX::XMFLOAT2(0.0f, 0.0f),
				false,
				*object.GetTransform());
		});
	// チェックポイント
	PrefabRegistry::Instance().Register("checkpoint", [](GameObject& object)
		{
			object.SetTag(ObjectTag::CHECKPOINT);

			// チェックポイントの画像とアニメーションを管理する。
			auto& anim = object.AddComponent<AnimatorComponent>(object.GetRenderer());
			anim.SetRenderLayer(RenderLayer::Default);
			anim.SetSortOrder(1);

			// プレイヤーが触れるまでは、これまで使っていた静止画像を表示する。
			anim.SetStatic(L"../../assets/flag.png");

			// プレイヤーが触れた後に再生する旗のアニメーションを登録する。
			anim.AddAnimation(
				"Activated",
				L"../../assets/sprite_ship/sprite_ship/sprite_ship/spritesheet.png",
				L"../../assets/sprite_ship/sprite_ship/sprite_ship/spritesheet.json");

			// 30フレームを12 FPSで繰り返し再生する。
			anim.SetAnimationRange("Activated", 0, 29, 12.0f, true);

			// 静止画像・シート画像の登録で変わったサイズを、表示用の大きさに設定する。
			object.GetTransform()->SetScale({ 72.0f, 72.0f, 1.0f });

			// チェックポイントを物理的に動かない状態にする。
			auto& checkpointRb =
				object.AddComponent<Rigidbody2DComponent>(*object.GetTransform(), 1.0f);
			checkpointRb.SetGravity(0.0f);
			checkpointRb.SetIsStatic(true);

			// プレイヤーが触れたことを検知するTrigger判定。
			object.AddComponent<BoxCollider2D>(
				DirectX::XMFLOAT2(0.5f, 0.5f),
				DirectX::XMFLOAT2(0.0f, 0.0f),
				true,
				*object.GetTransform());

			// 接触時の復帰地点登録と、アニメーション開始を担当する。
			object.AddComponent<CheckpointComponent>();
		});
	PrefabRegistry::Instance().Register("checkpoint_layout", [](GameObject& object)
		{
			object.AddComponent<CheckpointLayoutComponent>();
		});

	// 壊せるオブジェクトの配置
	PrefabRegistry::Instance().Register("breakable_layout", [](GameObject& object)
		{
			object.AddComponent<BreakableLayoutComponent>();
		});

	//針
	PrefabRegistry::Instance().Register("spike", [](GameObject& object)
		{
			object.SetTag(ObjectTag::SPIKE);
			object.GetTransform()->SetScale({ 64.f, 64.f, 1.f });
		
			auto& spikeRb = object.AddComponent<Rigidbody2DComponent>(
				*object.GetTransform(), 1.0f);

			spikeRb.SetGravity(0.0f);
			spikeRb.SetIsStatic(true);

			// 当たり判定
			object.AddComponent<BoxCollider2D>(
				DirectX::XMFLOAT2(0.5f, 0.5f),
				DirectX::XMFLOAT2(0.0f, 0.0f),
				true,
				*object.GetTransform());
		});




	// 壊れる床
	PrefabRegistry::Instance().Register("breakable_floor", [](GameObject& object)
		{
			object.SetTag(ObjectTag::GROUND);

			auto& rb = object.AddComponent<Rigidbody2DComponent>(
				*object.GetTransform(), 1.0f);

			rb.SetGravity(0.0f);
			rb.SetIsStatic(true);

			object.AddComponent<BoxCollider2D>(
				DirectX::XMFLOAT2(0.5f, 0.5f),
				DirectX::XMFLOAT2(0.0f, 0.0f),
				false,
				*object.GetTransform());

			auto& breakable = object.AddComponent<BreakableObjectComponent>();
			breakable.SetBreakType(
				BreakableObjectComponent::BreakType::Dive);
		});

	// 壊れる壁
	PrefabRegistry::Instance().Register("breakable_wall", [](GameObject& object)
		{
			object.SetTag(ObjectTag::GROUND);

			auto& rb = object.AddComponent<Rigidbody2DComponent>(
				*object.GetTransform(), 1.0f);

			rb.SetGravity(0.0f);
			rb.SetIsStatic(true);

			object.AddComponent<BoxCollider2D>(
				DirectX::XMFLOAT2(0.5f, 0.5f),
				DirectX::XMFLOAT2(0.0f, 0.0f),
				false,
				*object.GetTransform());

			auto& breakable = object.AddComponent<BreakableObjectComponent>();
			breakable.SetBreakType(
				BreakableObjectComponent::BreakType::Boost);
		});



	// 片方向足場
	PrefabRegistry::Instance().Register("one_way_platform",
		[](GameObject& object)
		{
			object.SetTag(ObjectTag::ONE_WAY_PLATFORM);

			auto& rb = object.AddComponent<Rigidbody2DComponent>(
				*object.GetTransform(), 1.0f);

			rb.SetGravity(0.0f);
			rb.SetIsStatic(true);

			object.AddComponent<BoxCollider2D>(
				DirectX::XMFLOAT2(0.5f, 0.5f),
				DirectX::XMFLOAT2(0.0f, 0.0f),
				false,
				*object.GetTransform());
		});

	// 片方向足場の配置
	PrefabRegistry::Instance().Register("one_way_platform_layout",
		[](GameObject& object)
		{
			//object.AddComponent<OneWayPlatformLayoutComponent>();
		});
}

