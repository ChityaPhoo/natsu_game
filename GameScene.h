#pragma once
#include "BossArmature.h"
#include "CameraController.h"
#include "DialogueSystem.h"
#include "MapChipField.h"
#include "Player.h"
#include "Skydome.h"
#include <array>
#include <cstddef>

class GameScene {
public:
	~GameScene();
	void Initialize();
	void Update();
	void Draw();
	bool ShouldRestartToTitle() const { return restartToTitleRequested_; }

private:
	enum class FlowState { kTitle, kTitleFadeOut, kPlay };
	enum class EndType { kNone, kPlayerDefeat, kBossDefeat };
	enum class EndPhase {
		kNone,
		kPlayerSlowMotion,
		kBossDefeatEffect,
		kBossDialogue,
		kFadeToBlack,
		kLogoFadeIn,
		kLogoWait,
		kLogoFadeOut,
	};

	struct DefeatParticle {
		KamataEngine::WorldTransform transform;
		KamataEngine::Vector3 velocity = {};
		float life = 0.0f;
		float maxLife = 1.0f;
		float initialScale = 0.1f;
		bool active = false;
	};

	void StartBossEncounter();
	void StartBossCombat();
	void UpdateTitle();
	void UpdateCombatCollisions();
	void ResolveBossBodyCollision();
	void StartPlayerDefeat();
	void StartBossDefeat();
	void UpdateEndSequence();
	void BeginFadeToResult();
	void SpawnBossDefeatParticles();
	void UpdateBossDefeatParticles();
	void DrawBossDefeatParticles(const KamataEngine::Camera& camera) const;
	void DrawEndOverlay() const;
	void DrawBossRangeVisual();
	void DrawCollisionDebug(const KamataEngine::Camera& camera) const;
	static bool Overlaps(
	    const KamataEngine::Vector3& minA,
	    const KamataEngine::Vector3& maxA,
	    const KamataEngine::Vector3& minB,
	    const KamataEngine::Vector3& maxB);
	static float SmoothStep(float t);
#ifdef USE_IMGUI
	void DrawCombatImGui();
#endif
	void StartHealthBarEntrance();
	void UpdateHealthBars();
	void DrawHealthBars() const;

	// =====================================================================
	// Game-flow tuning. These constants are the main place to adjust the
	// speed of scene, encounter, health-bar, and result-screen transitions.
	// =====================================================================
	static inline const float kCameraViewHalfWidth = 11.35f;
	static inline const float kCameraViewHalfHeight = 6.25f;
	static inline const float kBossVisibleLeftX = 51.50f;
	static inline const float kBossArenaCameraX = 48.65f;
	static inline const float kBossArenaCameraY = 6.25f;
	static inline const float kBossArenaPlayerMinX = 37.85f;
	static inline const float kBossArenaBossMinX = 40.50f;
	static inline const float kBossArenaBossMaxX = 57.0f;
	static inline const float kBossCameraEaseDuration = 1.10f;
	static inline const float kTitlePlayerX = 11.35f;
	static inline const float kTitleFadeDuration = 2.00f;
	static inline const float kHealthBarAppearDuration = 1.20f;
	static inline const float kFrameTime = 1.0f / 60.0f;

	// Result/defeat transition durations, in seconds.
	static inline const float kPlayerSlowMotionDuration = 1.25f;
	static inline const float kBossDefeatEffectDuration = 4.50f;
	static inline const float kScreenFadeDuration = 1.80f;
	static inline const float kResultLogoFadeInDuration = 2.25f;
	static inline const float kResultLogoFadeOutDuration = 1.50f;

	// =====================================================================
	// Combat tuning: maximum health and all currently implemented damage.
	// =====================================================================
	static inline const float kDamageInvincibilityDuration = 0.75f;
	static inline const int kPlayerMaximumHealth = 100;
	static inline const int kBossMaximumHealth = 100;
	static inline const int kPlayerAttackDamage = 10;
	static inline const int kBossBodyDamage = 8;
	static inline const int kScytheDamage = 20;

	// Boss-defeat particle tuning. Particle count changes the array size, so
	// rebuild the project after changing it.
	static inline constexpr std::size_t kBossDefeatParticleCount = 128;
	static inline const float kBossDefeatParticleMinimumSpeed = 2.4f;
	static inline const float kBossDefeatParticleAdditionalSpeed = 4.8f;
	static inline const float kBossDefeatParticleMinimumLife = 3.00f;
	static inline const float kBossDefeatParticleAdditionalLife = 2.00f;
	static inline const float kBossDefeatParticleMinimumScale = 0.10f;
	static inline const float kBossDefeatParticleAdditionalScale = 0.22f;
	static inline const float kBossDefeatParticleGravity = 1.55f;
	// Change this value to control how many sprite dialogue pages must be
	// advanced before the boss AI and health bars start.
	static inline const uint32_t kBossDialoguePageCount = 3;
	MapChipField* mapChipField_ = nullptr;
	Player* player_ = nullptr;
	CameraController* cameraController_ = nullptr;
	Skydome* skydome_ = nullptr;
	BossArmature* bossArmature_ = nullptr;
	DialogueSystem* dialogueSystem_ = nullptr;
	DialogueSystem* victoryDialogueSystem_ = nullptr;
	KamataEngine::Model* defeatParticleModel_ = nullptr;
	KamataEngine::ObjectColor defeatParticleColor_;
	std::array<DefeatParticle, kBossDefeatParticleCount> defeatParticles_;
	KamataEngine::Sprite* titleLogo_ = nullptr;
	KamataEngine::Sprite* blackOverlay_ = nullptr;
	KamataEngine::Sprite* whiteFlashOverlay_ = nullptr;
	KamataEngine::Sprite* playerHealthFrame_ = nullptr;
	KamataEngine::Sprite* playerHealthBackground_ = nullptr;
	KamataEngine::Sprite* playerHealthFill_ = nullptr;
	KamataEngine::Sprite* bossHealthFrame_ = nullptr;
	KamataEngine::Sprite* bossHealthBackground_ = nullptr;
	KamataEngine::Sprite* bossHealthFill_ = nullptr;
	std::array<KamataEngine::Sprite*, 5> bossRangeSprites_ = {};
	FlowState flowState_ = FlowState::kTitle;
	EndType endType_ = EndType::kNone;
	EndPhase endPhase_ = EndPhase::kNone;
	float titleFadeTimer_ = 0.0f;
	float healthBarAppearTimer_ = 0.0f;
	float damageInvincibilityTimer_ = 0.0f;
	float endPhaseTimer_ = 0.0f;
	float blackOverlayAlpha_ = 0.0f;
	float whiteFlashAlpha_ = 0.0f;
	float playerHealthRatio_ = 1.0f;
	float bossHealthRatio_ = 1.0f;
	KamataEngine::Vector3 defeatCameraBase_ = {};
	uint32_t effectRandomState_ = 0x9E3779B9u;
	uint32_t slowMotionFrameCounter_ = 0;
	int playerHealth_ = kPlayerMaximumHealth;
	int bossHealth_ = kBossMaximumHealth;
	bool healthBarsVisible_ = false;
	bool playerAttackHitBoss_ = false;
	bool showBossRangeVisual_ = false;
	bool showCollisionDebug_ = false;
	bool restartToTitleRequested_ = false;
	bool resultContinueRequested_ = false;
	bool bossEncounterStarted_ = false;
	bool bossDialogueStarted_ = false;
	bool bossAIStarted_ = false;
};
