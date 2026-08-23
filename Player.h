#pragma once
#include "KamataEngine.h"
#include "MapChipField.h"
#include <cstdint>

class Player {
public:
	struct AttackHitbox {
		KamataEngine::Vector3 min = {};
		KamataEngine::Vector3 max = {};
	};

	~Player();
	void Initialize();
	void SetPosition(const KamataEngine::Vector3& position);
	void ResolveHorizontalPush(float positionX);
	void StartPullToward(float targetX, float maximumDistance, float duration);
	void NotifyDamage();
	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }
	void SetLeftBoundary(float boundary) { leftBoundary_ = boundary; hasLeftBoundary_ = true; }
	void ClearLeftBoundary() { hasLeftBoundary_ = false; }
	void UpdateIdleAnimation();
	void Update();
	void Draw(const KamataEngine::Camera& camera);
	const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }
	const KamataEngine::Vector3& GetVelocity() const { return velocity_; }
	bool IsAttackActive() const;
	bool IsDashInvincible() const;
	bool IsFacingRight() const { return currentDirection_ == LRDirection::kRight; }
	AttackHitbox GetBodyHitbox() const;
	AttackHitbox GetAttackHitbox() const;

private:
	struct CollisionInfo {
		KamataEngine::Vector3 move = {};
		bool hitCeiling = false;
		bool hitWall = false;
		bool hitGround = false;
	};
	enum class LRDirection : uint32_t { kRight, kLeft };
	enum class ActionState : uint32_t { kNormal, kDash, kAttack };
	enum class DashPhase : uint32_t { kCharge, kBurst, kRecovery };
	enum class AttackPhase : uint32_t { kCharge, kStrike, kRecovery };

	void ResolveHorizontalCollision(CollisionInfo& collisionInfo) const;
	void ResolveVerticalCollision(CollisionInfo& collisionInfo) const;
	void BeginTurn(LRDirection direction);
	void UpdateRotation();
	void UpdateWorldMatrix();
	void UpdateAttackEffectTransform();
	void UpdatePullMotion();
	static float EaseOut(float start, float end, float t);

	static inline const float kAcceleration = 0.04f;
	static inline const float kMaxSpeed = 0.30f;
	static inline const float kAttenuation = 0.20f;
	static inline const float kJumpAcceleration = 0.45f;
	static inline const float kGravityAcceleration = 0.03f;
	static inline const float kMaxFallSpeed = 1.00f;
	static inline const float kCollisionHalfWidth = 0.45f;
	static inline const float kCollisionHalfHeight = 0.40f;
	static inline const float kCollisionEpsilon = 0.001f;
	static inline const float kTurnDuration = 0.15f;
	// The model is already authored around the gameplay origin. A negative
	// render offset pushes it inside the two-row floor and hides it by depth.
	static inline const float kVisualOffsetY = 0.0f;
	// Player breathing/idle animation tuning. This only affects drawing, so the
	// collision box and movement position remain unchanged.
	static inline const float kIdleMoveAmount = 0.18f;
	static inline const float kIdleScaleAmount = 0.14f;
	static inline const float kIdleHorizontalScaleAmount = 0.075f;
	static inline const float kIdleCycleDuration = 1.65f;
	static inline const float kIdleMaximumMovementSpeed = 0.05f;
	static inline const float kMapMinCenterX = 0.45f;
	static inline const float kMapMaxCenterX = 99.55f;
	static inline const float kFrameTime = 1.0f / 60.0f;
	static inline const float kDashChargeTime = 0.10f;
	static inline const float kDashBurstTime = 0.12f;
	static inline const float kDashRecoveryTime = 0.18f;
	static inline const float kDashBurstSpeed = 0.70f;
	static inline const float kDashBurstEndSpeed = 0.38f;
	static inline const float kDashCooldown = 0.85f;
	static inline const float kDashInvincibilityDuration = kDashChargeTime + kDashBurstTime;
	static inline const float kDamageBlinkDuration = 0.75f;
	static inline const float kDamageBlinkInterval = 0.075f;
	static inline const float kAttackChargeTime = 0.10f;
	static inline const float kAttackStrikeTime = 0.12f;
	static inline const float kAttackRecoveryTime = 0.18f;
	static inline const float kAttackEffectOffsetX = 1.10f;
	static inline const float kAttackEffectOffsetY = 0.0f;
	static inline const float kAttackEffectScale = 1.0f;
	static inline const float kAttackCollisionOffsetX = 1.10f;
	static inline const float kAttackCollisionOffsetY = 0.0f;
	static inline const float kAttackCollisionHalfWidth = 0.65f;
	static inline const float kAttackCollisionHalfHeight = 0.45f;

	KamataEngine::WorldTransform worldTransform_;
	// Uses a separate constant buffer from the gameplay/collision transform so
	// restoring the logical pose cannot erase the breathing pose before drawing.
	KamataEngine::WorldTransform visualTransform_;
	KamataEngine::WorldTransform attackEffectTransform_;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Model* attackEffectModel_ = nullptr;
	MapChipField* mapChipField_ = nullptr;
	KamataEngine::Vector3 velocity_ = {};
	LRDirection currentDirection_ = LRDirection::kRight;
	ActionState actionState_ = ActionState::kNormal;
	DashPhase dashPhase_ = DashPhase::kCharge;
	AttackPhase attackPhase_ = AttackPhase::kCharge;
	bool onGround_ = false;
	bool canAirDash_ = true;
	bool canAirAttack_ = true;
	bool isAttackEffectVisible_ = false;
	float turnStartRotationY_ = 0.0f;
	float turnTimer_ = 0.0f;
	float actionTimer_ = 0.0f;
	float dashCooldownTimer_ = 0.0f;
	float damageBlinkTimer_ = 0.0f;
	float pullStartX_ = 0.0f;
	float pullTargetX_ = 0.0f;
	float pullTimer_ = 0.0f;
	float pullDuration_ = 0.0f;
	float idleAnimationTimer_ = 0.0f;
	float dashDirection_ = 1.0f;
	float leftBoundary_ = 0.0f;
	bool hasLeftBoundary_ = false;
	bool pullActive_ = false;
};
