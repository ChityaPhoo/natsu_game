#pragma once

#include "KamataEngine.h"
#include <array>
#include <cstddef>
#include <cstdint>

class BossArmature {
public:
	struct CollisionBox {
		KamataEngine::Vector3 min = {};
		KamataEngine::Vector3 max = {};
	};

	~BossArmature();
	void Initialize();
	void Update(const KamataEngine::Vector3& playerPosition);
	void SetAIEnabled(bool enabled);
	void SetHorizontalBounds(float minX, float maxX);
	void SetVisible(bool visible) { isVisible_ = visible; }
	void SetDefeatBrightness(float brightness);
	CollisionBox GetBodyHitbox() const;
	CollisionBox GetScytheHitbox() const;
	bool IsScytheAttackActive() const;
	KamataEngine::Vector3 GetPosition() const { return joints_[kRoot].worldPosition; }
	float GetCloseDistance() const { return closeDistance_; }
	float GetMidDistance() const { return midDistance_; }
	void Draw(const KamataEngine::Camera& camera);
	void DrawDebug(const KamataEngine::Camera& camera);
#ifdef USE_IMGUI
	void DrawImGui();
#endif

private:
	enum JointIndex : uint32_t {
		kRoot,
		kBody,
		kChest,
		kNeck,
		kHead,
		kLeftShoulder,
		kLeftElbow,
		kLeftHand,
		kRightShoulder,
		kRightElbow,
		kRightHand,
		kJointCount,
	};

	enum class AnimationType {
		kNone,
		kNormalAttack,
		kScytheThrow,
		kSpinAttack,
		kVerticalHook,
	};

	enum class ControlMode {
		kAnimationDebug,
		kPlayTest,
	};

	enum class AIState {
		kWaiting,
		kMeleeAttack,
		kRetreat,
		kSpinAttack,
		kVerticalHook,
		kScytheThrow,
	};

	struct Joint {
		const char* name = "";
		int32_t parentIndex = -1;
		KamataEngine::Vector3 scale = {1.0f, 1.0f, 1.0f};
		KamataEngine::Vector3 rotation = {};
		KamataEngine::Vector3 translation = {};
		KamataEngine::Matrix4x4 localMatrix = {};
		KamataEngine::Matrix4x4 worldMatrix = {};
		KamataEngine::Vector3 worldPosition = {};
		KamataEngine::WorldTransform markerTransform;
		KamataEngine::ObjectColor markerColor;
	};

	struct ModelPart {
		KamataEngine::Model* model = nullptr;
		KamataEngine::WorldTransform worldTransform;
		JointIndex joint = kRoot;
		JointIndex segmentEndJoint = kRoot;
		KamataEngine::Vector3 bindJointPosition = {};
		bool followsJointSegment = false;
	};

	struct AttackPose {
		std::array<KamataEngine::Vector3, kJointCount> translationOffsets = {};
		std::array<KamataEngine::Vector3, kJointCount> rotationOffsets = {};
	};

	struct AttackKeyframe {
		float time = 0.0f;
		AttackPose pose = {};
	};

	void InitializeJoint(JointIndex index, const char* name, int32_t parentIndex, const KamataEngine::Vector3& translation, const KamataEngine::Vector4& color);
	void InitializeModelPart(ModelPart& part, const char* modelName, JointIndex joint, const KamataEngine::Vector3& bindJointPosition);
	void InitializeArmSegment(ModelPart& part, const char* modelName, JointIndex startJoint, JointIndex endJoint);
	void UpdateModelPart(ModelPart& part);
	void ResetPose();
	void InitializeNormalAttackClip();
	void InitializeScytheThrowClip();
	void InitializeSpinAttackClip();
	void InitializeVerticalHookClip();
	void StartAnimation(AnimationType animation);
	void StopAnimation();
	void UpdateAnimation();
	void UpdateScytheState();
	void SetControlMode(ControlMode mode);
	void UpdateAI();
	void PickNextAIAction();
	void EnterAIState(AIState state);
	void FacePlayer();
	void MoveBossX(float movement);
	void ResetBossPosition();
	void ApplyAttackPose(const AttackPose& start, const AttackPose& end, float t);
	void DrawDebugScythe(const KamataEngine::Camera& camera);
	float GetActiveAnimationDuration() const;
	float GetActivePlaybackDuration() const;
	float GetActivePlaybackSpeed() const;
	const char* GetActiveAnimationName() const;
	const char* GetAIStateName() const;
	const char* GetDistanceBandName() const;
	int NextRandomPercent();
	static KamataEngine::Vector3 Lerp(const KamataEngine::Vector3& start, const KamataEngine::Vector3& end, float t);
	static float SmoothStep(float t);

	std::array<Joint, kJointCount> joints_;
	std::array<KamataEngine::Vector3, kJointCount> defaultTranslations_ = {};
	std::array<KamataEngine::Vector3, kJointCount> defaultRotations_ = {};
	std::array<KamataEngine::Vector3, kJointCount> animationBaseTranslations_ = {};
	std::array<KamataEngine::Vector3, kJointCount> animationBaseRotations_ = {};
	std::array<AttackKeyframe, 6> normalAttackKeyframes_ = {};
	std::array<AttackKeyframe, 8> scytheThrowKeyframes_ = {};
	std::array<AttackKeyframe, 8> spinAttackKeyframes_ = {};
	std::array<AttackKeyframe, 6> verticalHookKeyframes_ = {};
	std::array<ModelPart, 6> modelParts_;
	KamataEngine::Model* jointSphereModel_ = nullptr;
	KamataEngine::ObjectColor defeatColor_;
	bool showBossModel_ = true;
	bool isVisible_ = true;
	bool showDebugArmature_ = false;
	bool showDebugScythe_ = true;
	bool isScytheDetached_ = false;
	bool useExplicitScythePose_ = false;
	bool hasScytheReleaseCenter_ = false;
	bool loopAnimation_ = false;
	bool pauseAnimation_ = false;
	bool aiEnabled_ = false;
	AnimationType activeAnimation_ = AnimationType::kNone;
	ControlMode controlMode_ = ControlMode::kPlayTest;
	AIState aiState_ = AIState::kWaiting;
	KamataEngine::Vector3 playerTargetPosition_ = {};
	KamataEngine::Vector3 actionTargetPosition_ = {};
	KamataEngine::Vector3 scytheReleaseCenter_ = {};
	KamataEngine::Vector3 scytheTargetCenter_ = {};
	KamataEngine::Vector3 scytheFlightDirection_ = {-1.0f, 0.0f, 0.0f};
	KamataEngine::Vector3 explicitScytheCenter_ = {};
	float explicitScytheRotation_ = 0.0f;
	float facingDirection_ = -1.0f;
	float playerDistance_ = 0.0f;
	float aiWaitTimer_ = 0.0f;
	float retreatTimer_ = 0.0f;
	int lastAIRoll_ = -1;
	uint32_t randomState_ = 0x4D595DF4u;
	float jointRadius_ = 0.10f;
	// Damage-hitbox defaults. They can also be edited live in the Boss
	// Armature ImGui window under "Damage Hitboxes".
	float bodyHitboxHalfWidth_ = 2.15f;
	float bodyHitboxBottomOffset_ = -0.15f;
	float bodyHitboxTopPadding_ = 1.05f;
	float bodyHitboxHalfDepth_ = 1.00f;
	KamataEngine::Vector3 scytheHitboxPadding_ = {0.30f, 0.30f, 0.30f};
	float normalAttackPlaybackSpeed_ = 1.8f;
	float normalAttackPlaybackDuration_ = 2.20f;
	float scytheThrowPlaybackSpeed_ = 1.0f;
	float scytheThrowPlaybackDuration_ = 2.45f;
	float scytheThrowRange_ = 14.0f;
	float scytheThrowArcHeight_ = 0.75f;
	float scytheThrowTargetYOffset_ = 1.80f;
	float scytheThrowSpinCount_ = 4.0f;
	float spinAttackPlaybackSpeed_ = 3.0f;
	float spinAttackPlaybackDuration_ = 5.00f;
	int spinAttackTurnCount_ = 3;
	float verticalHookPlaybackSpeed_ = 1.0f;
	float verticalHookPlaybackDuration_ = 1.40f;
	float verticalHookReach_ = 9.0f;
	float verticalHookTargetYOffset_ = 1.80f;
	float closeDistance_ = 4.5f;
	float midDistance_ = 10.0f;
	float aiDecisionDelay_ = 0.65f;
	float retreatSpeed_ = 2.0f;
	float retreatDuration_ = 0.90f;
	float spinDashSpeed_ = 2.2f;
	float spinDashStopDistance_ = 1.8f;
	float movementMinX_ = 3.0f;
	float movementMaxX_ = 57.0f;
	int closeMeleeChance_ = 65;
	int midSpinChance_ = 55;
	int farThrowChance_ = 100;
	float animationTime_ = 0.0f;
	static inline const float kFrameTime = 1.0f / 60.0f;
	static inline const float kInitialBossX = 56.0f;
	static inline const float kNormalAttackDuration = 1.35f;
	static inline const float kScytheThrowDuration = 2.45f;
	static inline const float kSpinAttackDuration = 1.95f;
	static inline const float kVerticalHookDuration = 1.40f;
	static inline const float kScytheReleaseTime = 0.62f;
	static inline const float kScytheCatchTime = 1.88f;
};
