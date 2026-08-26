#pragma once
#include "BossArmature.h"
#include "CameraController.h"
#include "DialogueSystem.h"
#include "MapChipField.h"
#include "Player.h"
#include "Skydome.h"
#include <array>
#include <cstddef>
#include <cstdint>

class GameScene {
public:
	~GameScene();
	void Initialize();
	void Update();
	void Draw();
	bool ShouldRestartToTitle() const { return restartToTitleRequested_; }

private:
	enum class FlowState {
		kIntroFadeIn,
		kIntroStay,
		kIntroFadeOut,
		kTitleLogoFadeIn,
		kTitleWorldFadeIn,
		kTitlePromptFadeIn,
		kTitle,
		kTitleFadeOut,
		kPlay,
	};
	enum class EndType { kNone, kPlayerDefeat, kBossDefeat };
	enum class BossPhaseState { kPhaseOne, kDialogue, kTransitionAnimation, kPhaseTwo };
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
	struct RingSrt {
		KamataEngine::Vector2 translation = {};
		float rotation = 0.0f;
		KamataEngine::Vector2 scale = {1.0f, 1.0f};
	};
	struct MiniatureSrt {
		KamataEngine::Vector2 translation = {};
		KamataEngine::Vector3 rotation = {};
		KamataEngine::Vector3 scale = {1.0f, 1.0f, 1.0f};
	};
	struct AudioClip {
		uint32_t soundHandle = 0;
		bool loaded = false;
	};
	enum class BgmTrack { kNone, kNovusOrdoSeclorum, kPhaseOneRequiem, kPhaseTwoGothic };

	void StartBossEncounter();
	void StartBossCombat();
	void StartBossPhaseDialogue();
	void StartBossPhaseAnimation();
	void UpdateBossPhaseSequence();
	bool IsBossPhaseSequenceActive() const;
	void UpdateTitle();
	void DrawTitleSequence() const;
	void UpdateBackgroundSprites();
	void DrawBackgroundSprites() const;
	void UpdateGameplayTutorialUi();
	void DrawGameplayTutorialUi() const;
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
	void DrawPhaseTwoAttackEffects(const KamataEngine::Camera& camera);
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
	void UpdateHealthPortraitCamera(
	    KamataEngine::Camera& camera,
	    const KamataEngine::Vector3& modelCenter,
	    const KamataEngine::Vector2& screenCenter,
	    float distance);
	void DrawDialogueSpeaker();
	void InitializeAudio();
	void UpdateAudio();
	void RequestBgm(BgmTrack track);
	void PlayTitleConfirmCue();
	void UpdateFootstepAudio(bool movementEnabled);
	void UpdatePlayerActionAudio();
	void UpdateBossActionAudio();
	void UpdateDialogueAudio();
	void PlayPlayerDamageCue();
	void PlayBossImpactCue();
	void PlayBossDefeatCue();
	void PlayResultCue();
	void StopAllAudio();
	const AudioClip* GetBgmClip(BgmTrack track) const;

	// =====================================================================
	// Game-flow tuning. These constants are the main place to adjust the
	// speed of scene, encounter, health-bar, and result-screen transitions.
	// =====================================================================
	static inline const float kCameraViewHalfWidth = 11.35f;
	static inline const float kCameraViewHalfHeight = 6.25f;
	static inline const float kMapWidth = 100.0f;
	static inline const float kBossVisibleLeftX = 91.50f;
	static inline const float kBossArenaCameraX = 88.65f;
	static inline const float kBossArenaCameraY = 6.25f;
	static inline const float kBossArenaPlayerMinX = 77.85f;
	static inline const float kBossArenaBossMinX = 80.50f;
	static inline const float kBossArenaBossMaxX = 97.0f;
	static inline const float kBossCameraEaseDuration = 1.10f;
	static inline const float kTitlePlayerX = 11.35f;

	// =====================================================================
	// Intro/title tuning, in seconds.
	// Full-screen 1280x720 intro artwork. It fades over the black title cover,
	// then the existing title sequence continues normally.
	// =====================================================================
	static inline const char* kIntroSpriteFile = "titleFont/intro.png";
	static inline const float kIntroSpriteFadeInDuration = 2.00f;
	static inline const float kIntroSpriteStayDuration = 2.50f;
	static inline const float kIntroSpriteFadeOutDuration = 2.00f;
	static inline const float kTitleLogoFadeInDuration = 2.50f;
	static inline const float kTitleWorldFadeInDuration = 2.00f;
	static inline const float kTitlePromptFadeInDuration = 1.25f;
	static inline const float kTitleStartFadeOutDuration = 2.00f;
	static inline const float kTitleStartBlinkDuration = 0.36f;
	static inline const float kTitleStartBlinkInterval = 0.06f;

	// Title idle movement and bottom prompt tuning.
	static inline const char* kTitlePromptSpriteFile = "titleFont/titleui.png";
	static inline const float kTitleLogoBaseY = 165.0f;
	static inline const float kTitleLogoIdleMoveAmount = 8.0f;
	static inline const float kTitleLogoIdleCycleDuration = 2.40f;
	static inline const float kTitlePromptBaseY = 675.0f;
	static inline const float kTitlePromptWidth = 560.0f;
	static inline const float kTitlePromptHeight = 56.0f;
	static inline const float kTitlePromptIdleMoveAmount = 5.0f;
	static inline const float kTitlePromptIdleCycleDuration = 1.90f;

	// Background tuning. Two screen-sized copies wrap horizontally. The scroll
	// ratio is relative to camera movement: 0 is fixed and 1 matches the world.
	static inline const char* kBackgroundSpriteFile = "BackGround/background.png";
	static inline const char* kMoonSpriteFile = "BackGround/moon1.png";
	static inline const float kBackgroundSpriteWidth = 870.4f;
	static inline const float kBackgroundSpriteHeight = 489.6f;
	static inline const float kBackgroundSpritePositionY = 230.4f;
	static inline const float kBackgroundScrollRatio = 0.30f;
	static inline const float kMoonPositionX = 1180.0f;
	static inline const float kMoonPositionY = 100.0f;
	static inline const float kMoonWidth = 128.0f;
	static inline const float kMoonHeight = 128.0f;
	static inline const float kMoonScaleAmount = 0.025f;
	static inline const float kMoonScaleCycleDuration = 3.20f;

	// Walking tutorial layout. The images live at fixed world positions, so they
	// scroll across the screen and the player walks past them like level objects.
	static inline const char* kGameplayUiOneSpriteFile = "titleFont/gameplayui1.png";
	static inline const char* kGameplayUiTwoSpriteFile = "titleFont/gameplayui2.png";
	static inline const float kGameplayUiOneWorldX = 34.05f;
	static inline const float kGameplayUiTwoWorldX = 56.75f;
	static inline const float kGameplayUiWorldY = 9.20f;
	static inline const float kGameplayUiWidth = 650.0f;
	static inline const float kGameplayUiHeight = 365.625f;
	static inline const float kGameplayUiIdleMoveAmount = 7.0f;
	static inline const float kGameplayUiIdleCycleDuration = 2.20f;

	static inline const float kHealthBarAppearDuration = 1.20f;
	static inline const char* kCharacterRingSpriteFile = "character_ring.png";
	static inline const float kPlayerHealthRingSize = 82.0f;
	static inline const float kBossHealthRingSize = 88.0f;
	static inline const float kPlayerPortraitCameraDistance = 5.0f;
	static inline const float kBossPortraitCameraDistance = 5.0f;
	static inline const float kPlayerPortraitBaseScale = 0.22f;
	static inline const float kBossPortraitBaseScale = 0.28f;
	static inline const float kDialogueSpeakerRingSize = 88.0f;
	static inline const float kDialogueSpeakerPositionX = 72.0f;
	static inline const float kDialogueSpeakerPositionY = 520.0f;
	static inline const float kFrameTime = 1.0f / 60.0f;

	// Result/defeat transition durations, in seconds.
	static inline const float kPlayerSlowMotionDuration = 1.25f;
	static inline const float kBossDefeatEffectDuration = 4.50f;
	static inline const float kScreenFadeDuration = 1.80f;
	static inline const float kResultLogoFadeInDuration = 2.25f;
	static inline const float kResultPromptDelay = 1.25f;
	static inline const float kResultPromptFadeInDuration = 0.55f;
	static inline const float kResultLogoFadeOutDuration = 1.50f;
	static inline const char* kGameOverSpriteFile = "gameover.png";
	static inline const char* kGameClearSpriteFile = "gameclear.png";
	static inline const float kResultSpriteWidth = 720.0f;
	static inline const float kResultSpriteHeight = 256.0f;

	// Audio files are relative to Resources/. The three music tracks are kept
	// separate so scene changes can crossfade without restarting the same track.
	static inline const char* kNovusOrdoSeclorumFile = "bgm/novus_ordo_seclorum.wav";
	static inline const char* kPhaseOneRequiemFile = "bgm/requiem_per_un_tradimento.wav";
	static inline const char* kPhaseTwoGothicFile = "bgm/gothic.wav";
	static inline const char* kGameOverCueFile = "sfx/gameover_church_bell.wav";
	static inline const char* kGameClearCueFile = "sfx/gameclear_victory_fanfare.wav";
	static inline const char* kTitleConfirmCueFile = "sfx/title_confirm.wav";
	static inline const char* kFootstepCueFile = "sfx/concrete_footstep.wav";
	static inline const char* kPlayerDamageCueFile = "sfx/player_damage.wav";
	static inline const char* kSwordSwingCueFile = "sfx/sword_swing.wav";
	static inline const char* kDialogueBlipCueFile = "sfx/dialogue_blip.wav";
	static inline const char* kPlayerJumpDashCueFile = "sfx/player_jump_dash.wav";
	static inline const char* kBossMoveCueFile = "sfx/boss_move.wav";
	static inline const char* kBossImpactCueFile = "sfx/boss_impact.wav";
	static inline const char* kBossDefeatCueFile = "sfx/boss_defeat_magic_death.wav";
	static inline const char* kGameOverFallbackFile = "mokugyo.wav";
	static inline const char* kGameClearFallbackFile = "fanfare.wav";
	static inline const float kBgmVolume = 0.38f;
	static inline const float kResultCueVolume = 0.82f;
	static inline const float kTitleConfirmCueVolume = 0.72f;
	static inline const float kFootstepCueVolume = 0.30f;
	static inline const float kPlayerDamageCueVolume = 0.62f;
	static inline const float kSwordSwingCueVolume = 0.52f;
	static inline const float kDialogueBlipCueVolume = 0.24f;
	static inline const float kPlayerJumpDashCueVolume = 0.23f;
	static inline const float kBossMoveCueVolume = 0.20f;
	static inline const float kBossImpactCueVolume = 0.27f;
	static inline const float kBossDefeatCueVolume = 0.52f;
	static inline const float kFootstepMinimumSpeed = 0.035f;
	static inline const float kBgmCrossfadeDuration = 1.20f;

	// =====================================================================
	// Combat tuning: maximum health and all currently implemented damage.
	// =====================================================================
	static inline const float kDamageInvincibilityDuration = 0.75f;
	static inline const int kDefaultPlayerMaximumHealth = 10;
	static inline const int kDefaultBossMaximumHealth = 30;
	// Health is measured in successful hits for now: every damaging attack
	// removes one HP, so these defaults are exactly 10 and 30 hits to defeat.
	static inline const int kPlayerAttackDamage = 1;
	static inline const int kBossBodyDamage = 1;
	static inline const int kScytheDamage = 1;
	static inline const int kJumpSlamDamage = 1;
	static inline const int kPhaseTwoGroundWaveDamage = 1;
	static inline const int kPhaseTwoPillarDamage = 1;
	static inline const float kPlayerHitShakeDuration = 0.22f;
	static inline const float kPlayerHitShakeIntensity = 0.18f;
	static inline const float kHookPullMaximumDistance = 6.00f;
	static inline const float kHookPullDuration = 0.55f;
	static inline const float kHookPullLiftAmount = 0.40f;
	static inline const float kHookPullStopPadding = 0.20f;
	static inline const float kBossSlamShakeDuration = 0.38f;
	static inline const float kBossSlamShakeIntensity = 0.32f;
	static inline const float kBossSlamGroundMinimumY = 2.00f;
	static inline const float kBossSlamGroundMaximumY = 2.72f;

	// Phase 2 begins when the boss reaches this fraction of maximum health.
	// The temporary transition is: compress -> power surge -> settle.
	static inline const float kBossPhaseTwoHealthRatio = 0.50f;
	static inline const float kBossPhaseTransitionAnimationDuration = 2.80f;
	static inline const uint32_t kBossPhaseDialoguePageCount = 6;
	// Regular encounter/phase dialogue box. The PNG includes a large transparent
	// canvas, so these values crop the visible panel before fitting it to 1/4 screen.
	static inline const char* kDialogueBoxSpriteFile = "dialougueBoxReal.png";
	static inline const float kDialogueBoxCropX = 147.0f;
	static inline const float kDialogueBoxCropY = 653.0f;
	static inline const float kDialogueBoxCropWidth = 1263.0f;
	static inline const float kDialogueBoxCropHeight = 194.0f;
	// Author future dialogue PNGs at exactly 820x203 pixels. That matches this
	// on-screen rectangle one-for-one and keeps text sharp without stretching.
	static inline const float kDialogueContentWidth = 820.0f;
	static inline const float kDialogueContentHeight = 203.0f;
	// The content PNGs have about ten transparent pixels before their text. This
	// places that first visible pixel at the dialogue's left text boundary and
	// centers the two-line artwork vertically inside the panel.
	static inline const float kDialogueContentOffsetX = -120.0f;
	static inline const float kDialogueContentOffsetY = 6.0f;
	static inline const float kHealthBarDrainEaseSpeed = 7.0f;

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
	static inline const uint32_t kBossDialoguePageCount = 5;
	MapChipField* mapChipField_ = nullptr;
	Player* player_ = nullptr;
	Player* playerHealthPortrait_ = nullptr;
	CameraController* cameraController_ = nullptr;
	Skydome* skydome_ = nullptr;
	BossArmature* bossArmature_ = nullptr;
	BossArmature* bossHealthPortrait_ = nullptr;
	KamataEngine::Camera playerHealthPortraitCamera_;
	KamataEngine::Camera bossHealthPortraitCamera_;
	KamataEngine::Camera dialogueSpeakerPortraitCamera_;
	DialogueSystem* dialogueSystem_ = nullptr;
	DialogueSystem* phaseDialogueSystem_ = nullptr;
	DialogueSystem* victoryDialogueSystem_ = nullptr;
	KamataEngine::Model* defeatParticleModel_ = nullptr;
	KamataEngine::Model* bossAttackModel_ = nullptr;
	KamataEngine::Model* bossAttackWaveModel_ = nullptr;
	KamataEngine::ObjectColor defeatParticleColor_;
	std::array<DefeatParticle, kBossDefeatParticleCount> defeatParticles_;
	std::array<KamataEngine::WorldTransform, BossArmature::kGroundWaveCount> groundWaveTransforms_;
	std::array<KamataEngine::WorldTransform, BossArmature::kShadowPillarCount> shadowPillarTransforms_;
	KamataEngine::Sprite* introSprite_ = nullptr;
	KamataEngine::Sprite* titleCoverSprite_ = nullptr;
	KamataEngine::Sprite* titleLogo_ = nullptr;
	KamataEngine::Sprite* gameOverSprite_ = nullptr;
	KamataEngine::Sprite* gameClearSprite_ = nullptr;
	KamataEngine::Sprite* titlePromptSprite_ = nullptr;
	std::array<KamataEngine::Sprite*, 3> backgroundSprites_ = {};
	KamataEngine::Sprite* moonSprite_ = nullptr;
	KamataEngine::Sprite* gameplayUiOneSprite_ = nullptr;
	KamataEngine::Sprite* gameplayUiTwoSprite_ = nullptr;
	KamataEngine::Sprite* blackOverlay_ = nullptr;
	KamataEngine::Sprite* whiteFlashOverlay_ = nullptr;
	KamataEngine::Sprite* playerHealthFrame_ = nullptr;
	KamataEngine::Sprite* playerHealthBackground_ = nullptr;
	KamataEngine::Sprite* playerHealthFill_ = nullptr;
	KamataEngine::Sprite* bossHealthFrame_ = nullptr;
	KamataEngine::Sprite* bossHealthBackground_ = nullptr;
	KamataEngine::Sprite* bossHealthFill_ = nullptr;
	KamataEngine::Sprite* playerHealthRing_ = nullptr;
	KamataEngine::Sprite* bossHealthRing_ = nullptr;
	KamataEngine::Sprite* dialogueSpeakerRing_ = nullptr;
	RingSrt playerHealthRingSrt_;
	RingSrt bossHealthRingSrt_;
	RingSrt dialogueSpeakerRingSrt_;
	MiniatureSrt playerHealthMiniatureSrt_;
	MiniatureSrt bossHealthMiniatureSrt_;
	MiniatureSrt dialogueSpeakerMiniatureSrt_;
	std::array<KamataEngine::Sprite*, 5> bossRangeSprites_ = {};
	AudioClip novusOrdoSeclorumClip_;
	AudioClip phaseOneRequiemClip_;
	AudioClip phaseTwoGothicClip_;
	AudioClip gameOverCueClip_;
	AudioClip gameClearCueClip_;
	AudioClip titleConfirmCueClip_;
	AudioClip footstepCueClip_;
	AudioClip playerDamageCueClip_;
	AudioClip swordSwingCueClip_;
	AudioClip dialogueBlipCueClip_;
	AudioClip playerJumpDashCueClip_;
	AudioClip bossMoveCueClip_;
	AudioClip bossImpactCueClip_;
	AudioClip bossDefeatCueClip_;
	BgmTrack currentBgmTrack_ = BgmTrack::kNone;
	BgmTrack nextBgmTrack_ = BgmTrack::kNone;
	uint32_t currentBgmVoice_ = 0;
	uint32_t nextBgmVoice_ = 0;
	uint32_t resultCueVoice_ = 0;
	uint32_t footstepVoice_ = 0;
	float bgmCrossfadeTimer_ = 0.0f;
	FlowState flowState_ = FlowState::kIntroFadeIn;
	EndType endType_ = EndType::kNone;
	EndPhase endPhase_ = EndPhase::kNone;
	BossPhaseState bossPhaseState_ = BossPhaseState::kPhaseOne;
	float titleFadeTimer_ = 0.0f;
	float titleIdleTimer_ = 0.0f;
	float titleStartBlinkTimer_ = 0.0f;
	float moonAnimationTimer_ = 0.0f;
	float gameplayUiIdleTimer_ = 0.0f;
	float titleCoverAlpha_ = 1.0f;
	KamataEngine::Vector4 introSpriteBaseColor_ = {1.0f, 1.0f, 1.0f, 1.0f};
	float healthBarAppearTimer_ = 0.0f;
	float damageInvincibilityTimer_ = 0.0f;
	float endPhaseTimer_ = 0.0f;
	float bossPhaseTimer_ = 0.0f;
	float blackOverlayAlpha_ = 0.0f;
	float whiteFlashAlpha_ = 0.0f;
	float playerHealthRatio_ = 1.0f;
	float bossHealthRatio_ = 1.0f;
	float displayedPlayerHealthRatio_ = 1.0f;
	float displayedBossHealthRatio_ = 1.0f;
	float dialogueBoxOpacity_ = 0.90f;
	KamataEngine::Vector3 defeatCameraBase_ = {};
	uint32_t effectRandomState_ = 0x9E3779B9u;
	uint32_t slowMotionFrameCounter_ = 0;
	int playerMaximumHealth_ = kDefaultPlayerMaximumHealth;
	int bossMaximumHealth_ = kDefaultBossMaximumHealth;
	int playerHealth_ = kDefaultPlayerMaximumHealth;
	int bossHealth_ = kDefaultBossMaximumHealth;
	bool healthBarsVisible_ = false;
	bool playerAttackHitBoss_ = false;
	bool hookPullApplied_ = false;
	bool throwHitApplied_ = false;
	bool showBossRangeVisual_ = false;
	bool showCollisionDebug_ = false;
	bool restartToTitleRequested_ = false;
	bool resultContinueRequested_ = false;
	bool bossEncounterStarted_ = false;
	bool bossDialogueStarted_ = false;
	bool bossAIStarted_ = false;
	bool currentBgmVoiceActive_ = false;
	bool nextBgmVoiceActive_ = false;
	bool bgmCrossfading_ = false;
	bool resultCueVoiceActive_ = false;
	bool footstepVoiceActive_ = false;
	bool resultCuePlayed_ = false;
};
