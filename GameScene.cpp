#include "GameScene.h"
#include "Matrix4x4Calculation.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#ifdef USE_IMGUI
#include <imgui.h>
#endif

using namespace KamataEngine;

void GameScene::Initialize() {
	mapChipField_ = new MapChipField();
	mapChipField_->Initialize("mapChip", "Resources/mapchip.csv", 1.0f, 1.0f);

	player_ = new Player();
	player_->Initialize();
	player_->SetMapChipField(mapChipField_);
	// The title and game share the same world. Starting at the camera center makes
	// the title-to-play transition continuous instead of repositioning the player.
	player_->SetPosition({kTitlePlayerX, 2.401f, 0.0f});

	cameraController_ = new CameraController();
	cameraController_->Initialize();
	cameraController_->GetCamera().farZ = 2000.0f;
	cameraController_->SetPlayer(player_);
	// These bounds match the camera frustum at Z=-15, so neither horizontal
	// outside space nor empty space below the floor can enter the view.
	cameraController_->SetMovableArea({
	    kCameraViewHalfWidth,
	    kMapWidth - kCameraViewHalfWidth,
	    kCameraViewHalfHeight,
	    20.0f - kCameraViewHalfHeight});
	cameraController_->ResetCameraPosition();

	skydome_ = new Skydome();
	skydome_->Initialize();
	bossArmature_ = new BossArmature();
	bossArmature_->Initialize();
	bossArmature_->SetAIEnabled(false);
	dialogueSystem_ = new DialogueSystem();
	dialogueSystem_->Initialize(
	    kBossDialoguePageCount,
	    std::vector<std::string>(kBossDialoguePageCount, kDialogueBoxSpriteFile),
	    0.25f,
	    {kDialogueBoxCropX, kDialogueBoxCropY},
	    {kDialogueBoxCropWidth, kDialogueBoxCropHeight});
	phaseDialogueSystem_ = new DialogueSystem();
	phaseDialogueSystem_->Initialize(
	    kBossPhaseDialoguePageCount,
	    std::vector<std::string>(kBossPhaseDialoguePageCount, kDialogueBoxSpriteFile),
	    0.25f,
	    {kDialogueBoxCropX, kDialogueBoxCropY},
	    {kDialogueBoxCropWidth, kDialogueBoxCropHeight});
	victoryDialogueSystem_ = new DialogueSystem();
	victoryDialogueSystem_->Initialize(
	    1,
	    {
	        // Add the full-screen post-boss dialogue sprite here later.
	        // Example: "dialogue/boss_defeated.png",
	    },
	    1.0f);
	// The post-boss dialogue is a separate full-screen black page. Its opacity
	// remains independently adjustable from the regular dialogue box.
	victoryDialogueSystem_->SetBaseColor({0.0f, 0.0f, 0.0f, 1.0f});
	dialogueSystem_->SetOpacity(dialogueBoxOpacity_);
	phaseDialogueSystem_->SetOpacity(dialogueBoxOpacity_);
	victoryDialogueSystem_->SetOpacity(defeatedDialogueOpacity_);
	defeatParticleModel_ = Model::CreateSphere(6, 6);
	defeatParticleColor_.Initialize();
	defeatParticleColor_.SetColor({1.0f, 0.24f, 0.06f, 0.90f});
	for (DefeatParticle& particle : defeatParticles_) {
		particle.transform.Initialize();
		particle.active = false;
	}
	const bool hasIntroSprite = kIntroSpriteFile[0] != '\0';
	const uint32_t introSpriteTexture = TextureManager::Load(hasIntroSprite ? kIntroSpriteFile : "white1x1.png");
	introSpriteBaseColor_ = {1.0f, 1.0f, 1.0f, 1.0f};
	introSprite_ = Sprite::Create(introSpriteTexture, {0.0f, 0.0f}, {introSpriteBaseColor_.x, introSpriteBaseColor_.y, introSpriteBaseColor_.z, 0.0f});
	introSprite_->SetSize({static_cast<float>(WinApp::kWindowWidth), static_cast<float>(WinApp::kWindowHeight)});

	const uint32_t titleLogoTexture = TextureManager::Load("titleFont/titlelogo.png");
	titleLogo_ = Sprite::Create(
	    titleLogoTexture,
	    {static_cast<float>(WinApp::kWindowWidth) * 0.5f, kTitleLogoBaseY},
	    {1.0f, 1.0f, 1.0f, 0.0f},
	    {0.5f, 0.5f});
	// Preserve the 252x102 source aspect ratio instead of stretching the logo.
	titleLogo_->SetSize({500.0f, 202.0f});
	const uint32_t titlePromptTexture = TextureManager::Load(kTitlePromptSpriteFile);
	titlePromptSprite_ = Sprite::Create(
	    titlePromptTexture,
	    {static_cast<float>(WinApp::kWindowWidth) * 0.5f, kTitlePromptBaseY},
	    {1.0f, 1.0f, 1.0f, 0.0f},
	    {0.5f, 0.5f});
	// titleui.png is a full-width 1280x128 strip authored for the bottom of the
	// screen, so keep its native layout and transparency.
	titlePromptSprite_->SetSize({kTitlePromptWidth, kTitlePromptHeight});
	const uint32_t backgroundTexture = TextureManager::Load(kBackgroundSpriteFile);
	for (Sprite*& backgroundSprite : backgroundSprites_) {
		backgroundSprite = Sprite::Create(backgroundTexture, {0.0f, 0.0f});
		backgroundSprite->SetSize({kBackgroundSpriteWidth, kBackgroundSpriteHeight});
	}
	const uint32_t moonTexture = TextureManager::Load(kMoonSpriteFile);
	moonSprite_ = Sprite::Create(
	    moonTexture, {kMoonPositionX, kMoonPositionY}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.5f});
	moonSprite_->SetSize({kMoonWidth, kMoonHeight});
	const uint32_t gameplayUiOneTexture = TextureManager::Load(kGameplayUiOneSpriteFile);
	gameplayUiOneSprite_ = Sprite::Create(
	    gameplayUiOneTexture, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 0.0f}, {0.5f, 0.5f});
	gameplayUiOneSprite_->SetSize({kGameplayUiWidth, kGameplayUiHeight});
	const uint32_t gameplayUiTwoTexture = TextureManager::Load(kGameplayUiTwoSpriteFile);
	gameplayUiTwoSprite_ = Sprite::Create(
	    gameplayUiTwoTexture, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 0.0f}, {0.5f, 0.5f});
	gameplayUiTwoSprite_->SetSize({kGameplayUiWidth, kGameplayUiHeight});
	const uint32_t healthBarTexture = TextureManager::Load("white1x1.png");
	const uint32_t playerHealthFillTexture = TextureManager::Load("hp.png");
	const uint32_t bossHealthFillTexture = TextureManager::Load("boss hp.png");
	titleCoverSprite_ = Sprite::Create(healthBarTexture, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f});
	titleCoverSprite_->SetSize({static_cast<float>(WinApp::kWindowWidth), static_cast<float>(WinApp::kWindowHeight)});
	playerHealthFrame_ = Sprite::Create(healthBarTexture, {0.0f, 0.0f}, {0.03f, 0.02f, 0.02f, 0.0f}, {0.0f, 0.5f});
	playerHealthBackground_ = Sprite::Create(healthBarTexture, {0.0f, 0.0f}, {0.025f, 0.09f, 0.035f, 0.0f}, {0.0f, 0.5f});
	playerHealthFill_ = Sprite::Create(playerHealthFillTexture, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 0.0f}, {0.0f, 0.5f});
	bossHealthFrame_ = Sprite::Create(healthBarTexture, {0.0f, 0.0f}, {0.025f, 0.015f, 0.015f, 0.0f}, {0.5f, 0.5f});
	bossHealthBackground_ = Sprite::Create(healthBarTexture, {0.0f, 0.0f}, {0.10f, 0.025f, 0.025f, 0.0f}, {0.5f, 0.5f});
	bossHealthFill_ = Sprite::Create(bossHealthFillTexture, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 0.0f}, {0.0f, 0.5f});
	blackOverlay_ = Sprite::Create(healthBarTexture, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f});
	blackOverlay_->SetSize({static_cast<float>(WinApp::kWindowWidth), static_cast<float>(WinApp::kWindowHeight)});
	whiteFlashOverlay_ = Sprite::Create(healthBarTexture, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 0.0f});
	whiteFlashOverlay_->SetSize({static_cast<float>(WinApp::kWindowWidth), static_cast<float>(WinApp::kWindowHeight)});
	for (Sprite*& rangeSprite : bossRangeSprites_) {
		rangeSprite = Sprite::Create(healthBarTexture, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 0.0f});
	}
	flowState_ = FlowState::kIntroFadeIn;
	endType_ = EndType::kNone;
	endPhase_ = EndPhase::kNone;
	bossPhaseState_ = BossPhaseState::kPhaseOne;
	titleFadeTimer_ = 0.0f;
	titleIdleTimer_ = 0.0f;
	gameplayUiIdleTimer_ = 0.0f;
	titleCoverAlpha_ = 1.0f;
	healthBarAppearTimer_ = 0.0f;
	damageInvincibilityTimer_ = 0.0f;
	endPhaseTimer_ = 0.0f;
	bossPhaseTimer_ = 0.0f;
	blackOverlayAlpha_ = 0.0f;
	whiteFlashAlpha_ = 0.0f;
	playerHealth_ = kPlayerMaximumHealth;
	bossHealth_ = kBossMaximumHealth;
	playerHealthRatio_ = 1.0f;
	bossHealthRatio_ = 1.0f;
	healthBarsVisible_ = false;
	playerAttackHitBoss_ = false;
	showBossRangeVisual_ = false;
	showCollisionDebug_ = false;
	restartToTitleRequested_ = false;
	resultContinueRequested_ = false;
	bossEncounterStarted_ = false;
	bossDialogueStarted_ = false;
	bossAIStarted_ = false;
	mapChipField_->Update();
	skydome_->Update(cameraController_->GetCamera());
	UpdateBackgroundSprites();
}

void GameScene::Update() {
	player_->UpdateIdleAnimation();
	if (flowState_ != FlowState::kPlay) {
		cameraController_->SetDebugMode(false, {});
		UpdateTitle();
		return;
	}
	if (endType_ != EndType::kNone) {
		cameraController_->SetDebugMode(false, {});
		UpdateEndSequence();
		UpdateHealthBars();
		mapChipField_->Update();
		skydome_->Update(cameraController_->GetCamera());
		UpdateBackgroundSprites();
		return;
	}

	const bool isBossEditing = bossArmature_->IsEditorCameraActive();
	const Vector3 bossDebugFocus = {
	    bossArmature_->GetPosition().x,
	    bossArmature_->GetPosition().y + 3.5f,
	    bossArmature_->GetPosition().z};
	cameraController_->SetDebugMode(isBossEditing, bossDebugFocus);

	// Freeze gameplay input while arranging a boss pose, in addition to the
	// normal encounter/dialogue/phase-transition freezes.
	if (!isBossEditing && (!bossEncounterStarted_ || bossAIStarted_) && !IsBossPhaseSequenceActive()) { player_->Update(); }
	cameraController_->Update();
	UpdateBackgroundSprites();
	UpdateGameplayTutorialUi();
	const float visibleRightEdge = cameraController_->GetCamera().translation_.x + kCameraViewHalfWidth;
	if (!bossEncounterStarted_ && visibleRightEdge >= kBossVisibleLeftX) {
		StartBossEncounter();
	}
	if (bossEncounterStarted_ && !bossDialogueStarted_ && cameraController_->IsLockComplete()) {
		bossDialogueStarted_ = true;
		dialogueSystem_->Start();
	}
	if (bossDialogueStarted_ && !bossAIStarted_) {
		dialogueSystem_->Update();
		if (dialogueSystem_->IsFinished()) { StartBossCombat(); }
	}
	UpdateBossPhaseSequence();
	bossArmature_->Update(player_->GetWorldTransform().translation_);
	if (!isBossEditing && bossAIStarted_ && !IsBossPhaseSequenceActive()) {
		UpdateCombatCollisions();
		if (endType_ == EndType::kNone && !IsBossPhaseSequenceActive()) { ResolveBossBodyCollision(); }
	}
	UpdateHealthBars();
#ifdef USE_IMGUI
	bossArmature_->DrawImGui();
	DrawCombatImGui();
#endif
	mapChipField_->Update();
	skydome_->Update(cameraController_->GetCamera());
}

void GameScene::StartBossCombat() {
	bossAIStarted_ = true;
	bossArmature_->SetAIEnabled(true);
	StartHealthBarEntrance();
}

bool GameScene::IsBossPhaseSequenceActive() const {
	return bossPhaseState_ == BossPhaseState::kDialogue || bossPhaseState_ == BossPhaseState::kTransitionAnimation;
}

void GameScene::StartBossPhaseDialogue() {
	if (bossPhaseState_ != BossPhaseState::kPhaseOne || endType_ != EndType::kNone || !bossAIStarted_) { return; }
	bossPhaseState_ = BossPhaseState::kDialogue;
	bossPhaseTimer_ = 0.0f;
	bossArmature_->SetAIEnabled(false);
	phaseDialogueSystem_->Start();
}

void GameScene::StartBossPhaseAnimation() {
	bossPhaseState_ = BossPhaseState::kTransitionAnimation;
	bossPhaseTimer_ = 0.0f;
	bossArmature_->BeginPhaseTransition();
}

void GameScene::UpdateBossPhaseSequence() {
	if (bossPhaseState_ == BossPhaseState::kDialogue) {
		phaseDialogueSystem_->Update();
		if (phaseDialogueSystem_->IsFinished()) { StartBossPhaseAnimation(); }
		return;
	}
	if (bossPhaseState_ != BossPhaseState::kTransitionAnimation) { return; }

	const float duration = (std::max)(kBossPhaseTransitionAnimationDuration, kFrameTime);
	bossPhaseTimer_ = (std::min)(bossPhaseTimer_ + kFrameTime, duration);
	bossArmature_->SetPhaseTransitionProgress(SmoothStep(bossPhaseTimer_ / duration));
	if (bossPhaseTimer_ >= duration) {
		bossArmature_->EndPhaseTransition();
		bossPhaseState_ = BossPhaseState::kPhaseTwo;
		bossPhaseTimer_ = 0.0f;
		// Phase 2 currently resumes the Phase 1 AI. Its new move set will be
		// selected here once those moves are authored.
		bossArmature_->SetAIEnabled(true);
	}
}

bool GameScene::Overlaps(
    const Vector3& minA,
    const Vector3& maxA,
    const Vector3& minB,
    const Vector3& maxB) {
	return minA.x <= maxB.x && maxA.x >= minB.x && minA.y <= maxB.y && maxA.y >= minB.y && minA.z <= maxB.z && maxA.z >= minB.z;
}

void GameScene::UpdateCombatCollisions() {
	damageInvincibilityTimer_ = (std::max)(0.0f, damageInvincibilityTimer_ - kFrameTime);
	const BossArmature::CollisionBox bossBody = bossArmature_->GetBodyHitbox();

	if (!player_->IsAttackActive()) {
		playerAttackHitBoss_ = false;
	} else if (!playerAttackHitBoss_) {
		const Player::AttackHitbox attack = player_->GetAttackHitbox();
		if (Overlaps(attack.min, attack.max, bossBody.min, bossBody.max)) {
			playerAttackHitBoss_ = true;
			bossHealth_ = (std::max)(0, bossHealth_ - kPlayerAttackDamage);
			bossHealthRatio_ = static_cast<float>(bossHealth_) / static_cast<float>(kBossMaximumHealth);
			if (bossHealth_ <= 0) {
				StartBossDefeat();
				return;
			}
			if (bossPhaseState_ == BossPhaseState::kPhaseOne && bossHealthRatio_ <= kBossPhaseTwoHealthRatio) {
				StartBossPhaseDialogue();
				return;
			}
		}
	}

	if (damageInvincibilityTimer_ > 0.0f || player_->IsDashInvincible()) { return; }
	const Player::AttackHitbox playerBody = player_->GetBodyHitbox();
	const bool scytheHit = bossArmature_->IsScytheAttackActive() && [&]() {
		const BossArmature::CollisionBox scythe = bossArmature_->GetScytheHitbox();
		return Overlaps(playerBody.min, playerBody.max, scythe.min, scythe.max);
	}();
	const bool bodyHit = bossArmature_->IsBodyAttackActive() && Overlaps(playerBody.min, playerBody.max, bossBody.min, bossBody.max);
	if (!scytheHit && !bodyHit) { return; }

	playerHealth_ = (std::max)(0, playerHealth_ - (scytheHit ? kScytheDamage : kBossBodyDamage));
	playerHealthRatio_ = static_cast<float>(playerHealth_) / static_cast<float>(kPlayerMaximumHealth);
	damageInvincibilityTimer_ = kDamageInvincibilityDuration;
	player_->NotifyDamage();
	cameraController_->StartShake(kPlayerHitShakeDuration, kPlayerHitShakeIntensity);
	if (scytheHit && bossArmature_->IsVerticalHookAttackActive()) {
		player_->StartPullToward(bossArmature_->GetPosition().x, kHookPullDistance, kHookPullDuration);
	}
	if (playerHealth_ <= 0) { StartPlayerDefeat(); }
}

void GameScene::ResolveBossBodyCollision() {
	const Player::AttackHitbox playerBody = player_->GetBodyHitbox();
	const BossArmature::CollisionBox bossBody = bossArmature_->GetBodyHitbox();
	if (!Overlaps(playerBody.min, playerBody.max, bossBody.min, bossBody.max)) { return; }

	const float playerHalfWidth = (playerBody.max.x - playerBody.min.x) * 0.5f;
	const float playerCenterX = (playerBody.min.x + playerBody.max.x) * 0.5f;
	const float bossCenterX = (bossBody.min.x + bossBody.max.x) * 0.5f;
	const float correctedX = playerCenterX < bossCenterX ? bossBody.min.x - playerHalfWidth : bossBody.max.x + playerHalfWidth;
	player_->ResolveHorizontalPush(correctedX);
}

float GameScene::SmoothStep(float t) {
	t = std::clamp(t, 0.0f, 1.0f);
	return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

void GameScene::StartPlayerDefeat() {
	if (endType_ != EndType::kNone) { return; }
	playerHealth_ = 0;
	playerHealthRatio_ = 0.0f;
	endType_ = EndType::kPlayerDefeat;
	endPhase_ = EndPhase::kPlayerSlowMotion;
	endPhaseTimer_ = 0.0f;
	slowMotionFrameCounter_ = 0;
	bossArmature_->SetAIEnabled(false);
	titleLogo_->SetPosition({static_cast<float>(WinApp::kWindowWidth) * 0.5f, static_cast<float>(WinApp::kWindowHeight) * 0.5f});
	titleLogo_->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
	blackOverlayAlpha_ = 0.0f;
	whiteFlashAlpha_ = 0.0f;
	resultContinueRequested_ = false;
}

void GameScene::StartBossDefeat() {
	if (endType_ != EndType::kNone) { return; }
	bossHealth_ = 0;
	bossHealthRatio_ = 0.0f;
	endType_ = EndType::kBossDefeat;
	endPhase_ = EndPhase::kBossDefeatEffect;
	endPhaseTimer_ = 0.0f;
	bossArmature_->SetAIEnabled(false);
	bossArmature_->SetVisible(true);
	bossArmature_->SetDefeatBrightness(1.0f);
	defeatCameraBase_ = cameraController_->GetCamera().translation_;
	blackOverlayAlpha_ = 0.0f;
	whiteFlashAlpha_ = 0.0f;
	resultContinueRequested_ = false;
	titleLogo_->SetPosition({static_cast<float>(WinApp::kWindowWidth) * 0.5f, static_cast<float>(WinApp::kWindowHeight) * 0.5f});
	titleLogo_->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
	SpawnBossDefeatParticles();
}

void GameScene::BeginFadeToResult() {
	endPhase_ = EndPhase::kFadeToBlack;
	endPhaseTimer_ = 0.0f;
	blackOverlayAlpha_ = 0.0f;
	whiteFlashAlpha_ = 0.0f;
}

void GameScene::UpdateEndSequence() {
	// Accept either result key as soon as the screen is fading to black. The
	// request is remembered until the logo finishes fading in, so a key press
	// during the transition is never lost.
	if (endPhase_ == EndPhase::kFadeToBlack || endPhase_ == EndPhase::kLogoFadeIn || endPhase_ == EndPhase::kLogoWait) {
		if (Input::GetInstance()->TriggerKey(DIK_SPACE) || Input::GetInstance()->TriggerKey(DIK_RETURN)) {
			resultContinueRequested_ = true;
		}
	}

	switch (endPhase_) {
	case EndPhase::kPlayerSlowMotion:
		endPhaseTimer_ += kFrameTime;
		++slowMotionFrameCounter_;
		if (slowMotionFrameCounter_ % 3u == 0u) {
			bossArmature_->Update(player_->GetWorldTransform().translation_);
		}
		if (endPhaseTimer_ >= kPlayerSlowMotionDuration) { BeginFadeToResult(); }
		break;
	case EndPhase::kBossDefeatEffect: {
		endPhaseTimer_ = (std::min)(endPhaseTimer_ + kFrameTime, kBossDefeatEffectDuration);
		const float progress = endPhaseTimer_ / kBossDefeatEffectDuration;
		const float easedProgress = SmoothStep(progress);
		bossArmature_->SetDefeatBrightness(1.0f + 4.0f * easedProgress);
		UpdateBossDefeatParticles();

		effectRandomState_ = effectRandomState_ * 1664525u + 1013904223u;
		const float randomX = static_cast<float>((effectRandomState_ >> 16u) & 0xFFFFu) / 32767.5f - 1.0f;
		effectRandomState_ = effectRandomState_ * 1664525u + 1013904223u;
		const float randomY = static_cast<float>((effectRandomState_ >> 16u) & 0xFFFFu) / 32767.5f - 1.0f;
		const float shakeStrength = 0.08f + 0.16f * (1.0f - progress);
		Camera& camera = cameraController_->GetCamera();
		camera.translation_ = {
		    defeatCameraBase_.x + randomX * shakeStrength,
		    defeatCameraBase_.y + randomY * shakeStrength,
		    defeatCameraBase_.z};
		camera.UpdateMatrix();

		const float flashDistance = std::abs(progress - 0.82f);
		whiteFlashAlpha_ = flashDistance < 0.12f ? (1.0f - flashDistance / 0.12f) * 0.72f : 0.0f;
		if (progress >= 0.88f) { bossArmature_->SetVisible(false); }
		if (endPhaseTimer_ >= kBossDefeatEffectDuration) {
			camera.translation_ = defeatCameraBase_;
			camera.UpdateMatrix();
			whiteFlashAlpha_ = 0.0f;
			healthBarsVisible_ = false;
			victoryDialogueSystem_->Start();
			endPhase_ = EndPhase::kBossDialogue;
			endPhaseTimer_ = 0.0f;
		}
		break;
	}
	case EndPhase::kBossDialogue:
		victoryDialogueSystem_->Update();
		if (victoryDialogueSystem_->IsFinished()) { BeginFadeToResult(); }
		break;
	case EndPhase::kFadeToBlack:
		endPhaseTimer_ = (std::min)(endPhaseTimer_ + kFrameTime, kScreenFadeDuration);
		blackOverlayAlpha_ = SmoothStep(endPhaseTimer_ / kScreenFadeDuration);
		if (endPhaseTimer_ >= kScreenFadeDuration) {
			endPhase_ = EndPhase::kLogoFadeIn;
			endPhaseTimer_ = 0.0f;
		}
		break;
	case EndPhase::kLogoFadeIn: {
		endPhaseTimer_ = (std::min)(endPhaseTimer_ + kFrameTime, kResultLogoFadeInDuration);
		const float alpha = SmoothStep(endPhaseTimer_ / kResultLogoFadeInDuration);
		titleLogo_->SetColor({1.0f, 1.0f, 1.0f, alpha});
		blackOverlayAlpha_ = 1.0f;
		if (endPhaseTimer_ >= kResultLogoFadeInDuration) {
			endPhase_ = EndPhase::kLogoWait;
			endPhaseTimer_ = 0.0f;
		}
		break;
	}
	case EndPhase::kLogoWait: {
		if (resultContinueRequested_) {
			endPhase_ = EndPhase::kLogoFadeOut;
			endPhaseTimer_ = 0.0f;
			resultContinueRequested_ = false;
		}
		break;
	}
	case EndPhase::kLogoFadeOut: {
		endPhaseTimer_ = (std::min)(endPhaseTimer_ + kFrameTime, kResultLogoFadeOutDuration);
		const float alpha = 1.0f - SmoothStep(endPhaseTimer_ / kResultLogoFadeOutDuration);
		titleLogo_->SetColor({1.0f, 1.0f, 1.0f, alpha});
		if (endPhaseTimer_ >= kResultLogoFadeOutDuration) { restartToTitleRequested_ = true; }
		break;
	}
	case EndPhase::kNone:
		break;
	}

	blackOverlay_->SetColor({0.0f, 0.0f, 0.0f, blackOverlayAlpha_});
	whiteFlashOverlay_->SetColor({1.0f, 1.0f, 1.0f, whiteFlashAlpha_});
}

void GameScene::SpawnBossDefeatParticles() {
	const Vector3 bossPosition = bossArmature_->GetPosition();
	for (size_t index = 0; index < defeatParticles_.size(); ++index) {
		DefeatParticle& particle = defeatParticles_[index];
		effectRandomState_ = effectRandomState_ * 1664525u + 1013904223u;
		const float randomValue = static_cast<float>((effectRandomState_ >> 16u) & 0xFFFFu) / 65535.0f;
		const float angle = static_cast<float>(index) / static_cast<float>(defeatParticles_.size()) * 2.0f * std::numbers::pi_v<float> + (randomValue - 0.5f) * 0.28f;
		effectRandomState_ = effectRandomState_ * 1664525u + 1013904223u;
		const float speedRandom = static_cast<float>((effectRandomState_ >> 16u) & 0xFFFFu) / 65535.0f;
		const float speed = kBossDefeatParticleMinimumSpeed + speedRandom * kBossDefeatParticleAdditionalSpeed;
		particle.velocity = {std::cos(angle) * speed, std::sin(angle) * speed + 0.8f, 0.0f};
		particle.maxLife = kBossDefeatParticleMinimumLife + speedRandom * kBossDefeatParticleAdditionalLife;
		particle.life = particle.maxLife;
		particle.initialScale = kBossDefeatParticleMinimumScale + randomValue * kBossDefeatParticleAdditionalScale;
		particle.active = true;
		particle.transform.translation_ = {bossPosition.x, bossPosition.y + 4.0f, bossPosition.z};
		particle.transform.scale_ = {particle.initialScale, particle.initialScale, particle.initialScale};
		particle.transform.matWorld_ = Matrix4x4Calculation::MakeAffineMatrix(
		    particle.transform.scale_, particle.transform.rotation_, particle.transform.translation_);
		particle.transform.TransferMatrix();
	}
}

void GameScene::UpdateBossDefeatParticles() {
	for (DefeatParticle& particle : defeatParticles_) {
		if (!particle.active) { continue; }
		particle.life = (std::max)(0.0f, particle.life - kFrameTime);
		if (particle.life <= 0.0f) {
			particle.active = false;
			continue;
		}
		particle.transform.translation_.x += particle.velocity.x * kFrameTime;
		particle.transform.translation_.y += particle.velocity.y * kFrameTime;
		particle.velocity.y -= kBossDefeatParticleGravity * kFrameTime;
		const float scale = particle.initialScale * (particle.life / particle.maxLife);
		particle.transform.scale_ = {scale, scale, scale};
		particle.transform.matWorld_ = Matrix4x4Calculation::MakeAffineMatrix(
		    particle.transform.scale_, particle.transform.rotation_, particle.transform.translation_);
		particle.transform.TransferMatrix();
	}
}

void GameScene::StartHealthBarEntrance() {
	healthBarsVisible_ = true;
	healthBarAppearTimer_ = 0.0f;
}

void GameScene::UpdateHealthBars() {
	if (!healthBarsVisible_) { return; }

	healthBarAppearTimer_ = (std::min)(healthBarAppearTimer_ + kFrameTime, kHealthBarAppearDuration);
	const float t = healthBarAppearTimer_ / kHealthBarAppearDuration;
	const float easedT = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
	const float windowWidth = static_cast<float>(WinApp::kWindowWidth);
	const float windowHeight = static_cast<float>(WinApp::kWindowHeight);

	const float playerX = -360.0f + (36.0f + 360.0f) * easedT;
	constexpr float playerY = 42.0f;
	constexpr float playerFrameWidth = 350.0f;
	constexpr float playerBarWidth = 336.0f;
	playerHealthFrame_->SetPosition({playerX, playerY});
	playerHealthFrame_->SetSize({playerFrameWidth, 22.0f});
	playerHealthBackground_->SetPosition({playerX + 7.0f, playerY});
	playerHealthBackground_->SetSize({playerBarWidth, 10.0f});
	playerHealthFill_->SetPosition({playerX + 7.0f, playerY});
	playerHealthFill_->SetSize({playerBarWidth * std::clamp(playerHealthRatio_, 0.0f, 1.0f), 10.0f});

	const float bossY = windowHeight + 36.0f + ((windowHeight - 56.0f) - (windowHeight + 36.0f)) * easedT;
	constexpr float bossFrameWidth = 1040.0f;
	constexpr float bossBarWidth = 1022.0f;
	bossHealthFrame_->SetPosition({windowWidth * 0.5f, bossY});
	bossHealthFrame_->SetSize({bossFrameWidth, 24.0f});
	bossHealthBackground_->SetPosition({windowWidth * 0.5f, bossY});
	bossHealthBackground_->SetSize({bossBarWidth, 10.0f});
	bossHealthFill_->SetPosition({windowWidth * 0.5f - bossBarWidth * 0.5f, bossY});
	bossHealthFill_->SetSize({bossBarWidth * std::clamp(bossHealthRatio_, 0.0f, 1.0f), 10.0f});

	playerHealthFrame_->SetColor({0.03f, 0.02f, 0.02f, 0.96f * easedT});
	playerHealthBackground_->SetColor({0.025f, 0.09f, 0.035f, 0.94f * easedT});
	playerHealthFill_->SetColor({1.0f, 1.0f, 1.0f, easedT});
	bossHealthFrame_->SetColor({0.025f, 0.015f, 0.015f, 0.96f * easedT});
	bossHealthBackground_->SetColor({0.10f, 0.025f, 0.025f, 0.94f * easedT});
	bossHealthFill_->SetColor({1.0f, 1.0f, 1.0f, easedT});
}

void GameScene::DrawHealthBars() const {
	if (!healthBarsVisible_) { return; }

	Sprite::PreDraw();
	playerHealthFrame_->Draw();
	playerHealthBackground_->Draw();
	playerHealthFill_->Draw();
	bossHealthFrame_->Draw();
	bossHealthBackground_->Draw();
	bossHealthFill_->Draw();
	Sprite::PostDraw();
}

void GameScene::DrawBossRangeVisual() {
	if (!showBossRangeVisual_ || !bossAIStarted_ || endType_ != EndType::kNone) { return; }
	const Camera& camera = cameraController_->GetCamera();
	const Vector3 bossPosition = bossArmature_->GetPosition();
	const float depth = (std::max)(bossPosition.z - camera.translation_.z, 0.1f);
	const float halfViewWidth = std::tan(camera.fovAngleY * 0.5f) * depth * camera.aspectRatio;
	const float windowWidth = static_cast<float>(WinApp::kWindowWidth);
	const float windowHeight = static_cast<float>(WinApp::kWindowHeight);
	auto worldToScreenX = [&](float worldX) {
		const float normalized = (worldX - (camera.translation_.x - halfViewWidth)) / (halfViewWidth * 2.0f);
		return std::clamp(normalized * windowWidth, 0.0f, windowWidth);
	};
	auto setBand = [&](Sprite* sprite, float left, float right, const Vector4& color) {
		const float width = (std::max)(0.0f, right - left);
		sprite->SetPosition({left, 0.0f});
		sprite->SetSize({width, windowHeight});
		sprite->SetColor(color);
	};

	const float nearLeft = worldToScreenX(bossPosition.x - bossArmature_->GetCloseDistance());
	const float nearRight = worldToScreenX(bossPosition.x + bossArmature_->GetCloseDistance());
	const float midLeft = worldToScreenX(bossPosition.x - bossArmature_->GetMidDistance());
	const float midRight = worldToScreenX(bossPosition.x + bossArmature_->GetMidDistance());
	setBand(bossRangeSprites_[0], 0.0f, midLeft, {0.20f, 0.80f, 0.30f, 0.10f});
	setBand(bossRangeSprites_[1], midRight, windowWidth, {0.20f, 0.80f, 0.30f, 0.10f});
	setBand(bossRangeSprites_[2], midLeft, nearLeft, {1.0f, 0.85f, 0.20f, 0.12f});
	setBand(bossRangeSprites_[3], nearRight, midRight, {1.0f, 0.85f, 0.20f, 0.12f});
	setBand(bossRangeSprites_[4], nearLeft, nearRight, {1.0f, 0.20f, 0.20f, 0.14f});
	Sprite::PreDraw();
	for (Sprite* sprite : bossRangeSprites_) { sprite->Draw(); }
	Sprite::PostDraw();
}

void GameScene::DrawCollisionDebug(const Camera& camera) const {
	if (!showCollisionDebug_ || !bossAIStarted_ || endType_ != EndType::kNone) { return; }
	PrimitiveDrawer* drawer = PrimitiveDrawer::GetInstance();
	drawer->SetCamera(&camera);
	auto drawBox = [&](const Vector3& minimum, const Vector3& maximum, const Vector4& color) {
		const float z = 0.10f;
		const Vector3 bottomLeft = {minimum.x, minimum.y, z};
		const Vector3 bottomRight = {maximum.x, minimum.y, z};
		const Vector3 topLeft = {minimum.x, maximum.y, z};
		const Vector3 topRight = {maximum.x, maximum.y, z};
		drawer->DrawLine3d(bottomLeft, bottomRight, color);
		drawer->DrawLine3d(bottomRight, topRight, color);
		drawer->DrawLine3d(topRight, topLeft, color);
		drawer->DrawLine3d(topLeft, bottomLeft, color);
	};
	const Player::AttackHitbox playerBody = player_->GetBodyHitbox();
	drawBox(playerBody.min, playerBody.max, {0.10f, 1.0f, 0.20f, 1.0f});
	if (bossArmature_->IsBodyAttackActive()) {
		const BossArmature::CollisionBox bossBody = bossArmature_->GetBodyHitbox();
		drawBox(bossBody.min, bossBody.max, {1.0f, 0.10f, 0.10f, 1.0f});
	}
	if (bossArmature_->IsScytheAttackActive()) {
		const BossArmature::CollisionBox weapon = bossArmature_->GetScytheHitbox();
		drawBox(weapon.min, weapon.max, {1.0f, 0.15f, 0.05f, 1.0f});
	}
	if (player_->IsAttackActive()) {
		const Player::AttackHitbox attack = player_->GetAttackHitbox();
		drawBox(attack.min, attack.max, {0.20f, 0.65f, 1.0f, 1.0f});
	}
}

void GameScene::DrawBossDefeatParticles(const Camera& camera) const {
	if (endPhase_ != EndPhase::kBossDefeatEffect || defeatParticleModel_ == nullptr) { return; }
	Model::PreDraw(Model::CullingMode::kNone, Model::BlendMode::kAdd, Model::DepthTestMode::kOff);
	for (const DefeatParticle& particle : defeatParticles_) {
		if (particle.active) { defeatParticleModel_->Draw(particle.transform, camera, &defeatParticleColor_); }
	}
	Model::PostDraw();
}

void GameScene::DrawEndOverlay() const {
	if (endType_ == EndType::kNone) { return; }
	Sprite::PreDraw();
	if (whiteFlashAlpha_ > 0.0f) { whiteFlashOverlay_->Draw(); }
	if (blackOverlayAlpha_ > 0.0f) { blackOverlay_->Draw(); }
	if (endPhase_ == EndPhase::kLogoFadeIn || endPhase_ == EndPhase::kLogoWait || endPhase_ == EndPhase::kLogoFadeOut) {
		titleLogo_->Draw();
	}
	Sprite::PostDraw();
}

#ifdef USE_IMGUI
void GameScene::DrawCombatImGui() {
	ImGui::SetNextWindowPos(ImVec2(930.0f, 20.0f), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(330.0f, 275.0f), ImGuiCond_Always);
	ImGui::Begin("Combat Debug");
	ImGui::Checkbox("Show boss AI ranges", &showBossRangeVisual_);
	ImGui::Checkbox("Show collision boxes", &showCollisionDebug_);
	ImGui::Text("Player health: %d / %d", playerHealth_, kPlayerMaximumHealth);
	ImGui::Text("Boss health: %d / %d", bossHealth_, kBossMaximumHealth);
	ImGui::Text("Damage: Player %d | Body %d | Scythe %d", kPlayerAttackDamage, kBossBodyDamage, kScytheDamage);
	if (ImGui::SliderFloat("Dialogue box opacity", &dialogueBoxOpacity_, 0.0f, 1.0f, "%.2f")) {
		dialogueSystem_->SetOpacity(dialogueBoxOpacity_);
		phaseDialogueSystem_->SetOpacity(dialogueBoxOpacity_);
	}
	if (ImGui::SliderFloat("Defeat dialogue opacity", &defeatedDialogueOpacity_, 0.0f, 1.0f, "%.2f")) {
		victoryDialogueSystem_->SetOpacity(defeatedDialogueOpacity_);
	}
	const char* phaseName = "Phase 1";
	if (bossPhaseState_ == BossPhaseState::kDialogue) { phaseName = "Phase dialogue"; }
	if (bossPhaseState_ == BossPhaseState::kTransitionAnimation) { phaseName = "Phase transition"; }
	if (bossPhaseState_ == BossPhaseState::kPhaseTwo) { phaseName = "Phase 2"; }
	ImGui::Text("Boss phase: %s", phaseName);
	if (bossAIStarted_ && bossPhaseState_ == BossPhaseState::kPhaseOne && ImGui::Button("Test Phase 2 Transition")) {
		bossHealth_ = (std::min)(bossHealth_, static_cast<int>(static_cast<float>(kBossMaximumHealth) * kBossPhaseTwoHealthRatio));
		bossHealthRatio_ = static_cast<float>(bossHealth_) / static_cast<float>(kBossMaximumHealth);
		StartBossPhaseDialogue();
	}
	if (ImGui::Button("Instant Player Lose")) { StartPlayerDefeat(); }
	ImGui::SameLine();
	if (ImGui::Button("Instant Boss Lose")) { StartBossDefeat(); }
	ImGui::End();
}
#endif

void GameScene::UpdateTitle() {
	auto advanceTimer = [&](float duration) {
		const float safeDuration = (std::max)(duration, kFrameTime);
		titleFadeTimer_ = (std::min)(titleFadeTimer_ + kFrameTime, safeDuration);
		return titleFadeTimer_ / safeDuration;
	};
	auto updateIdleMovement = [&]() {
		titleIdleTimer_ += kFrameTime;
		const float logoCycle = (std::max)(kTitleLogoIdleCycleDuration, kFrameTime);
		const float promptCycle = (std::max)(kTitlePromptIdleCycleDuration, kFrameTime);
		const float logoOffset = std::sin(titleIdleTimer_ / logoCycle * 2.0f * std::numbers::pi_v<float>) * kTitleLogoIdleMoveAmount;
		const float promptOffset = std::sin(titleIdleTimer_ / promptCycle * 2.0f * std::numbers::pi_v<float>) * kTitlePromptIdleMoveAmount;
		titleLogo_->SetPosition({static_cast<float>(WinApp::kWindowWidth) * 0.5f, kTitleLogoBaseY + logoOffset});
		titlePromptSprite_->SetPosition({static_cast<float>(WinApp::kWindowWidth) * 0.5f, kTitlePromptBaseY + promptOffset});
	};

	switch (flowState_) {
	case FlowState::kIntroFadeIn: {
		const float progress = SmoothStep(advanceTimer(kIntroSpriteFadeInDuration));
		introSprite_->SetColor({introSpriteBaseColor_.x, introSpriteBaseColor_.y, introSpriteBaseColor_.z, progress});
		if (titleFadeTimer_ >= (std::max)(kIntroSpriteFadeInDuration, kFrameTime)) {
			flowState_ = FlowState::kIntroStay;
			titleFadeTimer_ = 0.0f;
		}
		break;
	}
	case FlowState::kIntroStay:
		introSprite_->SetColor({introSpriteBaseColor_.x, introSpriteBaseColor_.y, introSpriteBaseColor_.z, 1.0f});
		advanceTimer(kIntroSpriteStayDuration);
		if (titleFadeTimer_ >= (std::max)(kIntroSpriteStayDuration, kFrameTime)) {
			flowState_ = FlowState::kIntroFadeOut;
			titleFadeTimer_ = 0.0f;
		}
		break;
	case FlowState::kIntroFadeOut: {
		const float progress = SmoothStep(advanceTimer(kIntroSpriteFadeOutDuration));
		introSprite_->SetColor({introSpriteBaseColor_.x, introSpriteBaseColor_.y, introSpriteBaseColor_.z, 1.0f - progress});
		if (titleFadeTimer_ >= (std::max)(kIntroSpriteFadeOutDuration, kFrameTime)) {
			flowState_ = FlowState::kTitleLogoFadeIn;
			titleFadeTimer_ = 0.0f;
			titleLogo_->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
		}
		break;
	}
	case FlowState::kTitleLogoFadeIn: {
		const float progress = SmoothStep(advanceTimer(kTitleLogoFadeInDuration));
		titleLogo_->SetColor({1.0f, 1.0f, 1.0f, progress});
		if (titleFadeTimer_ >= (std::max)(kTitleLogoFadeInDuration, kFrameTime)) {
			flowState_ = FlowState::kTitleWorldFadeIn;
			titleFadeTimer_ = 0.0f;
		}
		break;
	}
	case FlowState::kTitleWorldFadeIn: {
		const float progress = SmoothStep(advanceTimer(kTitleWorldFadeInDuration));
		titleCoverAlpha_ = 1.0f - progress;
		titleCoverSprite_->SetColor({0.0f, 0.0f, 0.0f, titleCoverAlpha_});
		if (titleFadeTimer_ >= (std::max)(kTitleWorldFadeInDuration, kFrameTime)) {
			flowState_ = FlowState::kTitlePromptFadeIn;
			titleFadeTimer_ = 0.0f;
			titleIdleTimer_ = 0.0f;
			titleCoverAlpha_ = 0.0f;
		}
		break;
	}
	case FlowState::kTitlePromptFadeIn: {
		updateIdleMovement();
		const float progress = SmoothStep(advanceTimer(kTitlePromptFadeInDuration));
		titlePromptSprite_->SetColor({1.0f, 1.0f, 1.0f, progress});
		if (titleFadeTimer_ >= (std::max)(kTitlePromptFadeInDuration, kFrameTime)) {
			flowState_ = FlowState::kTitle;
			titleFadeTimer_ = 0.0f;
		}
		break;
	}
	case FlowState::kTitle:
		updateIdleMovement();
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			flowState_ = FlowState::kTitleFadeOut;
			titleFadeTimer_ = 0.0f;
		}
		break;
	case FlowState::kTitleFadeOut: {
		updateIdleMovement();
		const float progress = SmoothStep(advanceTimer(kTitleStartFadeOutDuration));
		titleLogo_->SetColor({1.0f, 1.0f, 1.0f, 1.0f - progress});
		titlePromptSprite_->SetColor({1.0f, 1.0f, 1.0f, 1.0f - progress});
		if (titleFadeTimer_ >= (std::max)(kTitleStartFadeOutDuration, kFrameTime)) { flowState_ = FlowState::kPlay; }
		break;
	}
	case FlowState::kPlay:
		break;
	}

	mapChipField_->Update();
	skydome_->Update(cameraController_->GetCamera());
	UpdateBackgroundSprites();
}

void GameScene::DrawTitleSequence() const {
	if (flowState_ == FlowState::kPlay) { return; }

	Sprite::PreDraw();
	if (flowState_ == FlowState::kIntroFadeIn || flowState_ == FlowState::kIntroStay || flowState_ == FlowState::kIntroFadeOut) {
		titleCoverSprite_->Draw();
		introSprite_->Draw();
	} else {
		if (flowState_ == FlowState::kTitleLogoFadeIn || flowState_ == FlowState::kTitleWorldFadeIn) {
			titleCoverSprite_->Draw();
		}
		titleLogo_->Draw();
		if (flowState_ == FlowState::kTitlePromptFadeIn || flowState_ == FlowState::kTitle || flowState_ == FlowState::kTitleFadeOut) {
			titlePromptSprite_->Draw();
		}
	}
	Sprite::PostDraw();
}

void GameScene::UpdateBackgroundSprites() {
	if (backgroundSprites_[0] == nullptr || backgroundSprites_.back() == nullptr) { return; }
	const Camera& camera = cameraController_->GetCamera();
	const float pixelsPerWorldX = static_cast<float>(WinApp::kWindowWidth) / (kCameraViewHalfWidth * 2.0f);
	const float scrollPixels =
	    (camera.translation_.x - kTitlePlayerX) * pixelsPerWorldX * kBackgroundScrollRatio;
	float wrappedScroll = std::fmod(scrollPixels, kBackgroundSpriteWidth);
	if (wrappedScroll < 0.0f) { wrappedScroll += kBackgroundSpriteWidth; }
	for (size_t index = 0; index < backgroundSprites_.size(); ++index) {
		backgroundSprites_[index]->SetPosition(
		    {static_cast<float>(index) * kBackgroundSpriteWidth - wrappedScroll, kBackgroundSpritePositionY});
	}
}

void GameScene::DrawBackgroundSprites() const {
	Sprite::PreDraw();
	for (Sprite* backgroundSprite : backgroundSprites_) { backgroundSprite->Draw(); }
	moonSprite_->Draw();
	Sprite::PostDraw();
}

void GameScene::UpdateGameplayTutorialUi() {
	if (gameplayUiOneSprite_ == nullptr || gameplayUiTwoSprite_ == nullptr) { return; }

	gameplayUiIdleTimer_ += kFrameTime;
	const float idleCycle = (std::max)(kGameplayUiIdleCycleDuration, kFrameTime);
	const float idleOffset =
	    std::sin(gameplayUiIdleTimer_ / idleCycle * 2.0f * std::numbers::pi_v<float>) * kGameplayUiIdleMoveAmount;
	const Camera& camera = cameraController_->GetCamera();
	const float pixelsPerWorldX = static_cast<float>(WinApp::kWindowWidth) / (kCameraViewHalfWidth * 2.0f);
	const float pixelsPerWorldY = static_cast<float>(WinApp::kWindowHeight) / (kCameraViewHalfHeight * 2.0f);
	auto worldToScreen = [&](float worldX) {
		return Vector2{
		    static_cast<float>(WinApp::kWindowWidth) * 0.5f + (worldX - camera.translation_.x) * pixelsPerWorldX,
		    static_cast<float>(WinApp::kWindowHeight) * 0.5f - (kGameplayUiWorldY - camera.translation_.y) * pixelsPerWorldY + idleOffset};
	};
	gameplayUiOneSprite_->SetPosition(worldToScreen(kGameplayUiOneWorldX));
	gameplayUiTwoSprite_->SetPosition(worldToScreen(kGameplayUiTwoWorldX));
	const float alpha = !bossEncounterStarted_ && endType_ == EndType::kNone ? 1.0f : 0.0f;
	gameplayUiOneSprite_->SetColor({1.0f, 1.0f, 1.0f, alpha});
	gameplayUiTwoSprite_->SetColor({1.0f, 1.0f, 1.0f, alpha});
}

void GameScene::DrawGameplayTutorialUi() const {
	if (flowState_ != FlowState::kPlay || bossEncounterStarted_ || endType_ != EndType::kNone) { return; }
	Sprite::PreDraw();
	gameplayUiOneSprite_->Draw();
	gameplayUiTwoSprite_->Draw();
	Sprite::PostDraw();
}

void GameScene::StartBossEncounter() {
	bossEncounterStarted_ = true;
	cameraController_->LockToPosition(
	    {kBossArenaCameraX, kBossArenaCameraY, -15.0f}, kBossCameraEaseDuration);
	player_->SetLeftBoundary(kBossArenaPlayerMinX);
	bossArmature_->SetHorizontalBounds(kBossArenaBossMinX, kBossArenaBossMaxX);
}

void GameScene::Draw() {
	const Camera& camera = cameraController_->GetCamera();
	Model::PreDraw(Model::CullingMode::kNone);
	skydome_->Draw(camera);
	Model::PostDraw();
	DrawBackgroundSprites();
	// Tutorial text belongs to the world background. Drawing it before the map
	// and player lets the player visibly pass in front of it while walking.
	DrawGameplayTutorialUi();
	Model::PreDraw(Model::CullingMode::kNone);
	mapChipField_->Draw(camera);
	player_->Draw(camera);
	if (flowState_ == FlowState::kPlay) { bossArmature_->Draw(camera); }
	Model::PostDraw();
	DrawBossDefeatParticles(camera);
	if (flowState_ == FlowState::kPlay) { DrawBossRangeVisual(); }
	if (flowState_ == FlowState::kPlay) { bossArmature_->DrawDebug(camera); }
	if (flowState_ == FlowState::kPlay) { DrawCollisionDebug(camera); }
	if (flowState_ == FlowState::kPlay) { DrawHealthBars(); }
	if (flowState_ == FlowState::kPlay && dialogueSystem_ != nullptr) { dialogueSystem_->Draw(); }
	if (flowState_ == FlowState::kPlay && phaseDialogueSystem_ != nullptr) { phaseDialogueSystem_->Draw(); }
	if (flowState_ == FlowState::kPlay && victoryDialogueSystem_ != nullptr) { victoryDialogueSystem_->Draw(); }
	DrawTitleSequence();
	DrawEndOverlay();
}

GameScene::~GameScene() {
	for (Sprite*& rangeSprite : bossRangeSprites_) {
		delete rangeSprite;
		rangeSprite = nullptr;
	}
	delete whiteFlashOverlay_;
	delete blackOverlay_;
	delete bossHealthFill_;
	delete bossHealthBackground_;
	delete bossHealthFrame_;
	delete playerHealthFill_;
	delete playerHealthBackground_;
	delete playerHealthFrame_;
	delete moonSprite_;
	for (Sprite*& backgroundSprite : backgroundSprites_) {
		delete backgroundSprite;
		backgroundSprite = nullptr;
	}
	delete gameplayUiTwoSprite_;
	delete gameplayUiOneSprite_;
	delete titlePromptSprite_;
	delete titleLogo_;
	delete titleCoverSprite_;
	delete introSprite_;
	delete defeatParticleModel_;
	delete victoryDialogueSystem_;
	delete phaseDialogueSystem_;
	delete dialogueSystem_;
	delete bossArmature_;
	delete skydome_;
	delete cameraController_;
	delete player_;
	delete mapChipField_;
}
