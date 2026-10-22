# HEW2026

コンポーネント指向（COP）で作った DirectX 11 の 2D ゲームフレームワークです。
`GameObject` に `Component` を追加して機能を組み立てる Unity ライクな構造になっています。

現在は 2D 機能（スプライト・アニメーション・2D 物理・テキスト描画）と、ImGui によるエディタ（オブジェクト編集・プレハブ配置・シーン JSON の保存／読み込み）が中心です。3D は下地のみで、Assimp による読み込みは現在無効（コメントアウト）になっています。

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
13. [テキスト描画（TextRenderer / TextLabel）](#13-テキスト描画textrenderer--textlabel)
14. [デバッグ描画（DebugRenderer）](#14-デバッグ描画debugrenderer)
15. [テクスチャ・Renderer](#15-テクスチャrenderer)
16. [エディタ（ImGui）](#16-エディタimgui)
17. [拡張方法](#17-拡張方法)
18. [注意点](#18-注意点)

---

## 1. 開発環境・ビルド

| 項目 | 内容 |
|---|---|
| IDE / ツールセット | **Visual Studio 2026**（PlatformToolset `v145`） |
| 言語標準 | C++20 |
| プラットフォーム | x64（Debug / Release） |
| 文字コード | ソースは UTF-8（`/utf-8` オプション有効） |
| 依存ライブラリ | ImGui・nlohmann/json（vcpkg マニフェスト `vcpkg.json`）、stb_image（同梱） |
| ウィンドウサイズ | `WindowSettings.h` の `WIN_WIDTH` / `WIN_HEIGHT`（1366 x 768） |

> **VS 2022 ではビルドできません。** `v145` ツールセットは VS 2026 専用です。
> プロジェクトを「再ターゲット」して `v143` に変更したままコミットしないでください（他のメンバーや CI のビルドが壊れます）。

### 初回セットアップ（vcpkg）※1 回だけ

依存ライブラリは vcpkg が自動で取得します。ただし、各 PC で **vcpkg と Visual Studio の連携** を 1 回だけ有効にする必要があります。
これをしていないと、`include ファイルを開けません。'imgui.h'`（C1083）などのエラーになります。

1. Visual Studio Installer で、vcpkg パッケージマネージャー（「C++ によるデスクトップ開発」に含まれる）が入っていることを確認する
2. Visual Studio を閉じる
3. スタートメニューから **Developer PowerShell for VS 2026** を開く
   ※普通の PowerShell ではなく、必ず Developer PowerShell を使ってください
4. 次のコマンドを実行する
   ```
   vcpkg integrate install
   ```
5. ソリューションを開き直し、プロジェクトのプロパティに **vcpkg** の項目が出ていることを確認する

それでもエラーが出る場合は、プロジェクトのプロパティ → vcpkg → **Use Vcpkg Manifest** が **はい** になっているか確認してください。

> この設定はユーザー単位です。CI（GitHub Actions）でも、ワークフロー内で同じ `vcpkg integrate install` を実行しています。

### ビルド手順

1. リポジトリをクローンし、ルートの `HEW2026.slnx` を Visual Studio で開く。
2. 構成を `x64` の `Debug` か `Release` にする。
3. ビルドすると、vcpkg が `vcpkg.json` に書かれたライブラリ（ImGui・nlohmann/json）を自動で取得・ビルドします。初回は時間がかかります（`vcpkg_installed/` は Git 管理外）。
4. HLSL は `.cso` にコンパイルされ、実行時に `D3DReadFileToBlob` で読み込まれます（`VertexShader.cso` など）。実行ファイルと同じ場所に出力されます。
5. 画像などは `../../assets/` からの相対パスで読み込みます（例：`L"../../assets/jinx.jpg"`）。実行時のカレントディレクトリに注意してください。

### CI（GitHub Actions）

`.github/workflows/build.yml` で自動ビルドしています。

| きっかけ | 内容 |
|---|---|
| `master` への push / Pull Request | Debug・Release の両方をビルド。失敗すると PR に表示されます |
| `v*` タグの push（例：`v0.1.0`） | Release ビルドを zip にまとめ、GitHub Release として公開 |
| Actions タブから手動実行 | 任意のブランチでビルド |

- vcpkg のビルド結果はキャッシュされます。`vcpkg.json` を変更した直後の 1 回だけ時間がかかります。
- PR では **PR 側のブランチのワークフロー** が使われるので、ワークフローの変更は master にマージする前に PR で確認できます。
- ワークフローファイル（`.github/workflows/`）を push するには、`workflow` 権限付きのトークンが必要です。Sourcetree の場合は `repo` / `workflow` / `read:user` / `user:email` を付けた Personal Access Token（classic）で認証してください。

---

## 2. 全体構成とフレームの流れ

```
WinMain → App::Run()
            ├─ Init()            … プレハブ・コリジョンテスト・シーンの登録、最初のシーンへ遷移、ImGui 初期化
            └─ ループ
                 ├─ Window::ProcessMessages()
                 ├─ HandleInput(deltaTime)   … F1 でエディタ表示切り替え（Debug ビルドのみ）
                 ├─ Update(deltaTime)
                 ├─ Draw(deltaTime)
                 └─ keyboard.EndFrame()
```

### `App::Update` の順序

```
// 遷移中（EXITING / ENTERING）はここをスキップ
if (editor.ShouldTick())     // エディタで一時停止中はスキップ（1フレーム送り時のみ実行）
{
    scene->Update(dt)            // 各 GameObject の Update
    scene->OnUpdate(dt)          // シーン固有の処理（タイマー・遷移判定など）
    scriptSystem.Update(dt)      // Start（初回のみ）→ Update
    physicsSystem.Update(dt)     // 積分 → 衝突判定 → 拘束解決 → イベント通知
    scriptSystem.LateUpdate(dt)  // LateUpdate（カメラ追従など）
    animationSystem.Update(dt)   // アニメーション更新
}
scene->FlushPending()        // 生成・破棄の反映（一時停止中も実行）

sceneManager.Update(dt)      // 遷移の進行・シーン切り替え（常に最後）
```

### `App::Draw` の順序

```
renderer.BeginFrame()
imGui.BeginFrame()           // ImGui のフレーム開始
renderSystem.Render()        // スプライト
DrawTextLabels(scene)        // TextLabel コンポーネントの文字（全シーン共通）
DebugRender / DebugTextRender
scene->OnDrawUI(text)        // シーンの HUD・画面テキスト
editor.Draw()                // エディタ UI（F1 で表示中のみ）
debugRenderer.Flush()
textRenderer.Flush()
imGui.EndFrame()             // ImGui を最前面に描画
renderer.EndFrame()          // Present
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
- 回転は **ラジアン**。2D では `rotation.z` を使います（エディタのインスペクターでは度で表示されます）。
- `TextRenderer::DrawScreen`（と、それを使う `TextLabel` のスクリーン表示）だけは例外で、**左上原点・+Y が下**のピクセル座標です。
- **スクリーン空間のスプライト**（`SetEnableScreenSpace(true)`）はカメラの影響を受けず、画面に固定されます。座標系はワールドと同じく**画面中央原点・+Y が上**のピクセル座標です（`TextRenderer::DrawScreen` とは異なるので注意）。

---

## 4. クイックスタート

シーンの `OnEnter()` 内で、画像付きの落下する箱を作る例です（`scene.` は不要で、シーンのメンバー関数として呼べます）。

```cpp
// 1. オブジェクトを生成（2D 用のクアッドメッシュ付き）
GameObject* obj = Add2DObject();
obj->SetName("Box");                         // エディタのヒエラルキーに表示される名前
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

// 4. スクリプト（MonoBehavior 派生クラス）
obj->AddComponent<MyScript>();
```

---

## 5. Scene / GameObject

### Scene

| 関数 | 説明 |
|---|---|
| `GameObject* Add2DObject()` | 2D オブジェクトを即座にシーンへ追加して返す（初期化用） |
| `GameObject* Instantiate(const std::string& prefab, const XMFLOAT3& pos)` | 登録済みプレハブから生成。名前はプレハブ名になる。**シーンへの追加は `FlushPending()` で行われる**（フレーム末） |
| `GameObject* InstantiateAsLayout(const std::string& prefab, const XMFLOAT3& pos)` | シーン JSON に保存される「レイアウトオブジェクト」として生成（JSON ローダー・エディタ専用。ゲームコードでは `Instantiate` を使う）。[シーン JSON](#シーン-jsonレベルの保存読み込み) 参照 |
| `void Destroy(GameObject* object)` | 破棄を予約。同じオブジェクトを複数回渡しても 1 回だけ。`FlushPending()` で実際に削除 |
| `bool IsPendingDestroy(const GameObject* obj)` | 破棄予約済みか |
| `void Update(float dt)` | 全オブジェクトを更新 |
| `void FlushPending()` | 生成・破棄の反映。破棄時は物理・スクリプト・アニメーション・描画の各システムから登録解除 |
| `GetObjects()` | 現在のオブジェクト一覧を取得（const 版あり） |
| `GetPhysics()` | `PhysicsSystem` の参照 |
| `void Clear()` | 全オブジェクトを各システムから登録解除して破棄（シーン切り替え時に `SceneManager` が呼ぶ） |

#### シーンのフック（override して使う）

| 関数 | 呼ばれるタイミング |
|---|---|
| `OnEnter()` | シーン開始時。**コードで作るオブジェクトの生成はここで行う**（直後にシーン JSON が読み込まれる） |
| `OnExit()` | シーン終了時（`Clear()` の直前） |
| `OnUpdate(float dt)` | 毎フレーム（遷移中・エディタで一時停止中は呼ばれない）。タイマーや遷移判定など |
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
    player->SetName("Player");
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
| `Register<T>(name)` / `Register(name, factory)` | シーンを名前で登録 |
| `IsRegistered(name)` | 登録済みか |
| `GetSceneNames()` | 登録済みシーン名の一覧。エディタのシーン選択で使用 |
| `RequestChange(name, exitTime = 0, enterTime = 0)` | 遷移を**予約**（即座には切り替わらない）。遷移中の呼び出しは無視。未登録の名前は `assert` |
| `Update(dt)` | 遷移を進め、切り替えを実行（`App::Update` の最後で呼ばれる） |
| `GetCurrScene()` / `GetCurrSceneName()` | 現在のシーン・名前 |
| `GetNextSceneName()` | 遷移先のシーン名 |
| `GetTransitionState()` | 遷移の状態（下表） |
| `IsTransitioning()` | 遷移中か（`NONE` 以外） |
| `GetTransitionProgress()` | 現在のフェーズの進行度（0 → 1）。遷移していないときは 0 |
| `GetLastLayoutLoad()` | 直近のシーン切り替えで読み込んだシーン JSON の結果（`SceneLayout::Result`）。エディタに表示 |

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
  → シーン JSON（assets/scenes/<シーン名>.json）のレイアウトオブジェクトを読み込み（ファイルがなければ何もしない）
  → ENTERING（enterTime 秒。ゲーム処理は停止）
  → NONE（通常更新に戻る）
```

最初のシーン（`App::Init` での `RequestChange`）は `EXITING` を経由せず、すぐに読み込まれます。
現在のシーン名を渡すと、そのシーンを作り直します（エディタの「リロード」はこれを使っています）。リロードでもシーン JSON は読み込み直されます。

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

シーン内のものは切り替え時にすべて破棄されます。次のシーンに渡したい値は `GameContext::globalContext`（`GlobalContext`）に入れてください。

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
text.DrawScreen(std::format("スコア: {}", context.globalContext.score), ...);
```

#### 起動シーンの指定（デバッグ用）

コマンドライン引数に登録済みのシーン名を渡すと、そのシーンから起動します（例：`Stage`）。
Visual Studio の「プロジェクトのプロパティ → デバッグ → コマンド引数」で設定できます。未登録の名前や空の場合は `Title` から起動します。
実行中のシーン切り替えは、エディタのシーン選択からも行えます。

### GameObject

| 関数                                   | 説明 |
|--------------------------------------|---|
| `T& AddComponent<T>(args...)`        | コンポーネントを追加。型に応じて自動でシステムに登録される（下表） |
| `T* GetComponent<T>()`               | コンポーネント取得。なければ `nullptr` |
| `TransformComponent* GetTransform()` | Transform 取得 |
| `GetMeshes()`                        | メッシュ一覧 |
| `GetRenderer()` / `GetContext()`     | Renderer / GameContext の参照 |
| `GetTag()` / `SetTag(ObjectTag)`     | タグ（`ObjectTags.h` の `DEFAULT / ENEMY / PLAYER / BLOCK / GROUND`） |
| `GetName()` / `SetName(name)`        | 名前。エディタのヒエラルキーに表示される（デフォルト `"GameObject"`、`Instantiate` ではプレハブ名） |
| `GetPrefabName()`                    | 生成元のプレハブ名（`Instantiate` で自動設定。`Add2DObject` で作ったものは空） |
| `IsLayoutObject()`                   | シーン JSON に保存されるオブジェクトか（`InstantiateAsLayout` で生成されたものだけ `true`） |

> 名前を付けておくと、エディタでオブジェクトを見分けやすくなります。`OnEnter` で作るオブジェクトには `SetName` を付けることを推奨します。

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
> - `Awake` は `AddComponent` の**その場**で呼ばれるため、`Awake` 内で `GetComponent` したい相手は**先に追加**しておいてください（例：`Awake` で `Rigidbody2DComponent` を取得するスクリプトは、Rigidbody の後に追加する）。

### ObjectTag

タグは `ObjectTags.h` の `OBJECT_TAG_LIST` で定義されています。このリストから `enum class ObjectTag` と、エディタで表示するタグ名の配列 `objectTagNames` の両方が自動生成されます（X マクロ）。
タグを追加する方法は「[17. 拡張方法](#17-拡張方法)」を参照してください。

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
| `SetRenderLayer(RenderLayer)` / `GetRenderLayer()` | 描画レイヤー |
| `SetSortOrder(int)` / `GetSortOrder()` | 同一レイヤー内の描画順（小さいほど奥） |
| `SetFlipX(bool)` / `SetFlipY(bool)` | 反転 |
| `SetEnabled(bool)` / `IsEnabled()` | 有効／無効 |
| `SetEnableScreenSpace(bool)` | スクリーン空間で描画する（カメラに追従せず画面に固定）。UI 向け |
| `IsScreenSpaceEnabled()` | スクリーン空間で描画しているか |

#### アニメーション使用例

```cpp
auto& anim = playerObj->AddComponent<AnimatorComponent>(context.renderer);
anim.SetRenderLayer(RenderLayer::Player);
anim.AddAnimation("CharIdle", L"../../assets/PlayerCharacter.png",
                              L"../../assets/PlayerCharacter.json");
anim.AddAnimation("CharMove", L"../../assets/PlayerCharacterMove.png",
                              L"../../assets/PlayerCharacterMove.json");
anim.SetCurrAnimation("CharIdle");
```

#### JSON の形式

Aseprite の書き出し（Array 形式）を想定した簡易パーサです。

- `"frames"` 配下の各 `"frame": {x, y, w, h}` と `"duration"`（ミリ秒）
- `"meta"` 内の `"size": {w, h}`（シート全体サイズ）
- 任意で `"Loop"` タグの `"from"` / `"to"`（ループ範囲）

### スクリーン空間描画（UI）

`SetEnableScreenSpace(true)` にすると、そのスプライトはカメラの影響を受けず画面に固定されます。HUD・アイコン・UI パネルなどに使います。

```cpp
GameObject* icon = Add2DObject();
icon->SetName("Icon");
icon->GetTransform()->SetPosition({0.f, 0.f, 1.f});   // 画面中央が原点・+Y が上

auto& anim = icon->AddComponent<AnimatorComponent>(context.renderer);
anim.SetRenderLayer(RenderLayer::UI);   // 最前面に描画
anim.SetEnableScreenSpace(true);
anim.SetStatic(L"../../assets/icon.png");
```

- 設定は `SetStatic` / `AddAnimation` の前後どちらでも反映されます（後から追加したアニメーションにも引き継がれます）。
- UI オブジェクトも通常の GameObject なので、シーン切り替え時に自動で破棄されます。
- スプライトシェーダーの仕様上、アルファ 0.1 以下のピクセルは破棄されるため、半透明のフェードや薄いパネルには向きません。
- 単色の矩形が必要な場合は `TextureCache::LoadSolid` で 1x1 テクスチャを作り、スケールで大きさを指定します。

#### 仕組み

`SpriteRendererComponent::Render` が描画の直前に `Renderer::SetEnableScreenSpace(true)` を設定し、`TransformCBuffer` がこのフラグを見てビュー行列を単位行列に置き換えます（＝カメラ移動が適用されない）。描画後はフラグを `false` に戻します。

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
| `SetGravity(float)` / `GetGravity()` | 重力の強さ（**正の値で下向き**、デフォルト 200） |
| `SetRestitution(float)` / `GetRestitution()` | 反発係数（デフォルト 0.2） |
| `SetMass(float)` / `GetMass()` | 質量 |
| `SetIsStatic(bool)` / `IsStatic()` | 静的（動かない）オブジェクトにする。地面などに使用 |
| `SetFreezeRotation(bool)` / `IsFreezeRotation()` | 回転を固定（プレイヤー向け） |
| `SetInertia(float)` | 慣性モーメント。通常はコライダー追加時に**自動計算**されるので不要 |
| `GetFriction()` | 摩擦係数 |
| `Wake()` / `IsSleeping()` | スリープ制御 |

補足：

- 摩擦係数は 0.4、線形・角度ダンピングは 0.1 で固定です。
- 速度が小さく、下から支えられた状態が 0.5 秒続くとスリープし、積み木のような積み上げが安定します。他の動くオブジェクトが触れると自動で起きます。
- 慣性モーメントはコライダー追加時にだけ計算されます。後から `SetMass` で質量を変えた場合は、`SetInertia(collider->ComputeInertia(mass))` で更新してください（エディタの質量変更はこれを自動で行います）。

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
| `ComputeInertia(mass)` | 指定した質量での慣性モーメントを計算 |

### 衝突イベント

`IComponent` を継承したコンポーネント（`MonoBehavior` 含む）で以下を override すると受け取れます。

```cpp
void OnCollisionEnter2D(const GameObject& other) override;  // 接触した瞬間
void OnCollisionStay2D (const GameObject& other) override;  // 接触中
void OnCollisionExit2D (const GameObject& other) override;  // 離れた瞬間
```

トリガー同士・トリガーと通常コライダーでもイベントは発火します。

### デバッグ統計（PhysicsSystem）

物理の負荷を調べるための値を `PhysicsSystem` から取得できます。エディタの **Physics Stats** ウィンドウはこれを表示しています。

| 関数 | 説明 |
|---|---|
| `GetDebugStats()` | 直近の 1 ステップの統計（`DebugStats`、下表） |
| `ConsumeStepCount()` | 前回呼んでから `Update` が何回実行されたか。呼ぶと 0 に戻る（1 描画フレームに 1 回呼ぶ） |

| `DebugStats` の値 | 内容 |
|---|---|
| `bodies` / `gridCells` / `maxPerCell` | 剛体の数 / 使用中のグリッドセル数 / 1 セルに入っている最大の剛体数 |
| `rawPairChecks` / `candidatePairs` | 重複除去前のペア判定回数 / 重複除去後の候補ペア数 |
| `overlaps` / `contacts` | 接触したペア数（トリガー含む） / 押し出し計算の対象になったペア数 |
| `integrateMs` 〜 `sleepAndEventsMs` / `totalMs` | 各処理（積分・ブロードフェーズ・ナローフェーズ・解決・位置補正・スリープとイベント）の時間 / 合計（ミリ秒） |

`maxPerCell` や `rawPairChecks` が大きい場合は、オブジェクトが 1 か所に密集してブロードフェーズが効いていません。

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

### 例：スペースキーでプレハブを生成する

```cpp
// 生成側（OnEnter など）
obj->AddComponent<MyScript>();
obj->GetComponent<MyScript>()->SetInput(context.keyboard, context.mouse);

// MyScript.cpp
void MyScript::Update(float dt)
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
        object.SetTag(ObjectTag::BLOCK);

        auto& anim = object.AddComponent<AnimatorComponent>(object.GetRenderer());
        anim.SetRenderLayer(RenderLayer::Default);
        anim.SetStatic(L"../../assets/jinx.jpg");

        object.GetTransform()->SetScale(0.05f);

        auto& rb = object.AddComponent<Rigidbody2DComponent>(*object.GetTransform(), 1.0f);
        rb.SetGravity(500.f);

        object.AddComponent<BoxCollider2D>(DirectX::XMFLOAT2(0.5f, 0.5f),
            DirectX::XMFLOAT2(0.f, 0.f), false, *object.GetTransform());
    });
}
```

`RegisterPrefabs()` は `App::Init()` の先頭で呼ばれます。

### 使用

```cpp
scene.Instantiate("block", {0.f, 200.f, 1.f});   // どこからでも
Instantiate("block", pos);                         // MonoBehavior 内から
```

- 生成されたオブジェクトの名前とプレハブ名（`GetPrefabName()`）は、プレハブ名（例：`"block"`）になります。
- プレハブ関数より前に位置が設定されるため、プレハブ関数内で位置を上書きしないでください。
- 未登録の名前を渡すと `assert` で止まり、`nullptr` が返ります。

### 一覧の取得

| 関数 | 説明 |
|---|---|
| `Register(name, fn)` | プレハブを登録（同名は上書き） |
| `Find(name)` | 登録済みのプレハブ関数。なければ `nullptr` |
| `GetPrefabNames()` | 登録済みプレハブ名の一覧（名前順）。エディタの **プレハブ** ウィンドウで使用 |

登録したプレハブは、エディタの **プレハブ** ウィンドウに自動で表示され、クリックで配置できます。

> プレハブ名はシーン JSON に保存されます。**名前を変えると、保存済みのシーンでは読み込まれなくなります**（スキップされる）。変更するときは JSON 側も合わせて直してください。

---

## 11. カメラ

```cpp
Camera2D camera;
camera.SetPosition({0.f, 300.f});
DirectX::XMFLOAT2 p = camera.GetPosition();
renderer.SetView(camera.GetView());   // App::Draw で毎フレーム設定
```

- `GameContext::camera` から `owner->GetContext().camera` で取得できます。
- 追従などのカメラ制御は `MonoBehavior` で実装し、`LateUpdate`（物理の後）で位置を更新するのがおすすめです。
- シーン切り替え時にカメラ位置は `(0, 0)` にリセットされます。

### CameraController（追従スクリプト）

付けたオブジェクトをカメラが滑らかに追いかける `MonoBehavior` です。

```cpp
auto& cam = obj->AddComponent<CameraController>();
cam.followSpeed = 5.f;   // 大きいほど速く追いつく（0 で追従しない）
```

- `LateUpdate` で、カメラ位置をオブジェクトの位置に向けて指数補間します（フレームレートに依存しません）。
- 追従するのは 1 つのオブジェクトだけにしてください。複数に付けると、最後に更新されたものが優先されます。

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

### エディタとの入力の分担

Windows のメッセージは、まず ImGui に渡されます。エディタが入力を使っている間は、ゲーム側の `Keyboard` / `Mouse` には届きません。

| 状態 | ゲームに届かない入力 |
|---|---|
| マウスがエディタのウィンドウ上にある（`WantCaptureMouse`） | 左右クリック・ホイール・Raw Input |
| エディタのテキスト入力欄を編集中（`WantCaptureKeyboard`） | キー押下・文字入力 |

キーやボタンを**離した**メッセージは常にゲームにも届きます（押したままエディタ上に移動しても、キーが押しっぱなしにならないようにするため）。

---

## 13. テキスト描画（TextRenderer / TextLabel）

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

textRenderer.DrawScreen(std::format("スコア: {}", score),
                        {WIN_WIDTH * 0.5f, 24.f}, TextColor::Yellow, 1.f, TextAlign::Center);

textRenderer.DrawWorld("Player", {pos.x, pos.y + 80.f});
```

> 注意
>
> - 改行 `\n` とタブ `\t` に対応しています。
> - `BitmapFont` のアトラスは既定 1024x1024 です。文字種が非常に多い場合は満杯になり、それ以降の未登録文字は空白になります（`atlasSize` を上げてください）。
> - 現在の `App` は `L"Consolas"` を指定しています。日本語を確実に表示したい場合は `L"MS Gothic"` などに変更してください。

### TextLabel（オブジェクトに文字を付ける）

Unity のように、**オブジェクトにコンポーネントとして文字を付ける**仕組みです。シーンのコードを書かなくても、`TextLabel` を持つオブジェクトの文字が自動で描画されます。

```cpp
auto& label = obj->AddComponent<TextLabel>();
label.text = "Enemy";
label.posOffset = {0.f, 40.f};   // オブジェクトの少し上

// スクリプトから後で書き換える
owner->GetComponent<TextLabel>()->text = std::format("HP {}", hp);
```

| メンバー | 説明 |
|---|---|
| `text` | 表示する文字（UTF-8）。空なら描画しない |
| `posOffset` | 位置。意味は `isDrawingScreen` によって変わる（下表） |
| `isDrawingScreen` | `false`：オブジェクトに追従（ワールド）／`true`：画面に固定（スクリーン） |

| モード | 描画 | `posOffset` の意味 |
|---|---|---|
| ワールド（`false`、既定） | `DrawWorld`。カメラに合わせて動き、オブジェクトに付いていく。中央揃え | オブジェクトの位置からのずれ（ワールド単位、+Y が上） |
| スクリーン（`true`） | `DrawScreen`。カメラの影響を受けず、画面に固定される。左揃え | **画面の左上からの位置**（ピクセル、+Y が下）。オブジェクトの位置は使われない |

- 描画は `App::Draw` の `DrawTextLabels` で行われます（スプライトの後、`OnDrawUI` の前）。**全シーン共通**で、エディタで配置したオブジェクトやシーン JSON から読み込んだオブジェクトにも効きます。
- 描画処理なので、エディタで一時停止中も表示され、ドラッグ中のオブジェクトにも付いていきます。
- 文字は常にスプライトより手前に描画されます（スプライトのレイヤー・ソート順とは無関係）。
- 使い分け：オブジェクトに属する文字（名前・頭上の HP・操作案内など）は `TextLabel`、シーン全体の HUD（スコア・タイマーなど）は `Scene::OnDrawUI` に書きます。
- 現在、`TextLabel` はインスペクターに表示されず、シーン JSON にも保存されません。文字の内容はプレハブまたはスクリプトで設定してください。

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
| `SetEnableScreenSpace(bool)` / `IsScreenSpaceEnabled()` | `true` の間、`TransformCBuffer` がビュー行列の代わりに単位行列を使う（スクリーン空間描画） |

---

## 16. エディタ（ImGui）

[Dear ImGui](https://github.com/ocornut/imgui) で作った実行時エディタです。ゲームを動かしたまま、シーン内のオブジェクトを確認・調整できます。プレハブを配置してシーン JSON に保存すれば、レベルデザインにも使えます。

### 起動・表示

- **Debug ビルドのみ**使えます。Release ビルドでは F1 が無効になり、エディタは表示されません（`App::HandleInput` の `#ifndef NDEBUG`）。
- 起動直後は非表示で、**プレイモード**（ゲームが普通に動く状態）です。**F1** で表示／非表示を切り替えます。
- F1 で開くと、プレイモードのまま**一時停止**します。そのまま一時停止中の確認ができ、レイアウトを編集するときは **編集モード** ボタンを押します。
- F1 で閉じると一時停止は解除されます。モードは変わらないので、**編集モードのまま閉じるとゲームは止まったまま**です（画面左上に `EDIT MODE - game stopped` と表示されます）。

### 編集モードとプレイモード

レイアウトの編集は**編集モード**で行い、**プレイ** で動作を確認、**編集モード** ボタンで編集に戻ります。

| | 編集モード | プレイモード |
|---|---|---|
| ゲームの更新（`Update` / スクリプト / 物理 / アニメーション） | 止まる | 動く（一時停止・1 フレーム送りも可） |
| ビューでのクリック選択・ドラッグ移動 | できる | 一時停止中のみ |
| プレハブの配置 | できる | できない |
| シーンを保存 | できる | できない |
| 変更 | 保存すれば残る | **編集モードに戻ると破棄される** |

**プレイ**（編集モード中に表示）
- シーン JSON を自動保存してから、シーンを**ファイルから読み込み直して**実行します。テストプレイは必ず保存された内容で動きます。
- 次の場合は自動保存しません。
   - シーン JSON がなく、レイアウトオブジェクトも 1 つもない（空のファイルを作らないため）
   - 読み込みでエラーまたはスキップがあった（足りない状態で上書きしてオブジェクトを失わないため）。プレイは開始され、**シーンセーブ** ウィンドウに理由が表示されます。上書きしてよい場合は、編集モードで **シーンを保存** を手動で押してください。
- 自動保存に失敗した場合は、プレイせず編集モードのままエラーを表示します。

**編集モード**（プレイ中に表示）
- プレイを押したときのシーンを、**ファイルから読み込み直して**編集モードに戻ります。物理やスクリプトで動いた位置、プレイ中に生成・削除されたものはすべて元に戻ります。
- プレイ中にゲーム側でシーンが切り替わっていても、プレイを押したシーンに戻ります。まだ一度もプレイを押していない場合は、現在のシーンを読み込み直します。

- 遷移中はどちらのボタンも押せません。
- 切り替えるとシーンが作り直されるので、選択は解除されます。

### ウィンドウ構成

| ウィンドウ | 内容 |
|---|---|
| **エディター** | FPS 表示、プレイ／編集モードの切り替えと現在のモード、一時停止／再開、1 フレーム進める、シーン選択、リロード |
| **ヒエラルキー** | 現在のシーンのオブジェクト一覧（`GetName()` の名前で表示）。クリックで選択 |
| **インスペクター** | 選択中のオブジェクトの編集と削除 |
| **プレハブ** | 登録済みプレハブの一覧。クリックで画面中央に配置（編集モードのみ） |
| **シーンセーブ** | 「シーンを保存」ボタン（編集モードのみ）と、シーン JSON の読み込み／保存の結果 |
| **Physics Stats** | 物理の統計（剛体数・ペア数・各処理の時間）。[デバッグ統計](#デバッグ統計physicssystem) 参照 |

ウィンドウはドラッグで移動・リサイズできます。配置は実行時のカレントディレクトリの `imgui.ini` に保存されます（個人設定なので Git 管理外）。
コードの初期配置（`SetNextWindowPos` / `SetNextWindowSize` の `ImGuiCond_FirstUseEver`）は、`imgui.ini` にそのウィンドウの記録がないときだけ使われます。初期配置を変えても反映されない場合は `imgui.ini` を削除してください。

### 一時停止とコマ送り（プレイモードのみ）

- **一時停止**中は、シーンの `Update` / `OnUpdate`、スクリプト、物理、アニメーションが止まります。
- **1フレーム進める** は、一時停止中に 1 フレームだけ処理を進めます。物理の挙動を 1 フレームずつ確認するのに便利です。
- 一時停止中も `FlushPending()` とシーン切り替えは動くので、削除やシーン変更はすぐ反映されます（編集モードも同じです）。
- 一時停止中の変更も、編集モードに戻ると破棄されます。

### シーン選択・リロード

- **シーン** のドロップダウンから、登録済みのシーンへ切り替えられます。
- **リロード** は現在のシーンを作り直します（`OnEnter` からやり直し）。編集モードでは、保存していない変更が破棄されます。
- どちらもモードは変えません。編集モードでシーンを切り替えると、新しいシーンも編集モード（止まった状態）で開きます。
- 遷移中は、シーン操作・ヒエラルキー・インスペクター・プレハブ・保存が無効（グレー表示）になります。

### インスペクター

| 項目 | 編集できる内容 |
|---|---|
| 共通 | 名前、タグ |
| Transform | 位置、回転（度）、スケール |
| Rigidbody2D | Static、回転を固定、速度、角速度、質量、重力、反発係数、スリープ状態の確認と Wake |
| BoxCollider2D | Trigger、判定の有効／無効 |
| Animator | 表示の有効／無効、スクリーン空間かどうか（表示のみ） |

- 各セクションは、そのコンポーネントを持つオブジェクトにだけ表示されます。
- 位置・回転・スケールなどを変更すると、スリープ中の剛体は自動で起こされます。
- 質量を変更すると、慣性モーメントも自動で再計算されます。
- **削除** ボタンは `Scene::Destroy` で破棄を予約します（各システムからも正しく登録解除されます）。

### ビューでの選択と移動（編集モード、またはプレイ中の一時停止中）

- 画面上のオブジェクトをクリックすると選択できます。選択中のオブジェクトはオレンジの枠で表示されます。
- そのままドラッグすると移動できます。ドラッグ中は剛体の速度が 0 になります。
- 重なっている場合は、手前に描画されているもの（スクリーン空間 → レイヤー → ソート順）が優先されます。
- 何もない場所をクリックすると選択解除します。
- ゲーム中のクリック操作と競合しないよう、プレイ中（一時停止していないとき）はビューでの選択はできません（ヒエラルキーからは選択できます）。

### プレハブの配置

- 編集モードで **プレハブ** ウィンドウの名前をクリックすると、そのプレハブが**カメラの位置（画面中央）**に生成され、選択状態になります。そのままドラッグやインスペクターで調整できます。
- 配置したオブジェクトは `InstantiateAsLayout` で生成されるので、シーン JSON に保存されます。
- 生成は `FlushPending()` で反映されるため、ヒエラルキーに現れるのは次のフレームです（編集モードでも反映されます）。
- プレイモードでは配置できません（編集モードに戻ると消えてしまうため）。
- 不要なものはインスペクターの **削除** で消せます。

### シーン JSON（レベルの保存・読み込み）

エディタで配置したオブジェクトは、シーンごとの JSON ファイルに保存できます。

```
assets/scenes/<シーン名>.json      例：assets/scenes/Game.json
```

**保存されるのは「レイアウトオブジェクト」（`IsLayoutObject()` が `true`）だけです。**

| 生成方法 | 保存 |
|---|---|
| エディタの **プレハブ** ウィンドウで配置 | される |
| シーン JSON から読み込まれたもの | される |
| `OnEnter` で `Add2DObject` / `Instantiate` したもの | されない（コードが毎回作る） |
| スクリプトの `Instantiate`（弾・アイテムなど） | されない |

フラグを立てるのは `Scene::InstantiateAsLayout` だけです（JSON ローダーとエディタ専用）。ゲームコードからは通常の `Instantiate` を使ってください。

#### 作業の流れ

1. Debug ビルドで起動し、F1 でエディタを開いて **編集モード** を押す（シーンがファイルから読み込み直される）
2. **プレハブ** ウィンドウでプレハブをクリック → 画面中央に配置される
3. ドラッグやインスペクターで位置・回転・スケール・名前を調整（不要なものは **削除**）
4. **プレイ** → 自動保存されてからゲームが動くので、遊んで確認する
5. **編集モード** → 保存した状態に戻るので、3 に戻って調整を続ける

- 途中で保存だけしたいときは、**シーンセーブ** ウィンドウの **シーンを保存** を押します（`assets/scenes/<シーン名>.json` に書き出し）。
- **シーンを保存** は編集モードのみ押せます（プレイ中だと、動いた後の位置が保存されてしまうため）。
- 保存したファイルは Git でコミットして共有します。同じシーンを複数人で同時に編集するとコンフリクトするので、シーン（ステージ）ごとに担当を決めてください。

#### 読み込みのタイミング

`SceneManager` がシーンを切り替えるとき、**`OnEnter()` の直後**に読み込みます（リロード時も同じ）。ファイルがなければ何もしません。
結果（読み込んだ個数・エラー）は **シーンセーブ** ウィンドウに表示され、コードからは `SceneManager::GetLastLayoutLoad()` で取得できます。

- 未登録のプレハブ名（名前変更・削除したもの）や不正な項目は飛ばされ、「スキップ」として個数が表示されます。
- JSON の形式が壊れている場合は、ゲームを止めずにエラーメッセージを表示します。
- 保存される値は **プレハブ名・名前・位置・回転（度）・スケール** だけです。タグやコンポーネントの値（`TextLabel` の文字など）はプレハブの設定が使われます。
- スケールも保存されるので、あとでプレハブ側の既定スケールを変えても、保存済みのオブジェクトには反映されません。
- `OnEnter` で作るオブジェクトはまだ `FlushPending()` 前なので、`OnEnter` の中から JSON で配置したオブジェクトを探すことはできません。必要なら最初の `OnUpdate` で探してください。

#### ファイル形式

1 オブジェクト 1 行で書かれます（Git の差分が読みやすく、コンフリクトも起きにくいように）。手で編集しても構いません（`//` コメントも可）。`prefab` 以外の項目は省略できます。

```json
{
  "version": 1,
  "scene": "Game",
  "objects": [
    {"prefab":"enemy","name":"enemy","position":[120.0,-40.0,1.0],"rotation":0.0,"scale":[48.0,48.0,1.0]}
  ]
}
```

#### SceneLayout（コードから使う場合）

| 関数 | 説明 |
|---|---|
| `SceneLayout::Save(scene, sceneName)` | レイアウトオブジェクトを JSON に書き出す（一時ファイルに書いてから置き換えるので、途中で落ちても元のファイルは壊れない） |
| `SceneLayout::Load(scene, sceneName)` | JSON を読み込み、`InstantiateAsLayout` で生成する。通常は `SceneManager` が自動で呼ぶ |
| `SceneLayout::GetPath(sceneName)` | ファイルのパス（`../../assets/scenes/<シーン名>.json`） |

戻り値の `SceneLayout::Result` は `valid`（失敗で `false`）・`fileFound`・`objectCount`・`skippedCount`・`message` を持ちます。例外は投げません。

JSON の読み書きには [nlohmann/json](https://github.com/nlohmann/json) を使っています（vcpkg.json で自動インストール）。

### 注意点

- 保存されるのはレイアウトオブジェクトだけです。それ以外のオブジェクトへの変更は、リロード・シーン切り替え・モード切り替えで元に戻ります。
- 未保存の変更があっても、リロードやシーン切り替えで確認は出ません。こまめに保存（またはプレイ）してください。
- 編集モードではスクリプト（`CameraController` など）も止まるので、カメラは `OnEnter` 後の位置のままです。
- `OnEnter` やスクリプトで作ったオブジェクトも編集できますが、次回の生成時にはコードの値に戻ります。調整した値はコード側に反映してください。
- ビューでの選択は、カメラにズームがない前提（1 ワールド単位 = 1 ピクセル）で計算しています。
- 日本語表示のため、ImGui のフォントには `C:/Windows/Fonts/meiryo.ttc`（メイリオ）を使います。見つからない場合は英字のみの既定フォントになり、日本語が `?` で表示されます。
- Release ビルドでは F1 が無効なだけで、エディタのコード（ImGui を含む）はビルドに含まれています。

---

## 17. 拡張方法

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

`PrefabRegistry.cpp` の `RegisterPrefabs()` に `Register("名前", lambda)` を追加します。エディタの **プレハブ** ウィンドウにも自動で表示されます。

> 保存済みのシーン JSON はプレハブ名で参照しているので、既存のプレハブ名は変えないでください。

### 新しいタグを追加する

`ObjectTags.h` の `OBJECT_TAG_LIST` に 1 行追加するだけです。`enum class ObjectTag` とエディタのタグ一覧の両方に自動で反映されます。

```cpp
#define OBJECT_TAG_LIST(X) \
	X(DEFAULT)             \
	X(ENEMY)               \
	X(PLAYER)              \
	X(BLOCK)               \
	X(GROUND)              \
	X(ITEM)                // ← 追加
```

既存のタグの番号がずれないよう、新しいタグは**末尾に追加**してください。

### エディタに項目を追加する

インスペクターの表示は `EditorUI::DrawInspector` にあります。新しいコンポーネントを表示したい場合は、既存のセクションと同じ形で追加します。

```cpp
if (auto* hp = selectedObjRef.GetComponent<HealthComponent>())
{
    if (ImGui::CollapsingHeader("Health", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::DragInt("HP", &hp->hp, 1.f, 0, 999);
    }
}
```

値を変更したときに追加の処理が必要な場合は、「値をコピー → ImGui で編集 → 変更されたら Setter で書き戻す」形にしてください（Setter 内の処理を飛ばさないため）。

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
2. `App::Init()` で `sceneManager.Register<MyScene>("名前")` を追加する。エディタのシーン選択にも自動で表示されます。
3. `RequestChange("名前")` / `ChangeScene("名前")` で遷移する。

ステージ違いなど、配置だけが異なるシーンはクラスを増やさず、1 つのクラスを `GlobalContext` の値（ステージ番号など）で切り替えるのがおすすめです。

---

## 18. 注意点

- **初回は vcpkg の連携が必要**：`imgui.h` などが見つからない（C1083）場合は、「1. 初回セットアップ（vcpkg）」の `vcpkg integrate install` を実行してください。
- **Rigidbody + Collider が必須**：どちらか一方だけのオブジェクトは物理の対象になりません。
- **AddComponent の順序**：`Awake` 内で他コンポーネントを取得する場合、先に追加しておくこと。
- **`SetScale(float)` は乗算**：`SetStatic` / `AddAnimation` の後に使うこと。
- **プレハブ生成は遅延反映**：`Instantiate` したオブジェクトが `GetObjects()` に現れるのは `FlushPending()` 後です。`OnEnter` の中で `GetObjects()` を探しても、そのフレームに生成したものやシーン JSON のオブジェクトは見つかりません。
- **アセットのパス**：実行時のカレントディレクトリが変わるとテクスチャや `.cso` が見つからず、`assert` や `nullptr` 参照の原因になります。
- **シーン切り替えは遅延反映**：`RequestChange` は予約のみで、実際の切り替えはフレーム末の `sceneManager.Update()` で行われます。スクリプトの `Update` 中に呼んでも安全です。
- **遷移中・一時停止中はゲームが止まる**：`EXITING` / `ENTERING` の間とエディタで一時停止中は、`OnUpdate` / スクリプト / 物理 / アニメーションが更新されません（遷移時間 0 なら停止フレームはありません）。
- **切り替えでシーン内はすべて破棄**：`GameObject*` などシーン内オブジェクトへのポインタを、シーンをまたいで保持しないこと。残したい値は `GameContext::globalContext` へ。
- **`Clear()` 中の生成は無効**：シーン破棄中（`OnDestroy` 内など）の `Instantiate` / `Add2DObject` は `nullptr` を返します。
- **スクリーン空間フラグは描画ごとにリセット**：`Renderer::SetEnableScreenSpace(true)` を自分で使う場合は、描画後に必ず `false` に戻してください。戻さないと、それ以降の `TransformCBuffer` を使う描画がすべてスクリーン空間になります。
- **エディタで保存されるのはレイアウトオブジェクトだけ**：コードで作ったオブジェクトの調整値はコードに反映してください。
- **プレハブ名を変えない**：シーン JSON はプレハブ名で参照しているため、名前を変えると保存済みのオブジェクトが読み込まれなくなります。
- **`unique_ptr` の一覧はコピーしない**：`GetObjects()` をループするときは `for (const auto& obj : scene.GetObjects())` のように参照で受け取ってください（`auto obj` はコピーになり、コンパイルエラー C2280 になります）。
- **ヘッダーで `App.h` をインクルードしない**：`App.h` は多くのヘッダーを含むため、下位のクラスのヘッダーから読み込むと循環インクルードになり、型が見つからないエラー（C3646 など）の原因になります。