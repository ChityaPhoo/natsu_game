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
	void SetPhaseTwo(bool enabled) { isPhaseTwo_ = enabled; }
	void StartPhaseTwoAI();
	void SetHorizontalBounds(float minX, float maxX);
	void SetVisible(bool visible) { isVisible_ = visible; }
	void SetDefeatBrightness(float brightness);
	void SetModelOpacity(float opacity);
	void BeginPhaseTransition();
	void SetPhaseTransitionProgress(float progress);
	void EndPhaseTransition();
	CollisionBox GetBodyHitbox() const;
	CollisionBox GetScytheHitbox() const;
	bool IsScytheAttackActive() const;
	bool IsScytheThrowInProgress() const { return activeAnimation_ == AnimationType::kScytheThrow; }
	bool IsBodyAttackActive() const;
	bool IsVerticalHookAttackActive() const;
	bool IsJumpSlamImpactActive() const;
	bool IsJumpRetreating() const { return aiState_ == AIState::kRetreat; }
	static inline constexpr std::size_t kGroundWaveCount = 3;
	bool GetGroundWaveHitbox(std::size_t index, CollisionBox& hitbox) const;
	static inline constexpr std::size_t kShadowPillarCount = 14;
	bool GetShadowPillarState(
	    std::size_t index, CollisionBox& hitbox, float& telegraphProgress,
	    bool& damaging) const;
	bool ConsumeSlamImpact();
	KamataEngine::Vector3 GetPosition() const { return joints_[kRoot].worldPosition; }
	float GetCloseDistance() const { return closeDistance_; }
	float GetMidDistance() const { return midDistance_; }
	bool IsEditorCameraActive() const {
		return controlMode_ == ControlMode::kPoseEditor || controlMode_ == ControlMode::kKeyframeEditor;
	}
	bool IsAnimationDebugMode() const { return controlMode_ == ControlMode::kAnimationDebug; }
	void Draw(const KamataEngine::Camera& camera);
	void DrawHeadPortrait(
	    const KamataEngine::Camera& camera,
	    const KamataEngine::Vector3& rotationOffset,
	    const KamataEngine::Vector3& scaleMultiplier);
	KamataEngine::Vector3 GetHeadPortraitCenter() const;
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
		kJumpSlam,
		kPhaseTwoUppercut,
		kPhaseTwoGroundWave,
		kPhaseTwoPillars,
	};

	enum class ControlMode {
		kAnimationDebug,
		kPoseEditor,
		kKeyframeEditor,
		kPlayTest,
	};

	enum class AIState {
		kWaiting,
		kMeleeAttack,
		kRetreat,
		kSpinAttack,
		kVerticalHook,
		kScytheThrow,
		kJumpSlam,
		kPhaseTwoUppercut,
		kPhaseTwoGroundWave,
		kPhaseTwoPillars,
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
		KamataEngine::Vector3 sourcePivot = {};
		KamataEngine::Vector3 localScale = {1.0f, 1.0f, 1.0f};
		KamataEngine::Vector3 localRotation = {};
		KamataEngine::Vector3 jointOffset = {};
		KamataEngine::Vector3 defaultSourcePivot = {};
		KamataEngine::Vector3 defaultLocalScale = {1.0f, 1.0f, 1.0f};
		KamataEngine::Vector3 defaultLocalRotation = {};
		KamataEngine::Vector3 defaultJointOffset = {};
		std::array<KamataEngine::Model*, 3> articulatedModels = {};
		std::array<KamataEngine::WorldTransform, 4> articulatedMeshTransforms;
		std::array<JointIndex, 4> articulatedMeshJoints = {kRoot, kRoot, kRoot, kRoot};
		std::array<JointIndex, 4> articulatedMeshEndJoints = {
		    kJointCount, kJointCount, kJointCount, kJointCount};
		std::array<KamataEngine::Matrix4x4, 4> inverseBindJointMatrices = {};
		std::size_t articulatedMeshCount = 0;
		bool followsJointSegment = false;
		bool usesLocalAttachment = false;
		bool usesArticulatedMeshes = false;
		// The imported float arm meshes already hang naturally. Cancel only the
		// large debug-skeleton shoulder drop while keeping the small idle sway.
		float idleShoulderCompensationSign = 0.0f;
	};

	struct AttackPose {
		std::array<KamataEngine::Vector3, kJointCount> translationOffsets = {};
		std::array<KamataEngine::Vector3, kJointCount> rotationOffsets = {};
		std::array<KamataEngine::Vector3, kJointCount> scaleOffsets = {};
	};

	struct AttackKeyframe {
		float time = 0.0f;
		AttackPose pose = {};
	};

	void InitializeJoint(JointIndex index, const char* name, int32_t parentIndex, const KamataEngine::Vector3& translation, const KamataEngine::Vector4& color);
	void InitializeModelPart(ModelPart& part, const char* modelName, JointIndex joint, const KamataEngine::Vector3& bindJointPosition);
	void InitializeArmSegment(ModelPart& part, const char* modelName, JointIndex startJoint, JointIndex endJoint);
	void InitializeLocalModelPart(
	    ModelPart& part, const char* modelName, JointIndex joint,
	    const KamataEngine::Vector3& sourcePivot, const KamataEngine::Vector3& localScale,
	    const KamataEngine::Vector3& jointOffset, float idleShoulderCompensationSign = 0.0f);
	void InitializeArticulatedArm(
	    ModelPart& part, const char* modelBaseName, JointIndex elbowJoint, JointIndex handJoint);
	void UpdateModelPart(ModelPart& part);
	void UpdateWeaponTransform();
	void DrawArticulatedModelPart(const ModelPart& part, const KamataEngine::Camera& camera) const;
	void ResetPose();
	void ClearIdlePose();
	void UpdateIdleAnimation();
	void InitializeNormalAttackClip();
	void InitializeScytheThrowClip();
	void InitializeSpinAttackClip();
	void InitializeVerticalHookClip();
	void InitializeJumpSlamClip();
	void InitializePhaseTwoUppercutClip();
	void InitializePhaseTwoGroundWaveClip();
	void InitializePhaseTwoPillarsClip();
	void UpdatePhaseTwoAttackState();
	void StartAnimation(AnimationType animation);
	void StopAnimation();
	void FreezeCurrentPoseForEditing();
	void LoadSelectedKeyframePose();
	void StoreJointInSelectedKeyframe(JointIndex joint);
	void StartKeyframePreview();
	void StopKeyframePreview();
	AttackKeyframe* GetEditableKeyframes(AnimationType animation, std::size_t& count);
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
	static float SmootherStep(float t);

	std::array<Joint, kJointCount> joints_;
	std::array<KamataEngine::Vector3, kJointCount> defaultTranslations_ = {};
	std::array<KamataEngine::Vector3, kJointCount> defaultRotations_ = {};
	std::array<KamataEngine::Vector3, kJointCount> defaultScales_ = {};
	std::array<KamataEngine::Matrix4x4, kJointCount> defaultJointWorldMatrices_ = {};
	// The imported arm meshes were authored against the earlier alignment pose.
	// Keep that immutable mesh reference separate from the editable idle pose.
	std::array<KamataEngine::Matrix4x4, kJointCount> modelBindJointWorldMatrices_ = {};
	std::array<KamataEngine::Vector3, kJointCount> animationBaseTranslations_ = {};
	std::array<KamataEngine::Vector3, kJointCount> animationBaseRotations_ = {};
	std::array<KamataEngine::Vector3, kJointCount> animationBaseScales_ = {};
	std::array<AttackKeyframe, 6> normalAttackKeyframes_ = {};
	std::array<AttackKeyframe, 8> scytheThrowKeyframes_ = {};
	std::array<AttackKeyframe, 8> spinAttackKeyframes_ = {};
	std::array<AttackKeyframe, 6> verticalHookKeyframes_ = {};
	std::array<AttackKeyframe, 6> jumpSlamKeyframes_ = {};
	std::array<AttackKeyframe, 6> phaseTwoUppercutKeyframes_ = {};
	std::array<AttackKeyframe, 6> phaseTwoGroundWaveKeyframes_ = {};
	std::array<AttackKeyframe, 6> phaseTwoPillarsKeyframes_ = {};
	std::array<ModelPart, 4> modelParts_;
	KamataEngine::Model* weaponModel_ = nullptr;
	KamataEngine::WorldTransform weaponTransform_;
	KamataEngine::WorldTransform headPortraitTransform_;
	KamataEngine::Model* jointSphereModel_ = nullptr;
	KamataEngine::ObjectColor defeatColor_;
	bool showBossModel_ = true;
	bool isVisible_ = true;
	bool showDebugArmature_ = false;
	bool showDebugScythe_ = false;
	bool isScytheDetached_ = false;
	bool useExplicitScythePose_ = false;
	bool hasScytheReleaseCenter_ = false;
	bool loopAnimation_ = false;
	bool pauseAnimation_ = false;
	bool keyframePreviewPlaying_ = false;
	bool aiEnabled_ = false;
	bool phaseTransitionActive_ = false;
	bool slamImpactPending_ = false;
	bool isPhaseTwo_ = false;
	std::array<float, kShadowPillarCount> shadowPillarTargetX_ = {};
	std::array<bool, kShadowPillarCount> shadowPillarTargetLocked_ = {};
	AnimationType activeAnimation_ = AnimationType::kNone;
	AnimationType keyframeEditorAnimation_ = AnimationType::kNormalAttack;
	std::size_t selectedKeyframeIndex_ = 0;
	ControlMode controlMode_ = ControlMode::kPlayTest;
	AIState aiState_ = AIState::kWaiting;
	KamataEngine::Vector3 playerTargetPosition_ = {};
	KamataEngine::Vector3 actionTargetPosition_ = {};
	KamataEngine::Vector3 scytheReleaseCenter_ = {};
	KamataEngine::Vector3 scytheTargetCenter_ = {};
	KamataEngine::Vector3 scytheFlightDirection_ = {-1.0f, 0.0f, 0.0f};
	KamataEngine::Vector3 explicitScytheCenter_ = {};
	float explicitScytheRotation_ = 0.0f;
	float scytheThrowFlightDistance_ = 0.0f;
	float facingDirection_ = -1.0f;
	float playerDistance_ = 0.0f;
	float aiWaitTimer_ = 0.0f;
	float retreatTimer_ = 0.0f;
	float retreatTargetX_ = kInitialBossX;
	int lastAIRoll_ = -1;
	uint32_t randomState_ = 0x4D595DF4u;
	float jointRadius_ = 0.10f;
	float weaponModelScale_ = 1.650f;
	// Boss breathing/idle animation tuning. The root joint drives the entire
	// assembled model so every body part stays connected.
	float idleMoveAmount_ = 0.075f;
	float idleScaleAmount_ = 0.022f;
	float idleCycleDuration_ = 2.80f;
	float idleShoulderDrop_ = 0.0f;
	float idleElbowBend_ = 0.0f;
	float idleTorsoLean_ = 0.0f;
	float idleHeadCounterTilt_ = 0.0f;
	float idleArmSway_ = 0.010f;
	float idleAnimationTimer_ = 0.0f;
	float idleBlendTimer_ = 0.0f;
	static inline const float kIdleBlendInDuration = 0.38f;
	// Damage-hitbox defaults. They can also be edited live in the Boss
	// Armature ImGui window under "Damage Hitboxes".
	float bodyHitboxHalfWidth_ = 2.15f;
	float bodyHitboxBottomOffset_ = -0.15f;
	float bodyHitboxTopPadding_ = 1.05f;
	float bodyHitboxHalfDepth_ = 1.00f;
	KamataEngine::Vector3 scytheHitboxPadding_ = {0.20f, 0.20f, 0.20f};
	float weaponHitboxScale_ = 0.85f;
	float throwHitboxHalfWidth_ = 1.05f;
	float throwHitboxMinimumY_ = 1.80f;
	float throwHitboxMaximumY_ = 6.20f;
	float throwHitboxHalfDepth_ = 2.50f;
	// 0.50 is outbound-only; 1.00 keeps collision through the complete return.
	// A short early-return window accounts for the moving hand anchor without
	// restoring the original full boomerang threat.
	float throwDamageEndFlightProgress_ = 0.70f;
	float normalAttackPlaybackSpeed_ = 1.8f;
	float normalAttackPlaybackDuration_ = 2.20f;
	float scytheThrowPlaybackSpeed_ = 1.0f;
	float scytheThrowPlaybackDuration_ = 2.45f;
	float scytheThrowRange_ = 18.0f;
	float scytheThrowArcHeight_ = 0.75f;
	float scytheThrowTargetYOffset_ = 0.00f;
	float scytheThrowSpinCount_ = 4.0f;
	float spinAttackPlaybackSpeed_ = 3.0f;
	float spinAttackPlaybackDuration_ = 5.00f;
	int spinAttackTurnCount_ = 3;
	float verticalHookPlaybackSpeed_ = 1.0f;
	float verticalHookPlaybackDuration_ = 1.40f;
	float verticalHookReach_ = 9.0f;
	float verticalHookTargetYOffset_ = 1.80f;
	float jumpSlamPlaybackSpeed_ = 1.0f;
	float jumpSlamPlaybackDuration_ = 1.55f;
	float jumpSlamMoveSpeed_ = 14.0f;
	float jumpSlamStopDistance_ = 1.25f;
	float closeDistance_ = 3.5f;
	float midDistance_ = 10.0f;
	float phaseOneAIDecisionDelay_ = 0.50f;
	float phaseTwoAIDecisionDelay_ = 0.40f;
	float retreatSpeed_ = 12.0f;
	float retreatDuration_ = 1.10f;
	float retreatJumpHeight_ = 2.20f;
	float spinDashSpeed_ = 6.0f;
	float spinDashStopDistance_ = 1.2f;
	float movementMinX_ = 3.0f;
	float movementMaxX_ = 97.0f;
	// Exact weighted move tables. The final move in each distance band receives
	// the remaining percentage so every band always totals 100%.
	int closeMeleeChance_ = 40;
	int midHookChance_ = 30;
	int midSpinChance_ = 40;
	int farThrowChance_ = 40;
	int phaseTwoCloseUppercutChance_ = 40;
	int phaseTwoCloseMeleeChance_ = 30;
	int phaseTwoMidGroundWaveChance_ = 30;
	int phaseTwoMidSpinChance_ = 30;
	int phaseTwoMidHookChance_ = 20;
	int phaseTwoFarJumpSlamChance_ = 40;
	int phaseTwoFarThrowChance_ = 30;
	float phaseTwoUppercutDashSpeed_ = 11.0f;
	float phaseTwoUppercutStopDistance_ = 0.75f;
	float phaseTwoGroundWaveRange_ = 25.0f;
	float phaseTwoGroundWaveHalfWidth_ = 2.0f;
	float phaseTwoGroundWaveHeight_ = 3.20f;
	float phaseTwoGroundWaveInterval_ = 3.00f;
	float phaseTwoUppercutPlaybackSpeed_ = 1.0f;
	float phaseTwoUppercutPlaybackDuration_ = 1.15f;
	float phaseTwoGroundWavePlaybackSpeed_ = 2.0f;
	float phaseTwoGroundWavePlaybackDuration_ = 9.00f;
	float phaseTwoPillarsPlaybackSpeed_ = 1.0f;
	float phaseTwoPillarsPlaybackDuration_ = 10.00f;
	float animationTime_ = 0.0f;
	static inline const float kFrameTime = 1.0f / 60.0f;
	static inline const float kInitialBossX = 96.0f;
	static inline const float kIdleFacingYaw = -0.410f;
	static inline const float kNormalAttackDuration = 1.35f;
	static inline const float kScytheThrowDuration = 2.45f;
	static inline const float kSpinAttackDuration = 1.95f;
	static inline const float kVerticalHookDuration = 1.40f;
	static inline const float kJumpSlamDuration = 1.55f;
	static inline const float kJumpSlamLaunchTime = 0.28f;
	static inline const float kJumpSlamImpactTime = 1.05f;
	static inline const float kPhaseTwoUppercutDuration = 1.15f;
	static inline const float kPhaseTwoGroundWaveDuration = 9.00f;
	static inline const float kPhaseTwoGroundWaveImpactTime = 0.62f;
	static inline const float kPhaseTwoGroundWaveTravelDuration = 2.20f;
	static inline const float kPhaseTwoPillarsDuration = 10.00f;
	static inline const float kShadowPillarFirstTelegraphTime = 1.00f;
	static inline const float kShadowPillarInterval = 0.57f;
	static inline const float kShadowPillarTelegraphDuration = 0.85f;
	static inline const float kShadowPillarActiveDuration = 0.55f;
	static inline const float kShadowPillarRiseDuration = 0.22f;
	// Give the player a clearer reaction window without speeding up the actual
	// flight: release and catch move later by the same amount.
	static inline const float kScytheReleaseTime = 0.68f;
	static inline const float kScytheCatchTime = 1.94f;
};
