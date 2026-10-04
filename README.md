# HEW2026

コンポーネント指向（COP）で作った DirectX 11 の 2D ゲームフレームワークです。
`GameObject` に `Component` を追加して機能を組み立てる Unity ライクな構造になっています。

現在は 2D 機能（スプライト・アニメーション・2D 物理・テキスト描画）が中心です。3D（Assimp による読み込み）は下地のみ実装済みです。

---

## 目次

1. [開発環境・ビルド](#1-開発環境ビルド)
2. [全体構成とフレームの流れ](#2-全体構成とフレームの流れ)
3. [座標系](#3-座標系)
4. [クイックスタート](#4-クイックスタート)
5. [Scene / GameObject](#5-scene--gameobject)
6. [TransformComponent](#6-transformcomponent)
7. [スプライトとアニメーション](#7-スプライトとアニメーション)
8. [物理（Rigidbody2D / BoxCollider2D）](#8-物理rigidbody2d--boxcollider2d)
9. [スクリプト（MonoBehavior）](#9-スクリプトmonobehavior)
10. [プレハブ（PrefabRegistry）](#10-プレハブprefabregistry)
11. [カメラ](#11-カメラ)
12. [入力（Keyboard / Mouse）](#12-入力keyboard--mouse)
13. [テキスト描画（TextRenderer）](#13-テキスト描画textrenderer)
14. [デバッグ描画（DebugRenderer）](#14-デバッグ描画debugrenderer)
15. [テクスチャ・Renderer](#15-テクスチャrenderer)
16. [拡張方法](#16-拡張方法)
17. [注意点](#17-注意点)

---

## 1. 開発環境・ビルド

| 項目 | 内容 |
|---|---|
| IDE / ツールセット | Visual Studio（PlatformToolset `v145`） |
| 言語標準 | C++20 |
| プラットフォーム | x64（Debug / Release） |
| 文字コード | ソースは UTF-8（`/utf-8` オプション有効） |
| 依存ライブラリ | Assimp（vcpkg マニフェスト `vcpkg.json`）、stb_image（同梱） |
| ウィンドウサイズ | `WindowSettings.h` の `WIN_WIDTH` / `WIN_HEIGHT`（800 x 600） |

### ビルド手順

1. リポジトリをクローンし、`DXPractice.slnx` を Visual Studio で開く。
2. 構成を `x64` の `Debug` か `Release` にする。
3. ビルドすると vcpkg が Assimp を自動で取得します（`vcpkg_installed/` は Git 管理外）。
4. HLSL は `.cso` にコンパイルされ、実行時に `D3DReadFileToBlob` で読み込まれます（`VertexShader.cso` など）。実行ファイルと同じ場所に出力されます。
5. 画像などは `../../assets/` からの相対パスで読み込みます（例：`L"../../assets/jinx.jpg"`）。実行時のカレントディレクトリに注意してください。

---

## 2. 全体構成とフレームの流れ

```
WinMain → App::Run()
            ├─ Init()            … プレハブ・コリジョンテスト・シーンの登録、最初のシーンへ遷移
            └─ ループ
                 ├─ Window::ProcessMessages()
                 ├─ Update(deltaTime)
                 ├─ Draw(deltaTime)
                 └─ keyboard.EndFrame()
```

### `App::Update` の順序

```
// 遷移中（EXITING / ENTERING）はここをスキップ
scene->Update(dt)            // 各 GameObject の Update
scene->OnUpdate(dt)          // シーン固有の処理（タイマー・遷移判定など）
scriptSystem.Update(dt)      // Start（初回のみ）→ Update
physicsSystem.Update(dt)     // 積分 → 衝突判定 → 拘束解決 → イベント通知
scriptSystem.LateUpdate(dt)  // LateUpdate（カメラ追従など）
animationSystem.Update(dt)   // アニメーション更新
scene->FlushPending()        // 生成・破棄の反映

sceneManager.Update(dt)      // 遷移の進行・シーン切り替え（常に最後）
```

### `App::Draw` の順序

```
renderSystem.Render()        // スプライト
DebugRender / DebugTextRender
scene->OnDrawUI(text)        // シーンの HUD・画面テキスト
debugRenderer.Flush()
textRenderer.Flush()
```

### システム一覧

| システム | 役割 |
|---|---|
| `ScriptSystem` | `MonoBehavior` の Awake / Start / Update / LateUpdate / OnDestroy を管理 |
| `PhysicsSystem` | 2D 剛体の積分、グリッド方式ブロードフェーズ、SAT 判定、インパルス解決、スリープ |
| `AnimationSystem` | `AnimatorComponent` の更新 |
| `RenderSystem` | 2D スプライトの描画（レイヤー → ソート順で並べ替え） |

これらと `Keyboard` / `Mouse`、`SceneManager` は `GameContext` 構造体にまとめられ、`GameObject` から `GetContext()` で参照できます。

---

## 3. 座標系

- 2D 描画は正射影（幅 `WIN_WIDTH` x 高さ `WIN_HEIGHT`）です。
- **原点は画面中央、+X が右、+Y が上**、単位はピクセル相当です。
- スプライトの Z は `1` にしておきます（カメラは Z = -5 から +Z 方向を見ています）。
- 回転は **ラジアン**。2D では `rotation.z` を使います。
- `TextRenderer::DrawScreen` だけは例外で、**左上原点・+Y が下**のピクセル座標です。

---

## 4. クイックスタート

シーンの `OnEnter()` 内で、画像付きの落下する箱を作る例です（`scene.` は不要で、シーンのメンバー関数として呼べます）。

```cpp
// 1. オブジェクトを生成（2D 用のクアッドメッシュ付き）
GameObject* obj = Add2DObject();
obj->GetTransform()->SetPosition({0.f, 100.f, 1.f});

// 2. スプライト
auto& anim = obj->AddComponent<AnimatorComponent>(obj->GetRenderer());
anim.SetRenderLayer(RenderLayer::Default);
anim.SetSortOrder(0);
anim.SetStatic(L"../../assets/jinx.jpg");   // ← スケールが画像サイズになる
obj->GetTransform()->SetScale(0.1f);        // ← その後で縮小

// 3. 物理（Rigidbody → Collider の順で追加）
auto& rb = obj->AddComponent<Rigidbody2DComponent>(*obj->GetTransform(), 1.0f);
rb.SetGravity(500.f);
obj->AddComponent<BoxCollider2D>(
    DirectX::XMFLOAT2(0.5f, 0.5f),   // halfExtents（スケール倍される）
    DirectX::XMFLOAT2(0.f, 0.f),     // offset
    false,                           // isTrigger
    *obj->GetTransform());

// 4. スクリプト
obj->AddComponent<PhysicsTest>();
```

---

## 5. Scene / GameObject

### Scene

| 関数 | 説明 |
|---|---|
| `GameObject* Add2DObject()` | 2D オブジェクトを即座にシーンへ追加して返す（初期化用） |
| `GameObject* Instantiate(const std::string& prefab, const XMFLOAT3& pos)` | 登録済みプレハブから生成。**シーンへの追加は `FlushPending()` で行われる**（フレーム末） |
| `void Destroy(GameObject* object)` | 破棄を予約。同じオブジェクトを複数回渡しても 1 回だけ。`FlushPending()` で実際に削除 |
| `void Update(float dt)` | 全オブジェクトを更新 |
| `void FlushPending()` | 生成・破棄の反映。破棄時は物理・スクリプト・アニメーション・描画の各システムから登録解除 |
| `GetObjects()` | 現在のオブジェクト一覧を取得 |
| `void Clear()` | 全オブジェクトを各システムから登録解除して破棄（シーン切り替え時に `SceneManager` が呼ぶ） |

#### シーンのフック（override して使う）

| 関数 | 呼ばれるタイミング |
|---|---|
| `OnEnter()` | シーン開始時。**オブジェクトの生成はここで行う** |
| `OnExit()` | シーン終了時（`Clear()` の直前） |
| `OnUpdate(float dt)` | 毎フレーム（遷移中は呼ばれない）。タイマーや遷移判定など |
| `OnDrawUI(TextRenderer& text)` | 毎フレームの描画。HUD・画面テキスト |

シーンのメンバーとして `context`（`GameContext&`）が使えます。

### シーンの作成

```cpp
// StageScene.h
#pragma once
#include "Scene.h"

class StageScene : public Scene
{
public:
    using Scene::Scene;

    void OnEnter() override;
    void OnUpdate(float dt) override;
    void OnDrawUI(TextRenderer& text) override;
};
```

```cpp
// StageScene.cpp
void StageScene::OnEnter()
{
    GameObject* player = Add2DObject();
    // ...
}

void StageScene::OnUpdate(float dt)
{
    if (/* クリア条件 */)
        context.sceneManager->RequestChange("Result");
}
```

### SceneManager（シーン管理）

シーンは**文字列名**で登録し、名前で切り替えます。シーンは同時に 1 つだけ存在し、切り替えるたびに新しく生成されます。

#### 登録（`App::Init`）

```cpp
sceneManager.Register<TitleScene>("Title");
sceneManager.Register<StageScene>("Stage");
sceneManager.Register<ResultScene>("Result");

sceneManager.RequestChange("Title");   // 最初のシーン
```

`Register<T>(name)` は `T` を生成する関数（ファクトリ）を登録するだけで、シーン自体はまだ作られません。コンストラクタに追加の引数が必要な場合はラムダで登録します。

```cpp
sceneManager.Register("Stage2", [](GameContext& c) {
    return std::make_unique<StageScene>(c /*, 追加の引数 */);
});
```

#### 関数一覧

| 関数 | 説明 |
|---|---|
| `Register(name)` / `Register(name, factory)` | シーンを名前で登録 |
| `IsRegistered(name)` | 登録済みか |
| `RequestChange(name, exitTime = 0, enterTime = 0)` | 遷移を**予約**（即座には切り替わらない）。遷移中の呼び出しは無視。未登録の名前は `assert` |
| `Update(dt)` | 遷移を進め、切り替えを実行（`App::Update` の最後で呼ばれる） |
| `GetCurrScene()` / `GetCurrSceneName()` | 現在のシーン・名前 |
| `GetNextSceneName()` | 遷移先のシーン名 |
| `GetTransitionState()` | 遷移の状態（下表） |
| `IsTransitioning()` | 遷移中か（`NONE` 以外） |
| `GetTransitionProgress()` | 現在のフェーズの進行度（0 → 1）。遷移していないときは 0 |

#### 遷移の状態（`SceneManager::TransitionState`）

| 状態 | 意味 |
|---|---|
| `NONE` | 通常状態 |
| `EXITING` | 旧シーンが残っている状態。`exitTime` 秒後に切り替わる |
| `ENTERING` | 新シーンの読み込み後。`enterTime` 秒後に `NONE` に戻る |

`exitTime` / `enterTime` が 0 のフェーズはスキップされます。両方 0（デフォルト）なら、`RequestChange` を呼んだフレームの最後で即座に切り替わります。

#### 遷移の流れ

```
RequestChange("Result", exitTime, enterTime)
  → EXITING（exitTime 秒。ゲーム処理は停止）
  → 現シーンの OnExit() → Clear() → 破棄
  → カメラ位置を (0, 0) にリセット
  → 新シーン生成 → context.gameScene を更新 → OnEnter()
  → ENTERING（enterTime 秒。ゲーム処理は停止）
  → NONE（通常更新に戻る）
```

最初のシーン（`App::Init` での `RequestChange`）は `EXITING` を経由せず、すぐに読み込まれます。

#### 遷移演出（フェードなど）の実装

`SceneManager` 自体は描画を行いません。フェードやワイプなどの演出は、状態と進行度を参照して別途描画してください。

```cpp
using TS = SceneManager::TransitionState;

const float t = sceneManager.GetTransitionProgress();
float alpha = 0.f;
if (sceneManager.GetTransitionState() == TS::EXITING)  alpha = t;        // 0 → 1
if (sceneManager.GetTransitionState() == TS::ENTERING) alpha = 1.f - t;  // 1 → 0
// alpha で画面全体を覆う矩形を描画
```

> 演出はシーンの**外**（`App` など）に置いてください。シーン内のオブジェクトは遷移の途中で破棄されます。

#### 呼び出し方

```cpp
// シーンから
context.sceneManager->RequestChange("Result");             // 即座に切り替え
context.sceneManager->RequestChange("Result", 0.5f, 0.5f); // 0.5 秒ずつ EXITING / ENTERING

// MonoBehavior から
ChangeScene("Result");
ChangeScene("Result", 0.5f, 0.5f);
```

#### シーン間のデータ受け渡し

シーン内のものは切り替え時にすべて破棄されます。次のシーンに渡したい値は `GameContext::shared`（`SharedGameData`）に入れてください。

```cpp
// GameContext.h
struct GlobalContext
{
    int score = 0;
};

// StageScene
context.globalContext.score = currentScore;
context.sceneManager->RequestChange("Result");

// ResultScene::OnDrawUI
text.DrawScreen(std::format("スコア: {}", context.shared.score), ...);
```

#### 起動シーンの指定（デバッグ用）

コマンドライン引数に登録済みのシーン名を渡すと、そのシーンから起動します（例：`Stage`）。
Visual Studio の「プロジェクトのプロパティ → デバッグ → コマンド引数」で設定できます。未登録の名前や空の場合は `Title` から起動します。

### GameObject

| 関数 | 説明 |
|---|---|
| `T& AddComponent(args...)` | コンポーネントを追加。型に応じて自動でシステムに登録される（下表） |
| `T* GetComponent()` | コンポーネント取得。なければ `nullptr` |
| `TransformComponent* GetTransform()` | Transform 取得 |
| `GetMeshes()` | メッシュ一覧 |
| `GetRenderer()` / `GetContext()` | Renderer / GameContext の参照 |
| `GetTag()` / `SetTag(ObjectTag)` | タグ（`Default / Enemy / Player / Block / Ground`） |

#### `AddComponent` の自動登録

| 追加するコンポーネント | 登録先 |
|---|---|
| `MonoBehavior` 派生 | `ScriptSystem`（追加時に `Awake` 呼び出し） |
| `AnimatorComponent` | `AnimationSystem` と `RenderSystem` |
| `Rigidbody2DComponent` | `PhysicsSystem` |
| `Collider2D` 派生（`BoxCollider2D`） | `PhysicsSystem` |

> **ポイント**
> 
> - `GetComponent<T>()` は **型を完全一致**で検索します。基底クラス（例：`MonoBehavior`）で派生を探すことはできません。
> - 同じ型のコンポーネントは 1 つのみ（後から追加すると上書き）。
> - `Awake` は `AddComponent` の**その場**で呼ばれるため、`Awake` 内で `GetComponent` したい相手は**先に追加**しておいてください（例：`PlayerController::Awake` は `Rigidbody2DComponent` を取得するので、先に Rigidbody を追加）。

---

## 6. TransformComponent

| 関数 | 説明 |
|---|---|
| `SetPosition(XMFLOAT3)` | 位置を設定 |
| `SetRotation(XMFLOAT3)` | 回転（ラジアン）。2D は `z` |
| `SetScale(XMFLOAT3)` | スケールを強制設定（縦横比を無視） |
| `SetScale(float s)` | **現在のスケールに `s` を掛ける**（現在の縦横比を維持） |
| `Rotate(XMFLOAT3)` / `Translate(XMFLOAT3)` | 相対回転・相対移動 |
| `GetPosition()` / `GetRotation()` / `GetScale()` | 取得 |
| `GetMatrix()` | ワールド行列（Scale → Rotation → Translation） |

> スプライトのスケールは、画像またはアニメーションフレームの**ピクセルサイズ**になります。
> `SetStatic` / `AddAnimation` の**後で** `SetScale(float)` を呼ぶと縮尺を調整できます。

---

## 7. スプライトとアニメーション

### AnimatorComponent

```cpp
auto& anim = obj->AddComponent<AnimatorComponent>(obj->GetRenderer());
```

| 関数 | 説明 |
|---|---|
| `SetStatic(path)` | 1 枚絵を設定（アニメーションなし）。スケールを画像サイズにする |
| `AddAnimation(name, spritePath, jsonPath)` | スプライトシートとフレーム情報 JSON からアニメーションを追加。**同名を追加すると例外** |
| `SetCurrAnimation(name)` | 再生するアニメーションを切り替え。存在しなければ `false` |
| `GetCurrAnimName()` | 現在のアニメーション名 |
| `SetRenderLayer(RenderLayer)` | 描画レイヤー |
| `SetSortOrder(int)` | 同一レイヤー内の描画順（小さいほど奥） |
| `SetFlipX(bool)` / `SetFlipY(bool)` | 反転 |
| `SetEnabled(bool)` / `IsEnabled()` | 有効／無効 |

#### アニメーション使用例（デモの `GameScene::OnEnter` より）

```cpp
auto* anim = playerObj->GetComponent<AnimatorComponent>();
anim->SetRenderLayer(RenderLayer::Player);
anim->AddAnimation("CharIdle", L"../../assets/PlayerCharacter.png",
                               L"../../assets/PlayerCharacter.json");
anim->AddAnimation("CharMove", L"../../assets/PlayerCharacterMove.png",
                               L"../../assets/PlayerCharacterMove.json");
anim->SetCurrAnimation("CharIdle");
```

#### JSON の形式

Aseprite の書き出し（Array 形式）を想定した簡易パーサです。

- `"frames"` 配下の各 `"frame": {x, y, w, h}` と `"duration"`（ミリ秒）
- `"meta"` 内の `"size": {w, h}`（シート全体サイズ）
- 任意で `"Loop"` タグの `"from"` / `"to"`（ループ範囲）

### RenderLayer

| 値 | 数値 |
|---|---|
| `BackGround` | 0 |
| `Default` | 10 |
| `Player` | 20 |
| `Enemy` | 30 |
| `ForeGround` | 40 |
| `UI` | 100 |

描画順は「レイヤー → ソート順」の安定ソートです。ソートは**オブジェクトの登録／登録解除時のみ**再実行されるので、実行中にレイヤーを変えても並び順は次の登録変更まで反映されません。

### スプライトシェーダー

`SpriteVertexShader.hlsl` / `SpritePixelShader.hlsl` を使用します。

- `b0`：Transform（world / view / projection）
- `b2`：UV オフセット・スケール（アニメーション用）
- `b3`：反転フラグ（`Renderer::SetSpriteFlip`）
- アルファが 0.1 以下のピクセルは `clip`（破棄）されます。
- サンプラーはポイントフィルタ（ドット絵向け）です。

---

## 8. 物理（Rigidbody2D / BoxCollider2D）

物理を有効にするには **`Rigidbody2DComponent` と `BoxCollider2D` の両方**が必要です（どちらか片方だけだと衝突判定の対象になりません）。

### Rigidbody2DComponent

```cpp
Rigidbody2DComponent(TransformComponent& transform, float mass, bool isStatic = false);
```

| 関数 | 説明 |
|---|---|
| `AddForce(XMFLOAT2)` | 力を加える（フレーム末にクリア） |
| `AddTorque(float)` | トルクを加える |
| `ApplyImpulse(impulse, contactVector)` | 接触点にインパルスを加える |
| `SetVelocity(XMFLOAT2)` / `GetVelocity()` | 速度 |
| `SetAngularVel(float)` / `GetAngularVel()` | 角速度 |
| `SetGravity(float)` | 重力の強さ（**正の値で下向き**、デフォルト 200） |
| `SetRestitution(float)` / `GetRestitution()` | 反発係数（デフォルト 0.2） |
| `SetMass(float)` / `GetMass()` | 質量 |
| `SetIsStatic(bool)` / `IsStatic()` | 静的（動かない）オブジェクトにする。地面などに使用 |
| `SetFreezeRotation(bool)` | 回転を固定（プレイヤー向け） |
| `SetInertia(float)` | 慣性モーメント。通常はコライダー追加時に**自動計算**されるので不要 |
| `Wake()` / `IsSleeping()` | スリープ制御 |

補足：

- 摩擦係数は 0.4、線形・角度ダンピングは 0.1 で固定です（`GetFriction()` で取得）。
- 速度が小さく、下から支えられた状態が 0.5 秒続くとスリープし、積み木のような積み上げが安定します。他の動くオブジェクトが触れると自動で起きます。

### BoxCollider2D

```cpp
BoxCollider2D(XMFLOAT2 halfExtents, XMFLOAT2 offset, bool isTrigger, TransformComponent& transform);
```

| 引数 | 説明 |
|---|---|
| `halfExtents` | 半分のサイズ。**Transform のスケール倍**される（`{0.5, 0.5}` でスプライトと同じ大きさ） |
| `offset` | 中心のオフセット（同じくスケール倍） |
| `isTrigger` | `true` なら衝突イベントのみ通知し、物理的な押し出しはしない |

| 関数 | 説明 |
|---|---|
| `GetWorldOBB()` | ワールド空間の回転付きボックス |
| `GetWorldAABB()` | ワールド空間の軸並行ボックス |
| `SetTrigger(bool)` / `IsTrigger()` | トリガー切り替え |
| `SetColliderActive(bool)` / `IsColliderActive()` | 判定の有効／無効 |

### 衝突イベント

`IComponent` を継承したコンポーネント（`MonoBehavior` 含む）で以下を override すると受け取れます。

```cpp
void OnCollisionEnter2D(const GameObject& other) override;  // 接触した瞬間
void OnCollisionStay2D (const GameObject& other) override;  // 接触中
void OnCollisionExit2D (const GameObject& other) override;  // 離れた瞬間
```

トリガー同士・トリガーと通常コライダーでもイベントは発火します。

---

## 9. スクリプト（MonoBehavior）

`MonoBehavior` を継承してゲームロジックを書きます。

```cpp
// MyScript.h
#pragma once
#include "MonoBehavior.h"

class MyScript : public MonoBehavior
{
public:
    void Awake() override;                    // AddComponent 時
    void Start() override;                    // 最初の Update 前に 1 回
    void Update(float dt) override;
    void LateUpdate(float dt) override;       // 物理の後
    void OnDestroy() override;
    void OnCollisionEnter2D(const GameObject& other) override;
};
```

| 関数 | 説明 |
|---|---|
| `Awake` / `Start` / `Update` / `LateUpdate` / `OnDestroy` | ライフサイクル（上記の順で呼ばれる） |
| `SetInput(Keyboard&, Mouse&)` | 入力への参照をセット。**入力を使うなら生成後に必ず呼ぶ** |
| `Instantiate(prefab, pos)` | プレハブ生成（Scene::Instantiate の呼び出し） |
| `Destroy(GameObject*)` / `DestroySelf()` | 破棄予約 |
| `SetEnabled(bool)` / `IsEnabled()` | 有効／無効（無効だと Update されない） |
| `IsStarted()` | Start 済みか |
| `ChangeScene(name, exitTime = 0, enterTime = 0)` | シーン遷移を予約（`SceneManager::RequestChange` の呼び出し） |

### 例：スペースキーでブロックを落とす（`PlayerController` より）

```cpp
void PlayerController::HandleBlockSpawn()
{
    if (keyboard->KeyIsTriggered(' '))
    {
        Instantiate("block", owner->GetTransform()->GetPosition());
    }
}
```

> `owner` はこのスクリプトが付いている `GameObject*` です。
> `Instantiate` で作ったオブジェクトには `SetInput` は自動では呼ばれません（入力が必要なら戻り値に対して設定してください）。

---

## 10. プレハブ（PrefabRegistry）

名前でオブジェクトのテンプレートを登録し、`Instantiate` で量産します。

### 登録（`PrefabRegistry.cpp`）

```cpp
void RegisterPrefabs()
{
    PrefabRegistry::Instance().Register("block", [](GameObject& object)
    {
        auto& anim = object.AddComponent<AnimatorComponent>(object.GetRenderer());
        anim.SetRenderLayer(RenderLayer::Default);
        anim.SetStatic(L"../../assets/jinx.jpg");

        object.GetTransform()->SetScale(0.05f);

        auto& rb = object.AddComponent<Rigidbody2DComponent>(*object.GetTransform(), 1.0f);
        rb.SetGravity(500.f);

        object.AddComponent<BoxCollider2D>(DirectX::XMFLOAT2(0.5f, 0.5f),
            DirectX::XMFLOAT2(0.f, 0.f), false, *object.GetTransform());

        object.AddComponent<PhysicsTest>();
        object.SetTag(ObjectTag::Block);
    });
}
```

`RegisterPrefabs()` は `App::Init()` の先頭で呼ばれます。

### 使用

```cpp
scene.Instantiate("block", {0.f, 200.f, 1.f});   // どこからでも
Instantiate("block", pos);                         // MonoBehavior 内から
```

未登録の名前を渡すと `assert` で止まり、`nullptr` が返ります。

---

## 11. カメラ

```cpp
Camera2D camera;
camera.SetPosition({0.f, 300.f});
DirectX::XMFLOAT2 p = camera.GetPosition();
renderer.SetView(camera.GetView());   // App::Draw で毎フレーム設定
```

- `GameContext::camera` から `owner->GetContext().camera` で取得できます。
- サンプルの `CameraController`（`MonoBehavior`）は、積み上がったブロックの最上部を検出して、上昇は速く・下降は遅延付きでスムーズに追従します。
- シーン切り替え時にカメラ位置は `(0, 0)` にリセットされます。

---

## 12. 入力（Keyboard / Mouse）

`Window` が `keyboard` と `mouse` を公開しています（`wnd.keyboard`, `wnd.mouse`）。

### Keyboard

キーコードは Windows の仮想キー（`'A'`, `' '`, `VK_LEFT` など）です。

| 関数 | 説明 |
|---|---|
| `KeyIsPressed(key)` | 押されている間 `true` |
| `KeyIsTriggered(key)` | **押した瞬間のフレームのみ** `true`（オートリピートは無視） |
| `KeyIsReleased(key)` | **離した瞬間のフレームのみ** `true` |
| `ReadKey()` / `KeyIsEmpty()` / `FlushKey()` | キーイベントキュー |
| `ReadChar()` / `CharIsEmpty()` / `FlushChar()` | 文字入力キュー |
| `EnableAutorepeat()` / `DisableAutorepeat()` | オートリピート切り替え |

`Triggered` / `Released` は `App::Run` の最後で `keyboard.EndFrame()` によりクリアされます。ウィンドウがフォーカスを失うと状態はリセットされます。

### Mouse

| 関数 | 説明 |
|---|---|
| `GetPos()` / `GetPosX()` / `GetPosY()` | クライアント座標 |
| `LeftPressed()` / `RightPressed()` | ボタン状態 |
| `IsInWindow()` | カーソルがウィンドウ内か |
| `Read()` / `IsEmpty()` / `Flush()` | イベントキュー（`LPressed / LReleased / RPressed / RReleased / WheelUp / WheelDown / Move / Enter / Leave`） |
| `EnableRaw()` / `DisableRaw()` / `RawEnabled()` | Raw Input（相対移動量）の有効化 |
| `readRawDelta()` | Raw の移動量 `{x, y}` を 1 件取得 |

### カーソル制御（Window）

```cpp
wnd.DisableCursor();  // 非表示 + ウィンドウ内に拘束（FPS 視点向け）
wnd.EnableCursor();
```

---

## 13. テキスト描画（TextRenderer）

即時モードのテキスト描画です。フレーム中にキューに積み、`Flush` で一括描画します。文字は **UTF-8**（日本語対応）で、初めて使う文字は GDI で自動的にアトラスへラスタライズされます。

### 基本の流れ

```cpp
// 生成（日本語を出すなら日本語対応フォントを推奨：MS Gothic / BIZ UDGothic / Meiryo / Yu Gothic UI）
TextRenderer textRenderer(renderer, L"MS Gothic", 18);

// 毎フレーム
renderer.BeginFrame(0, 0, 0);
textRenderer.Begin();                                  // キューをクリア
textRenderer.DrawScreen("残り時間: 12.3", {400.f, 40.f},
                        TextColor::Yellow, 1.f, TextAlign::Center);
textRenderer.Flush(renderer);                          // まとめて描画
renderer.EndFrame();
```

### 関数一覧

| 関数 | 説明 |
|---|---|
| `Begin()` | キューとデバッグ行カーソルをクリア |
| `Flush(renderer)` | 描画実行（ワールド文字 → 画面文字の順） |
| `DrawScreen(text, posPx, color, scale, align)` | **画面座標**（左上原点・+Y 下・ピクセル）に描画。カメラの影響を受けない（HUD 向け） |
| `DrawWorld(text, worldPos, color, scale, align)` | **ワールド座標**に描画。カメラに追従（オブジェクトのラベル向け）。デフォルトは中央揃え |
| `DrawScreenF(posPx, color, fmt, ...)` | printf 形式（画面座標） |
| `DrawWorldF(worldPos, color, fmt, ...)` | printf 形式（ワールド座標） |
| `DebugLine(fmt, ...)` / `DebugLine(color, fmt, ...)` | 画面左上に 1 行ずつ積み上げるデバッグ表示 |
| `SetDebugOrigin(posPx)` / `SetDebugScale(s)` | デバッグ行の位置・拡大率 |
| `Measure(text, scale)` | 描画サイズ `{幅, 高さ}` を取得 |
| `SetScreenSize(w, h)` | 画面サイズ変更 |
| `GetFont()` | `BitmapFont` を取得 |

### TextAlign / TextColor

- `TextAlign::Left / Center / Right`
- `TextColor::White / Black / Red / Green / Blue / Yellow / Cyan`（`XMFLOAT4`。自分で `{r, g, b, a}` を渡すことも可能）

### 使用例

```cpp
textRenderer.DebugLine("%.1f fps  (%.2f ms)", fps, ms);
textRenderer.DebugLine(TextColor::Cyan, "cam y %.0f", camera.GetPosition().y);

textRenderer.DrawScreen(std::format("タワーの高さ: {:.2f}", height),
                        {WIN_WIDTH * 0.5f, 24.f}, TextColor::Yellow, 1.f, TextAlign::Center);

textRenderer.DrawWorld("Player", {pos.x, pos.y + 80.f});
```

> 注意
> 
> - 改行 `\n` とタブ `\t` に対応しています。
> - `BitmapFont` のアトラスは既定 1024x1024 です。文字種が非常に多い場合は満杯になり、それ以降の未登録文字は空白になります（`atlasSize` を上げてください）。
> - 現在の `App` は `L"Consolas"` を指定しています。日本語を確実に表示したい場合は `L"MS Gothic"` などに変更してください。

---

## 14. デバッグ描画（DebugRenderer）

線分ベースのデバッグ描画です。

| 関数 | 説明 |
|---|---|
| `Begin()` | 頂点キューをクリア |
| `DrawLine(start, end, color)` | 線分 |
| `DrawBox(corners[4], color)` | 4 頂点で囲む矩形 |
| `DrawCross(center, size, color)` | 十字マーク |
| `Flush(renderer)` | 描画実行（最後に呼ぶ） |

```cpp
debugRenderer.Begin();
for (auto& go : scene.GetObjects())
    if (auto* col = go->GetComponent<BoxCollider2D>())
        debugRenderer.DrawBox(col->GetWorldOBB().GetCorners(), {0, 1, 0, 1});  // コライダー表示
// ...スプライト描画の後...
debugRenderer.Flush(renderer);
```

---

## 15. テクスチャ・Renderer

### TextureCache

| 関数 | 説明 |
|---|---|
| `Load(renderer, path)` | 画像を読み込み SRV を返す。**同じパスはキャッシュ**を再利用。8bit / 16bit / HDR を自動判別 |
| `LoadSolid(renderer, rgba)` | 1x1 の単色テクスチャ（`0xAABBGGRR` の並び）を生成・キャッシュ |
| `Clear()` | キャッシュ全破棄 |

### Renderer

| 関数 | 説明 |
|---|---|
| `BeginFrame(r, g, b)` / `EndFrame()` | 画面クリア / Present（VSync 有効） |
| `Set2DMode()` | 深度オフ・アルファブレンドオン・正射影 |
| `Set3DMode()` | 深度オン・ブレンドオフ・透視投影 |
| `SetView(XMMATRIX)` / `GetView()` / `GetProj()` | ビュー・射影行列 |
| `SetSpriteFlip(flipX, flipY)` | スプライト反転フラグ |
| `GetDevice()` / `GetContext()` | D3D11 デバイス / コンテキスト |

---

## 16. 拡張方法

### 新しいコンポーネントを作る

```cpp
class HealthComponent : public IComponent
{
public:
    void Update(float dt) override { /* ... */ }
    int hp = 100;
};

obj->AddComponent<HealthComponent>();
auto* hp = obj->GetComponent<HealthComponent>();
```

`IComponent` の `owner` から所有する `GameObject` にアクセスできます。ゲームロジックなら `MonoBehavior` 継承を推奨します。

### 新しい Prefab を増やす

`PrefabRegistry.cpp` の `RegisterPrefabs()` に `Register("名前", lambda)` を追加します。

### 新しいコライダー形状を追加する

1. `ColliderType` に種類を追加し、`Collider2D` を継承したクラスを作る（`GetType / GetWorldAABB / ComputeInertia` を実装）。
2. `CollisionTests.cpp` に判定関数を書き、`RegisterCollisionTests()` で登録する。

```cpp
CollisionDispatch::GetInstance().Register(ColliderType::Box, ColliderType::Box,
    [](const Collider2D& a, const Collider2D& b) -> std::optional<CollisionManifold> { /* ... */ });
```

異なる型の組み合わせは、法線を反転した逆方向の判定が自動登録されます。

### 新しいシェーダーを追加する

1. `.hlsl` を追加し、`.vcxproj` の `FxCompile` に `ShaderType`（Vertex / Pixel）を設定。
2. `MaterialComponent(renderer, matData, L"XXVertexShader.cso", L"XXPixelShader.cso")` のように、パスを指定して使う（同じ組み合わせはキャッシュされます）。

### 新しいシーンを追加する

1. `Scene` を継承したクラスを作り、`OnEnter()` でオブジェクトを生成する（「5. シーンの作成」参照）。
2. `App::Init()` で `sceneManager.Register<MyScene>("名前")` を追加する。
3. `RequestChange("名前")` / `ChangeScene("名前")` で遷移する。

ステージ違いなど、配置だけが異なるシーンはクラスを増やさず、1 つのクラスを `SharedGameData` の値（ステージ番号など）で切り替えるのがおすすめです。

---

## 17. 注意点

- **Rigidbody + Collider が必須**：どちらか一方だけのオブジェクトは物理の対象になりません。
- **AddComponent の順序**：`Awake` 内で他コンポーネントを取得する場合、先に追加しておくこと。
- **`SetScale(float)` は乗算**：`SetStatic` / `AddAnimation` の後に使うこと。
- **プレハブ生成は遅延反映**：`Instantiate` したオブジェクトが `GetObjects()` に現れるのは `FlushPending()` 後です。
- **アセットのパス**：実行時のカレントディレクトリが変わるとテクスチャや `.cso` が見つからず、`assert` や `nullptr` 参照の原因になります。
- **シーン切り替えは遅延反映**：`RequestChange` は予約のみで、実際の切り替えはフレーム末の `sceneManager.Update()` で行われます。スクリプトの `Update` 中に呼んでも安全です。
- **遷移中はゲームが止まる**：`EXITING` / `ENTERING` の間は `OnUpdate` / スクリプト / 物理 / アニメーションが更新されません（時間 0 なら停止フレームはありません）。
- **切り替えでシーン内はすべて破棄**：`GameObject*` などシーン内オブジェクトへのポインタを、シーンをまたいで保持しないこと。残したい値は `GameContext::shared` へ。
- **`Clear()` 中の生成は無効**：シーン破棄中（`OnDestroy` 内など）の `Instantiate` / `Add2DObject` は `nullptr` を返します。

