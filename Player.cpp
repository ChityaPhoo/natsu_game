#include "Player.h"
#include "GamepadInput.h"
#include "Matrix4x4Calculation.h"
#include <algorithm>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

void Player::Initialize() {
	worldTransform_.Initialize();
	visualTransform_.Initialize();
	portraitTransform_.Initialize();
	portraitColor_.Initialize();
	portraitColor_.SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	attackEffectTransform_.Initialize();
	model_ = Model::CreateFromOBJ("player_body", true);
	attackEffectModel_ = Model::CreateFromOBJ("player_weapon", true);
	velocity_ = {};
	onGround_ = false;
	jumpsRemaining_ = 2;
	canAirDash_ = true;
	canAirAttack_ = true;
	actionState_ = ActionState::kNormal;
	dashPhase_ = DashPhase::kCharge;
	attackPhase_ = AttackPhase::kCharge;
	actionTimer_ = 0.0f;
	dashCooldownTimer_ = 0.0f;
	damageBlinkTimer_ = 0.0f;
	pullStartX_ = 0.0f;
	pullTargetX_ = 0.0f;
	pullStartY_ = 0.0f;
	pullLiftAmount_ = 0.0f;
	pullTimer_ = 0.0f;
	pullDuration_ = 0.0f;
	pullActive_ = false;
	idleAnimationTimer_ = 0.0f;
	isAttackEffectVisible_ = false;
	attackSwingStarted_ = false;
	movementCueStarted_ = false;
	currentDirection_ = LRDirection::kRight;
	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;
	attackEffectTransform_.scale_ = {kAttackEffectScale, kAttackEffectScale, kAttackEffectScale};
	UpdateWorldMatrix();
}

void Player::UpdateIdleAnimation() {
	idleAnimationTimer_ += kFrameTime;
	damageBlinkTimer_ = (std::max)(0.0f, damageBlinkTimer_ - kFrameTime);
	const float cycleDuration = (std::max)(kIdleCycleDuration, kFrameTime);
	if (idleAnimationTimer_ >= cycleDuration) { idleAnimationTimer_ = std::fmod(idleAnimationTimer_, cycleDuration); }
}

void Player::SetPosition(const Vector3& position) {
	worldTransform_.translation_ = position;
	UpdateWorldMatrix();
}

void Player::ResolveHorizontalPush(float positionX) {
	pullActive_ = false;
	worldTransform_.translation_.x = positionX;
	velocity_.x = 0.0f;
	UpdateWorldMatrix();
}

void Player::StartPullToward(float targetX, float maximumDistance, float duration, float liftAmount) {
	const float minimumCenterX = hasLeftBoundary_ ? (std::max)(kMapMinCenterX, leftBoundary_) : kMapMinCenterX;
	const float distance = std::clamp(targetX - worldTransform_.translation_.x, -std::abs(maximumDistance), std::abs(maximumDistance));
	pullStartX_ = worldTransform_.translation_.x;
	pullTargetX_ = std::clamp(pullStartX_ + distance, minimumCenterX, kMapMaxCenterX);
	pullStartY_ = worldTransform_.translation_.y;
	pullLiftAmount_ = (std::max)(liftAmount, 0.0f);
	pullTimer_ = 0.0f;
	pullDuration_ = (std::max)(duration, kFrameTime);
	pullActive_ = std::abs(pullTargetX_ - pullStartX_) > kCollisionEpsilon;
	velocity_.x = 0.0f;
}

void Player::NotifyDamage() { damageBlinkTimer_ = kDamageBlinkDuration; }

void Player::Update() {
	attackSwingStarted_ = false;
	movementCueStarted_ = false;
	Input* input = Input::GetInstance();
	const GamepadInput::Snapshot gamepad = GamepadInput::ReadPlayerOne();
	const bool right = input->PushKey(DIK_D) ||
	                   GamepadInput::IsHeld(gamepad, XINPUT_GAMEPAD_DPAD_RIGHT);
	const bool left = input->PushKey(DIK_A) ||
	                  GamepadInput::IsHeld(gamepad, XINPUT_GAMEPAD_DPAD_LEFT);
	dashCooldownTimer_ = (std::max)(0.0f, dashCooldownTimer_ - kFrameTime);

	if (actionState_ == ActionState::kNormal) {
		const bool attackTriggered = input->IsTriggerMouse(0) ||
		                             GamepadInput::IsTriggered(gamepad, XINPUT_GAMEPAD_X);
		const bool dashTriggered = input->IsTriggerMouse(1) ||
		                           GamepadInput::IsTriggered(gamepad, XINPUT_GAMEPAD_B);
		if (attackTriggered && (onGround_ || canAirAttack_)) {
			actionState_ = ActionState::kAttack;
			attackPhase_ = AttackPhase::kCharge;
			actionTimer_ = 0.0f;
			velocity_.x = 0.0f;
			isAttackEffectVisible_ = true;
			if (!onGround_) { canAirAttack_ = false; }
		} else if (dashTriggered && dashCooldownTimer_ <= 0.0f && (onGround_ || canAirDash_)) {
			actionState_ = ActionState::kDash;
			movementCueStarted_ = true;
			dashPhase_ = DashPhase::kCharge;
			actionTimer_ = 0.0f;
			dashCooldownTimer_ = kDashCooldown;
			dashDirection_ = right != left ? (right ? 1.0f : -1.0f) : (currentDirection_ == LRDirection::kRight ? 1.0f : -1.0f);
			const LRDirection dashFacing = dashDirection_ > 0.0f ? LRDirection::kRight : LRDirection::kLeft;
			if (currentDirection_ != dashFacing) { BeginTurn(dashFacing); }
			velocity_.x = 0.0f;
			if (!onGround_) { canAirDash_ = false; }
		}
	}

	Vector3 plannedMove = velocity_;
	if (actionState_ == ActionState::kNormal) {
		worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
		isAttackEffectVisible_ = false;
		if (right != left) {
			const LRDirection direction = right ? LRDirection::kRight : LRDirection::kLeft;
			if ((right && velocity_.x < 0.0f) || (left && velocity_.x > 0.0f)) { velocity_.x *= 1.0f - kAttenuation; }
			velocity_.x = std::clamp(velocity_.x + (right ? kAcceleration : -kAcceleration), -kMaxSpeed, kMaxSpeed);
			if (currentDirection_ != direction) { BeginTurn(direction); }
		} else {
			velocity_.x *= 1.0f - kAttenuation;
			if (std::abs(velocity_.x) < 0.01f) { velocity_.x = 0.0f; }
		}

		const bool jumpTriggered = input->TriggerKey(DIK_SPACE) ||
		                           GamepadInput::IsTriggered(gamepad, XINPUT_GAMEPAD_A);
		if (jumpTriggered && jumpsRemaining_ > 0) {
			velocity_.y = kJumpAcceleration;
			movementCueStarted_ = true;
			onGround_ = false;
			--jumpsRemaining_;
		} else if (!onGround_) {
			velocity_.y = (std::max)(velocity_.y - kGravityAcceleration, -kMaxFallSpeed);
		}
		plannedMove = velocity_;
	} else if (actionState_ == ActionState::kDash) {
		actionTimer_ += kFrameTime;
		const float chargeEnd = kDashChargeTime;
		const float burstEnd = chargeEnd + kDashBurstTime;
		const float recoveryEnd = burstEnd + kDashRecoveryTime;
		plannedMove.x = 0.0f;
		velocity_.x = 0.0f;
		if (actionTimer_ < chargeEnd) {
			dashPhase_ = DashPhase::kCharge;
			const float t = actionTimer_ / kDashChargeTime;
			worldTransform_.scale_.z = EaseOut(1.0f, 0.3f, t);
			worldTransform_.scale_.y = EaseOut(1.0f, 1.6f, t);
		} else if (actionTimer_ < burstEnd) {
			dashPhase_ = DashPhase::kBurst;
			const float t = (actionTimer_ - chargeEnd) / kDashBurstTime;
			worldTransform_.scale_.z = EaseOut(0.3f, 1.3f, t);
			worldTransform_.scale_.y = EaseOut(1.6f, 0.7f, t);
			plannedMove.x = dashDirection_ * EaseOut(kDashBurstSpeed, kDashBurstEndSpeed, t);
		} else if (actionTimer_ < recoveryEnd) {
			dashPhase_ = DashPhase::kRecovery;
			const float t = (actionTimer_ - burstEnd) / kDashRecoveryTime;
			worldTransform_.scale_.z = EaseOut(1.3f, 1.0f, t);
			worldTransform_.scale_.y = EaseOut(0.7f, 1.0f, t);
		} else {
			actionState_ = ActionState::kNormal;
			worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
			velocity_.x = dashDirection_ * kMaxSpeed;
		}
		if (!onGround_) { velocity_.y = (std::max)(velocity_.y - kGravityAcceleration, -kMaxFallSpeed); }
		plannedMove.y = velocity_.y;
		isAttackEffectVisible_ = false;
	} else {
		const AttackPhase previousAttackPhase = attackPhase_;
		actionTimer_ += kFrameTime;
		const float chargeEnd = kAttackChargeTime;
		const float strikeEnd = chargeEnd + kAttackStrikeTime;
		const float recoveryEnd = strikeEnd + kAttackRecoveryTime;
		plannedMove.x = 0.0f;
		velocity_.x = 0.0f;
		if (actionTimer_ < chargeEnd) {
			attackPhase_ = AttackPhase::kCharge;
			const float t = actionTimer_ / kAttackChargeTime;
			worldTransform_.scale_.z = EaseOut(1.0f, 0.3f, t);
			worldTransform_.scale_.y = EaseOut(1.0f, 1.6f, t);
			isAttackEffectVisible_ = true;
		} else if (actionTimer_ < strikeEnd) {
			attackPhase_ = AttackPhase::kStrike;
			attackSwingStarted_ = previousAttackPhase != AttackPhase::kStrike;
			const float t = (actionTimer_ - chargeEnd) / kAttackStrikeTime;
			worldTransform_.scale_.z = EaseOut(0.3f, 1.3f, t);
			worldTransform_.scale_.y = EaseOut(1.6f, 0.7f, t);
			isAttackEffectVisible_ = true;
		} else if (actionTimer_ < recoveryEnd) {
			attackPhase_ = AttackPhase::kRecovery;
			const float t = (actionTimer_ - strikeEnd) / kAttackRecoveryTime;
			worldTransform_.scale_.z = EaseOut(1.3f, 1.0f, t);
			worldTransform_.scale_.y = EaseOut(0.7f, 1.0f, t);
			isAttackEffectVisible_ = true;
		} else {
			actionState_ = ActionState::kNormal;
			worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
			isAttackEffectVisible_ = false;
		}
		if (!onGround_) { velocity_.y = (std::max)(velocity_.y - kGravityAcceleration, -kMaxFallSpeed); }
		plannedMove.y = velocity_.y;
	}

	CollisionInfo collisionInfo = {};
	collisionInfo.move = plannedMove;
	ResolveHorizontalCollision(collisionInfo);
	ResolveVerticalCollision(collisionInfo);
	const float minimumCenterX = hasLeftBoundary_ ? (std::max)(kMapMinCenterX, leftBoundary_) : kMapMinCenterX;
	const float proposedCenterX = worldTransform_.translation_.x + collisionInfo.move.x;
	const float clampedCenterX = std::clamp(proposedCenterX, minimumCenterX, kMapMaxCenterX);
	if (clampedCenterX != proposedCenterX) {
		collisionInfo.move.x = clampedCenterX - worldTransform_.translation_.x;
		collisionInfo.hitWall = true;
	}
	worldTransform_.translation_.x += collisionInfo.move.x;
	worldTransform_.translation_.y += collisionInfo.move.y;

	if (collisionInfo.hitWall) { velocity_.x *= 1.0f - kAttenuation; }
	if (collisionInfo.hitCeiling) { velocity_.y = 0.0f; }
	if (collisionInfo.hitGround) {
		velocity_.y = 0.0f;
		onGround_ = true;
		jumpsRemaining_ = 2;
		canAirDash_ = true;
		canAirAttack_ = true;
	} else if (velocity_.y <= 0.0f) {
		onGround_ = false;
	}
	UpdatePullMotion();

	UpdateRotation();
	UpdateWorldMatrix();
	UpdateAttackEffectTransform();
}

void Player::ResolveHorizontalCollision(CollisionInfo& collisionInfo) const {
	if (mapChipField_ == nullptr || collisionInfo.move.x == 0.0f) { return; }
	const Vector3& center = worldTransform_.translation_;
	const float playerBottom = center.y - kCollisionHalfHeight + kCollisionEpsilon;
	const float playerTop = center.y + kCollisionHalfHeight - kCollisionEpsilon;

	for (uint32_t y = 0; y < mapChipField_->GetMapChipCountVertical(); ++y) {
		for (uint32_t x = 0; x < mapChipField_->GetMapChipCountHorizontal(); ++x) {
			if (mapChipField_->GetMapChipTypeFromIndex(x, y) != MapChipField::MapChipType::kBlock) { continue; }
			const MapChipField::Rect rect = mapChipField_->GetRectFromIndex(x, y);
			if (playerTop <= rect.bottom || playerBottom >= rect.top) { continue; }

			if (collisionInfo.move.x > 0.0f) {
				const float currentRight = center.x + kCollisionHalfWidth;
				const float movedRight = currentRight + collisionInfo.move.x;
				if (currentRight <= rect.left + kCollisionEpsilon && movedRight >= rect.left) {
					collisionInfo.move.x = (std::min)(collisionInfo.move.x, rect.left - currentRight - kCollisionEpsilon);
					collisionInfo.hitWall = true;
				}
			} else {
				const float currentLeft = center.x - kCollisionHalfWidth;
				const float movedLeft = currentLeft + collisionInfo.move.x;
				if (currentLeft >= rect.right - kCollisionEpsilon && movedLeft <= rect.right) {
					collisionInfo.move.x = (std::max)(collisionInfo.move.x, rect.right - currentLeft + kCollisionEpsilon);
					collisionInfo.hitWall = true;
				}
			}
		}
	}
}

void Player::ResolveVerticalCollision(CollisionInfo& collisionInfo) const {
	if (mapChipField_ == nullptr) { return; }
	const Vector3& center = worldTransform_.translation_;
	const float movedCenterX = center.x + collisionInfo.move.x;
	const float playerLeft = movedCenterX - kCollisionHalfWidth + kCollisionEpsilon;
	const float playerRight = movedCenterX + kCollisionHalfWidth - kCollisionEpsilon;
	const float probeMoveY = collisionInfo.move.y == 0.0f ? -kCollisionEpsilon : collisionInfo.move.y;

	for (uint32_t y = 0; y < mapChipField_->GetMapChipCountVertical(); ++y) {
		for (uint32_t x = 0; x < mapChipField_->GetMapChipCountHorizontal(); ++x) {
			if (mapChipField_->GetMapChipTypeFromIndex(x, y) != MapChipField::MapChipType::kBlock) { continue; }
			const MapChipField::Rect rect = mapChipField_->GetRectFromIndex(x, y);
			if (playerRight <= rect.left || playerLeft >= rect.right) { continue; }

			if (probeMoveY > 0.0f) {
				const float currentTop = center.y + kCollisionHalfHeight;
				const float movedTop = currentTop + probeMoveY;
				if (currentTop <= rect.bottom + kCollisionEpsilon && movedTop >= rect.bottom) {
					collisionInfo.move.y = (std::min)(collisionInfo.move.y, rect.bottom - currentTop - kCollisionEpsilon);
					collisionInfo.hitCeiling = true;
				}
			} else {
				const float currentBottom = center.y - kCollisionHalfHeight;
				const float movedBottom = currentBottom + probeMoveY;
				if (currentBottom >= rect.top - kCollisionEpsilon && movedBottom <= rect.top) {
					if (collisionInfo.move.y < 0.0f) {
						collisionInfo.move.y = (std::max)(collisionInfo.move.y, rect.top - currentBottom + kCollisionEpsilon);
					}
					collisionInfo.hitGround = true;
				}
			}
		}
	}
}

void Player::BeginTurn(LRDirection direction) {
	currentDirection_ = direction;
	turnStartRotationY_ = worldTransform_.rotation_.y;
	turnTimer_ = kTurnDuration;
}

void Player::UpdateRotation() {
	const float targets[] = {std::numbers::pi_v<float> / 2.0f, std::numbers::pi_v<float> * 3.0f / 2.0f};
	const float target = targets[static_cast<uint32_t>(currentDirection_)];
	if (turnTimer_ <= 0.0f) { worldTransform_.rotation_.y = target; return; }
	turnTimer_ = (std::max)(0.0f, turnTimer_ - 1.0f / 60.0f);
	float t = 1.0f - turnTimer_ / kTurnDuration;
	t = t * t * (3.0f - 2.0f * t);
	worldTransform_.rotation_.y = std::lerp(turnStartRotationY_, target, t);
}

void Player::UpdateWorldMatrix() {
	worldTransform_.matWorld_ = Matrix4x4Calculation::MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void Player::UpdateAttackEffectTransform() {
	if (!isAttackEffectVisible_) { return; }

	const float facing = currentDirection_ == LRDirection::kRight ? 1.0f : -1.0f;
	const float offsetX = facing * kAttackEffectOffsetX;
	attackEffectTransform_.translation_ = worldTransform_.translation_;
	attackEffectTransform_.translation_.x += offsetX;
	attackEffectTransform_.translation_.y += kAttackEffectOffsetY;

	// The sword is authored upright with its grip at the model origin. Swing it
	// around that grip: a short wind-up, a quick cutting arc, then a small settle.
	// Reversing the Z angle mirrors the same motion when the player faces left.
	const float chargeEnd = kAttackChargeTime;
	const float strikeEnd = chargeEnd + kAttackStrikeTime;
	float swingAngle = kSwordRestAngle;
	if (actionTimer_ < chargeEnd) {
		const float t = SmoothStep(actionTimer_ / kAttackChargeTime);
		swingAngle = std::lerp(kSwordRestAngle, kSwordWindUpAngle, t);
	} else if (actionTimer_ < strikeEnd) {
		const float t = SmoothStep((actionTimer_ - chargeEnd) / kAttackStrikeTime);
		swingAngle = std::lerp(kSwordWindUpAngle, kSwordStrikeAngle, t);
	} else {
		const float t = SmoothStep((actionTimer_ - strikeEnd) / kAttackRecoveryTime);
		swingAngle = std::lerp(kSwordStrikeAngle, kSwordRecoveryAngle, t);
	}
	attackEffectTransform_.rotation_ = {0.0f, 0.0f, facing * swingAngle};
	attackEffectTransform_.scale_ = {kAttackEffectScale, kAttackEffectScale, kAttackEffectScale};
	attackEffectTransform_.matWorld_ = Matrix4x4Calculation::MakeAffineMatrix(
	    attackEffectTransform_.scale_, attackEffectTransform_.rotation_, attackEffectTransform_.translation_);
	attackEffectTransform_.TransferMatrix();
}

bool Player::IsAttackActive() const {
	return actionState_ == ActionState::kAttack && attackPhase_ == AttackPhase::kStrike;
}

bool Player::ConsumeAttackSwingStarted() {
	const bool started = attackSwingStarted_;
	attackSwingStarted_ = false;
	return started;
}

bool Player::ConsumeMovementCueStarted() {
	const bool started = movementCueStarted_;
	movementCueStarted_ = false;
	return started;
}

bool Player::IsDashInvincible() const {
	return actionState_ == ActionState::kDash && actionTimer_ <= kDashInvincibilityDuration;
}

void Player::UpdatePullMotion() {
	if (!pullActive_) { return; }
	pullTimer_ = (std::min)(pullTimer_ + kFrameTime, pullDuration_);
	const float t = pullTimer_ / pullDuration_;
	const float easedT = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
	worldTransform_.translation_.x = std::lerp(pullStartX_, pullTargetX_, easedT);
	worldTransform_.translation_.y = pullStartY_ + std::sin(t * std::numbers::pi_v<float>) * pullLiftAmount_;
	velocity_.x = 0.0f;
	if (pullTimer_ >= pullDuration_) { pullActive_ = false; }
}

Player::AttackHitbox Player::GetBodyHitbox() const {
	const Vector3& center = worldTransform_.translation_;
	return {
	    {center.x - kCollisionHalfWidth, center.y - kCollisionHalfHeight, center.z - kCollisionHalfWidth},
	    {center.x + kCollisionHalfWidth, center.y + kCollisionHalfHeight, center.z + kCollisionHalfWidth}};
}

Player::AttackHitbox Player::GetAttackHitbox() const {
	Vector3 center = worldTransform_.translation_;
	center.x += currentDirection_ == LRDirection::kRight ? kAttackCollisionOffsetX : -kAttackCollisionOffsetX;
	center.y += kAttackCollisionOffsetY;
	return {
	    {center.x - kAttackCollisionHalfWidth, center.y - kAttackCollisionHalfHeight, center.z - kAttackCollisionHalfWidth},
	    {center.x + kAttackCollisionHalfWidth, center.y + kAttackCollisionHalfHeight, center.z + kAttackCollisionHalfWidth}};
}

float Player::EaseOut(float start, float end, float t) {
	t = std::clamp(t, 0.0f, 1.0f);
	const float easedT = 1.0f - (1.0f - t) * (1.0f - t);
	return std::lerp(start, end, easedT);
}

float Player::SmoothStep(float t) {
	t = std::clamp(t, 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}

void Player::Draw(const Camera& camera) {
	if (model_ == nullptr) { return; }
	visualTransform_.translation_ = worldTransform_.translation_;
	visualTransform_.rotation_ = worldTransform_.rotation_;
	visualTransform_.scale_ = worldTransform_.scale_;
	const bool isIdle =
	    actionState_ == ActionState::kNormal &&
	    std::abs(velocity_.x) <= kIdleMaximumMovementSpeed &&
	    std::abs(velocity_.y) <= kIdleMaximumMovementSpeed;
	if (isIdle) {
		const float cycleDuration = (std::max)(kIdleCycleDuration, kFrameTime);
		const float breath = std::sin(idleAnimationTimer_ / cycleDuration * 2.0f * std::numbers::pi_v<float>);
		visualTransform_.translation_.y += breath * kIdleMoveAmount;
		visualTransform_.scale_.x *= 1.0f - breath * kIdleHorizontalScaleAmount;
		visualTransform_.scale_.y *= 1.0f + breath * kIdleScaleAmount;
		visualTransform_.scale_.z *= 1.0f - breath * kIdleHorizontalScaleAmount;
	}
	visualTransform_.translation_.y += kVisualOffsetY;
	visualTransform_.matWorld_ = Matrix4x4Calculation::MakeAffineMatrix(
	    visualTransform_.scale_, visualTransform_.rotation_, visualTransform_.translation_);
	visualTransform_.TransferMatrix();
	const bool blinkHidden = damageBlinkTimer_ > 0.0f &&
	                         static_cast<int>(damageBlinkTimer_ / kDamageBlinkInterval) % 2 == 0;
	if (!blinkHidden) { model_->Draw(visualTransform_, camera); }

	if (isAttackEffectVisible_ && attackEffectModel_ != nullptr) {
		attackEffectModel_->Draw(attackEffectTransform_, camera);
	}
}

void Player::DrawPortrait(
    const Camera& camera, const Vector3& rotationOffset,
    const Vector3& scaleMultiplier) {
	if (model_ == nullptr) { return; }
	portraitTransform_.translation_ = worldTransform_.translation_;
	// Portraits always face left like the boss portrait. Gameplay turning is not
	// inherited here, so the HP icon cannot unexpectedly flip or expose the blank
	// back of the helmet as the player moves.
	portraitTransform_.rotation_ = {
	    rotationOffset.x,
	    -std::numbers::pi_v<float> * 0.5f + rotationOffset.y,
	    rotationOffset.z};
	portraitTransform_.scale_ = {
	    worldTransform_.scale_.x * scaleMultiplier.x,
	    worldTransform_.scale_.y * scaleMultiplier.y,
	    worldTransform_.scale_.z * scaleMultiplier.z};
	portraitTransform_.matWorld_ = Matrix4x4Calculation::MakeAffineMatrix(
	    portraitTransform_.scale_, portraitTransform_.rotation_, portraitTransform_.translation_);
	portraitTransform_.TransferMatrix();
	model_->Draw(portraitTransform_, camera, &portraitColor_);
}

void Player::SetPortraitOpacity(float opacity) {
	opacity = std::clamp(opacity, 0.0f, 1.0f);
	portraitColor_.SetColor({1.0f, 1.0f, 1.0f, opacity});
}

void Player::DrawDashCooldownMeter(const Camera& camera) const {
	if (dashCooldownTimer_ <= 0.0f) { return; }
	const float remainingRatio = std::clamp(dashCooldownTimer_ / kDashCooldown, 0.0f, 1.0f);
	const float left = worldTransform_.translation_.x - kDashMeterWidth * 0.5f;
	const float right = worldTransform_.translation_.x + kDashMeterWidth * 0.5f;
	const float fillRight = left + kDashMeterWidth * remainingRatio;
	const float y = worldTransform_.translation_.y + kDashMeterHeightOffset;
	const float z = worldTransform_.translation_.z - 0.25f;
	PrimitiveDrawer* drawer = PrimitiveDrawer::GetInstance();
	drawer->SetCamera(&camera);
	for (float offset : {-0.025f, 0.0f, 0.025f}) {
		drawer->DrawLine3d({left, y + offset, z}, {right, y + offset, z}, {0.03f, 0.03f, 0.03f, 0.75f});
		if (fillRight > left) {
			drawer->DrawLine3d({left, y + offset, z - 0.01f}, {fillRight, y + offset, z - 0.01f}, {1.0f, 1.0f, 1.0f, 1.0f});
		}
	}
}

Player::~Player() {
	delete attackEffectModel_;
	attackEffectModel_ = nullptr;
	delete model_;
	model_ = nullptr;
}
