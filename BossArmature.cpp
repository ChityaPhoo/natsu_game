#include "BossArmature.h"
#include "Matrix4x4Calculation.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#ifdef USE_IMGUI
#include <imgui.h>
#endif

using namespace KamataEngine;

void BossArmature::Initialize() {
	InitializeJoint(kRoot, "Root", -1, {kInitialBossX, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f});
	InitializeJoint(kBody, "Body", kRoot, {0.0f, 2.80f, 0.0f}, {1.0f, 0.55f, 0.0f, 1.0f});
	InitializeJoint(kChest, "Chest", kBody, {0.0f, 1.962f, 0.0f}, {1.0f, 0.30f, 0.0f, 1.0f});
	InitializeJoint(kNeck, "Neck", kChest, {0.0f, 0.717f, 0.0f}, {0.0f, 0.75f, 1.0f, 1.0f});
	InitializeJoint(kHead, "Head", kNeck, {0.0f, 0.470f, 0.0f}, {0.0f, 0.35f, 1.0f, 1.0f});
	InitializeJoint(kLeftShoulder, "Left Shoulder", kChest, {1.050f, -0.063f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f});
	InitializeJoint(kLeftElbow, "Left Elbow", kLeftShoulder, {1.409f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f});
	InitializeJoint(kLeftHand, "Left Hand", kLeftElbow, {1.410f, 0.0f, 0.0f}, {0.0f, 0.35f, 1.0f, 1.0f});
	InitializeJoint(kRightShoulder, "Right Shoulder", kChest, {-1.050f, 0.064f, -0.036f}, {1.0f, 0.0f, 0.0f, 1.0f});
	InitializeJoint(kRightElbow, "Right Elbow", kRightShoulder, {-1.409f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f});
	InitializeJoint(kRightHand, "Right Hand", kRightElbow, {-1.409f, 0.0f, 0.0f}, {0.0f, 0.35f, 1.0f, 1.0f});

	// The bind pose stays assembled. The attack clip supplies the scythe grip.
	for (uint32_t index = 0; index < kJointCount; ++index) {
		defaultRotations_[index] = joints_[index].rotation;
	}
	InitializeNormalAttackClip();
	InitializeScytheThrowClip();
	InitializeSpinAttackClip();
	InitializeVerticalHookClip();

	InitializeModelPart(modelParts_[0], "bossBody", kBody, {0.0f, 2.80f, 0.0f});
	InitializeModelPart(modelParts_[1], "bossHead", kHead, {0.0f, 6.279f, 0.0f});
	// Arm meshes are centered unit segments. Their endpoints are fitted to the
	// animated joints every frame, so OBJ handedness and bind-pose pivots cannot
	// pull the geometry away from the debug armature.
	InitializeArmSegment(modelParts_[2], "bossLeftUpperArm", kLeftShoulder, kLeftElbow);
	InitializeArmSegment(modelParts_[3], "bossLeftLowerArm", kLeftElbow, kLeftHand);
	InitializeArmSegment(modelParts_[4], "bossRightUpperArm", kRightShoulder, kRightElbow);
	InitializeArmSegment(modelParts_[5], "bossRightLowerArm", kRightElbow, kRightHand);
	jointSphereModel_ = Model::CreateSphere(8, 8);
	defeatColor_.Initialize();
	defeatColor_.SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	isVisible_ = true;
	phaseTransitionActive_ = false;
	idleAnimationTimer_ = 0.0f;
	playerTargetPosition_ = {2.5f, 1.401f, 0.0f};
	actionTargetPosition_ = playerTargetPosition_;
	FacePlayer();
	Update(playerTargetPosition_);
}

void BossArmature::InitializeJoint(JointIndex index, const char* name, int32_t parentIndex, const Vector3& translation, const Vector4& color) {
	Joint& joint = joints_[index];
	joint.name = name;
	joint.parentIndex = parentIndex;
	joint.translation = translation;
	defaultTranslations_[index] = translation;
	joint.markerTransform.Initialize();
	joint.markerColor.Initialize();
	joint.markerColor.SetColor(color);
}

void BossArmature::InitializeModelPart(ModelPart& part, const char* modelName, JointIndex joint, const Vector3& bindJointPosition) {
	part.model = Model::CreateFromOBJ(modelName, true);
	part.worldTransform.Initialize();
	part.joint = joint;
	part.bindJointPosition = bindJointPosition;
	part.followsJointSegment = false;
}

void BossArmature::InitializeArmSegment(ModelPart& part, const char* modelName, JointIndex startJoint, JointIndex endJoint) {
	part.model = Model::CreateFromOBJ(modelName, true);
	part.worldTransform.Initialize();
	part.joint = startJoint;
	part.segmentEndJoint = endJoint;
	part.followsJointSegment = true;
}

void BossArmature::Update(const Vector3& playerPosition) {
	playerTargetPosition_ = playerPosition;
	playerDistance_ = std::abs(playerTargetPosition_.x - defaultTranslations_[kRoot].x);
	UpdateAI();
	UpdateAnimation();
	UpdateIdleAnimation();
	for (uint32_t index = 0; index < kJointCount; ++index) {
		Joint& joint = joints_[index];
		joint.localMatrix = Matrix4x4Calculation::MakeAffineMatrix(joint.scale, joint.rotation, joint.translation);
		joint.worldMatrix = joint.parentIndex < 0
		    ? joint.localMatrix
		    : Matrix4x4Calculation::Multiply(joint.localMatrix, joints_[static_cast<uint32_t>(joint.parentIndex)].worldMatrix);
		joint.worldPosition = Matrix4x4Calculation::TransformPoint({}, joint.worldMatrix);
		joint.markerTransform.scale_ = {jointRadius_, jointRadius_, jointRadius_};
		joint.markerTransform.translation_ = joint.worldPosition;
		joint.markerTransform.matWorld_ = Matrix4x4Calculation::MakeAffineMatrix(
		    joint.markerTransform.scale_, joint.markerTransform.rotation_, joint.markerTransform.translation_);
		joint.markerTransform.TransferMatrix();
	}
	UpdateScytheState();

	for (ModelPart& part : modelParts_) { UpdateModelPart(part); }
}

void BossArmature::UpdateModelPart(ModelPart& part) {
	if (part.followsJointSegment) {
		const Vector3& start = joints_[part.joint].worldPosition;
		const Vector3& end = joints_[part.segmentEndJoint].worldPosition;
		const Vector3 difference = {end.x - start.x, end.y - start.y, end.z - start.z};
		const float length = std::sqrt(
		    difference.x * difference.x + difference.y * difference.y + difference.z * difference.z);
		if (length <= 0.0001f) { return; }

		// Rotate local +X onto the bone direction, then put the centered unit mesh
		// halfway between its two joints. This guarantees its ends stay on them.
		const float horizontalLength = std::sqrt(difference.x * difference.x + difference.y * difference.y);
		const Vector3 segmentRotation = {
		    0.0f, -std::atan2(difference.z, horizontalLength), std::atan2(difference.y, difference.x)};
		const Vector3 segmentCenter = {
		    (start.x + end.x) * 0.5f, (start.y + end.y) * 0.5f, (start.z + end.z) * 0.5f};
		part.worldTransform.matWorld_ = Matrix4x4Calculation::MakeAffineMatrix(
		    {length, 1.0f, 1.0f}, segmentRotation, segmentCenter);
		part.worldTransform.TransferMatrix();
		return;
	}

	const Matrix4x4 bindOffset = Matrix4x4Calculation::MakeTranslateMatrix({
	    -part.bindJointPosition.x, -part.bindJointPosition.y, -part.bindJointPosition.z});
	part.worldTransform.matWorld_ = Matrix4x4Calculation::Multiply(bindOffset, joints_[part.joint].worldMatrix);
	part.worldTransform.TransferMatrix();
}

void BossArmature::Draw(const Camera& camera) {
	if (!showBossModel_ || !isVisible_) { return; }
	for (const ModelPart& part : modelParts_) {
		if (part.model != nullptr) { part.model->Draw(part.worldTransform, camera, &defeatColor_); }
	}
}

void BossArmature::DrawDebug(const Camera& camera) {
	if (!isVisible_) { return; }
	if (showDebugArmature_) {
		Model::PreDraw(Model::CullingMode::kNone);
		for (const Joint& joint : joints_) {
			jointSphereModel_->Draw(joint.markerTransform, camera, &joint.markerColor);
		}
		Model::PostDraw();

		PrimitiveDrawer* drawer = PrimitiveDrawer::GetInstance();
		drawer->SetCamera(&camera);
		for (const Joint& joint : joints_) {
			if (joint.parentIndex >= 0) {
				drawer->DrawLine3d(joints_[static_cast<uint32_t>(joint.parentIndex)].worldPosition, joint.worldPosition, {1.0f, 1.0f, 1.0f, 1.0f});
			}
		}
	}
	if (showDebugScythe_) {
		DrawDebugScythe(camera);
	}
}

#ifdef USE_IMGUI
void BossArmature::DrawImGui() {
	ImGui::Begin("Boss Armature");
	ImGui::Checkbox("Show boss model", &showBossModel_);
	ImGui::Checkbox("Show debug armature", &showDebugArmature_);
	ImGui::Checkbox("Show debug scythe", &showDebugScythe_);
	ImGui::DragFloat("Joint sphere radius", &jointRadius_, 0.005f, 0.02f, 0.50f);
	if (ImGui::CollapsingHeader("Idle Animation", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::DragFloat("Idle vertical move", &idleMoveAmount_, 0.005f, 0.0f, 0.50f);
		ImGui::DragFloat("Idle breathing scale", &idleScaleAmount_, 0.001f, 0.0f, 0.15f);
		ImGui::DragFloat("Idle cycle duration", &idleCycleDuration_, 0.05f, 0.20f, 8.0f, "%.2f sec");
	}
	if (ImGui::CollapsingHeader("Damage Hitboxes", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::TextUnformatted("Enable collision boxes in Combat Debug to preview these.");
		ImGui::DragFloat("Body half width", &bodyHitboxHalfWidth_, 0.02f, 0.10f, 8.0f);
		ImGui::DragFloat("Body bottom offset", &bodyHitboxBottomOffset_, 0.02f, -4.0f, 4.0f);
		ImGui::DragFloat("Body head padding", &bodyHitboxTopPadding_, 0.02f, 0.0f, 5.0f);
		ImGui::DragFloat("Body half depth", &bodyHitboxHalfDepth_, 0.02f, 0.10f, 5.0f);
		ImGui::DragFloat3("Scythe hitbox padding", &scytheHitboxPadding_.x, 0.02f, 0.0f, 4.0f);
	}

	ImGui::SeparatorText("Control Mode");
	if (ImGui::RadioButton("Animation Debug", controlMode_ == ControlMode::kAnimationDebug)) {
		SetControlMode(ControlMode::kAnimationDebug);
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("AI Play Test", controlMode_ == ControlMode::kPlayTest)) {
		SetControlMode(ControlMode::kPlayTest);
	}

	if (controlMode_ == ControlMode::kPlayTest) {
		ImGui::SeparatorText("AI Play Test");
		ImGui::Text("Encounter: %s", aiEnabled_ ? "Active" : "Waiting for player");
		ImGui::Text("State: %s", GetAIStateName());
		ImGui::Text("Distance: %.2f (%s)", playerDistance_, GetDistanceBandName());
		ImGui::Text("Facing: %s", facingDirection_ < 0.0f ? "Left" : "Right");
		if (lastAIRoll_ < 0) {
			ImGui::TextUnformatted("Last decision roll: None");
		} else {
			ImGui::Text("Last decision roll: %d", lastAIRoll_);
		}

		if (ImGui::CollapsingHeader("AI Distance and Movement", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::DragFloat("Close distance", &closeDistance_, 0.10f, 1.0f, 20.0f);
			ImGui::DragFloat("Mid distance", &midDistance_, 0.10f, 2.0f, 30.0f);
			closeDistance_ = std::clamp(closeDistance_, 1.0f, 19.5f);
			midDistance_ = std::clamp(midDistance_, closeDistance_ + 0.5f, 30.0f);
			ImGui::DragFloat("Decision delay", &aiDecisionDelay_, 0.05f, 0.0f, 3.0f, "%.2f sec");
			ImGui::DragFloat("Retreat speed", &retreatSpeed_, 0.10f, 0.1f, 8.0f);
			ImGui::DragFloat("Retreat duration", &retreatDuration_, 0.05f, 0.1f, 3.0f, "%.2f sec");
			ImGui::DragFloat("Spin dash speed", &spinDashSpeed_, 0.10f, 0.0f, 8.0f);
			ImGui::DragFloat("Spin stop distance", &spinDashStopDistance_, 0.10f, 0.5f, 5.0f);
		}

		if (ImGui::CollapsingHeader("AI Move Chances", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Text("Close (distance < %.1f)", closeDistance_);
			ImGui::SliderInt("Close Melee", &closeMeleeChance_, 0, 100, "%d%%");
			ImGui::Text("Close Retreat (auto): %d%%", 100 - closeMeleeChance_);
			ImGui::Separator();
			ImGui::Text("Mid (%.1f - %.1f)", closeDistance_, midDistance_);
			ImGui::SliderInt("Mid Spin Attack", &midSpinChance_, 0, 100, "%d%%");
			ImGui::Text("Mid Vertical Hook (auto): %d%%", 100 - midSpinChance_);
			ImGui::Separator();
			ImGui::Text("Far (distance >= %.1f)", midDistance_);
			ImGui::SliderInt("Far Scythe Throw", &farThrowChance_, 0, 100, "%d%%");
			ImGui::Text("Far Wait (auto): %d%%", 100 - farThrowChance_);
		}

		if (ImGui::CollapsingHeader("Force AI Move")) {
			if (ImGui::Button("Force Melee")) { EnterAIState(AIState::kMeleeAttack); }
			ImGui::SameLine();
			if (ImGui::Button("Force Retreat")) { EnterAIState(AIState::kRetreat); }
			if (ImGui::Button("Force Spin")) { EnterAIState(AIState::kSpinAttack); }
			ImGui::SameLine();
			if (ImGui::Button("Force Hook")) { EnterAIState(AIState::kVerticalHook); }
			ImGui::SameLine();
			if (ImGui::Button("Force Throw")) { EnterAIState(AIState::kScytheThrow); }
		}
		if (ImGui::Button("Reset AI and boss position")) { ResetBossPosition(); }
	} else {
		ImGui::TextUnformatted("AI inactive: use the Play buttons to inspect each move.");
	}

	ImGui::SeparatorText("Animation Tuning");
	if (ImGui::CollapsingHeader("Normal Melee", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (controlMode_ == ControlMode::kAnimationDebug && ImGui::Button("Play Normal Attack")) { StartAnimation(AnimationType::kNormalAttack); }
		ImGui::DragFloat("Melee speed", &normalAttackPlaybackSpeed_, 0.05f, 0.10f, 3.0f);
		ImGui::DragFloat("Melee duration", &normalAttackPlaybackDuration_, 0.05f, 0.30f, 4.0f, "%.2f sec");
	}
	if (ImGui::CollapsingHeader("Scythe Throw", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (controlMode_ == ControlMode::kAnimationDebug && ImGui::Button("Play Scythe Throw")) { StartAnimation(AnimationType::kScytheThrow); }
		ImGui::DragFloat("Throw speed", &scytheThrowPlaybackSpeed_, 0.05f, 0.10f, 3.0f);
		ImGui::DragFloat("Throw duration", &scytheThrowPlaybackDuration_, 0.05f, 0.50f, 6.0f, "%.2f sec");
		ImGui::DragFloat("Throw range", &scytheThrowRange_, 0.10f, 1.0f, 14.0f);
		ImGui::DragFloat("Throw arc height", &scytheThrowArcHeight_, 0.05f, 0.0f, 4.0f);
		ImGui::DragFloat("Throw target Y offset", &scytheThrowTargetYOffset_, 0.05f, -2.0f, 5.0f);
		ImGui::DragFloat("Throw spin count", &scytheThrowSpinCount_, 0.25f, 0.0f, 10.0f);
	}
	if (ImGui::CollapsingHeader("Spin Attack")) {
		if (controlMode_ == ControlMode::kAnimationDebug && ImGui::Button("Play Spin Attack")) { StartAnimation(AnimationType::kSpinAttack); }
		ImGui::DragFloat("Spin speed", &spinAttackPlaybackSpeed_, 0.05f, 0.10f, 3.0f);
		ImGui::DragFloat("Spin duration", &spinAttackPlaybackDuration_, 0.05f, 0.30f, 5.0f, "%.2f sec");
		ImGui::SliderInt("Spin turn count", &spinAttackTurnCount_, 1, 3);
	}
	if (ImGui::CollapsingHeader("Vertical Hook", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (controlMode_ == ControlMode::kAnimationDebug && ImGui::Button("Play Vertical Hook")) { StartAnimation(AnimationType::kVerticalHook); }
		ImGui::DragFloat("Hook speed", &verticalHookPlaybackSpeed_, 0.05f, 0.10f, 3.0f);
		ImGui::DragFloat("Hook duration", &verticalHookPlaybackDuration_, 0.05f, 0.40f, 5.0f, "%.2f sec");
		ImGui::DragFloat("Hook range", &verticalHookReach_, 0.10f, 4.0f, 14.0f);
		ImGui::DragFloat("Hook target Y offset", &verticalHookTargetYOffset_, 0.05f, -2.0f, 5.0f);
	}
	ImGui::SeparatorText("Playback Debug");
	ImGui::Text("Playing: %s", GetActiveAnimationName());
	if (isScytheDetached_) { ImGui::TextUnformatted("Scythe state: airborne"); }
	if (controlMode_ == ControlMode::kAnimationDebug) {
		ImGui::Checkbox("Pause animation", &pauseAnimation_);
		ImGui::SameLine();
		ImGui::Checkbox("Loop animation", &loopAnimation_);
	}
	const float authoredDuration = GetActiveAnimationDuration();
	if (authoredDuration > 0.0f) {
		if (controlMode_ == ControlMode::kAnimationDebug) {
			ImGui::SliderFloat("Timeline", &animationTime_, 0.0f, authoredDuration, "%.2f");
		}
		const float effectiveSeconds = GetActivePlaybackDuration() / GetActivePlaybackSpeed();
		ImGui::Text("Effective play time: %.2f sec", effectiveSeconds);
	}
	const float progress = authoredDuration > 0.0f ? std::clamp(animationTime_ / authoredDuration, 0.0f, 1.0f) : 0.0f;
	ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f));
	if (controlMode_ == ControlMode::kAnimationDebug) {
		if (ImGui::Button("Stop Animation")) { StopAnimation(); }
		ImGui::SameLine();
		if (ImGui::Button("Reset pose")) { ResetPose(); }
		ImGui::Separator();
		ImGui::TextUnformatted("Local SRT (radians for rotation)");
		ImGui::BeginDisabled(activeAnimation_ != AnimationType::kNone);
		for (uint32_t index = 0; index < kJointCount; ++index) {
			Joint& joint = joints_[index];
			ImGui::PushID(static_cast<int>(index));
			if (ImGui::TreeNode(joint.name)) {
				ImGui::Text("Parent: %s", joint.parentIndex < 0 ? "None" : joints_[static_cast<uint32_t>(joint.parentIndex)].name);
				ImGui::DragFloat3("Translation", &joint.translation.x, 0.01f);
				ImGui::DragFloat3("Rotation", &joint.rotation.x, 0.01f);
				ImGui::DragFloat3("Scale", &joint.scale.x, 0.01f, 0.01f, 10.0f);
				ImGui::Text("World: %.2f, %.2f, %.2f", joint.worldPosition.x, joint.worldPosition.y, joint.worldPosition.z);
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
		ImGui::EndDisabled();
	}
	ImGui::End();
}
#endif

void BossArmature::ResetPose() {
	activeAnimation_ = AnimationType::kNone;
	animationTime_ = 0.0f;
	isScytheDetached_ = false;
	useExplicitScythePose_ = false;
	hasScytheReleaseCenter_ = false;
	pauseAnimation_ = false;
	for (uint32_t index = 0; index < kJointCount; ++index) {
		joints_[index].translation = defaultTranslations_[index];
		joints_[index].rotation = defaultRotations_[index];
		joints_[index].scale = {1.0f, 1.0f, 1.0f};
	}
}

void BossArmature::ClearIdlePose() {
	joints_[kRoot].translation.y = defaultTranslations_[kRoot].y;
	joints_[kRoot].translation.z = defaultTranslations_[kRoot].z;
	joints_[kRoot].scale = {1.0f, 1.0f, 1.0f};
}

void BossArmature::UpdateIdleAnimation() {
	idleAnimationTimer_ += kFrameTime;
	const float cycleDuration = (std::max)(idleCycleDuration_, kFrameTime);
	if (idleAnimationTimer_ >= cycleDuration) { idleAnimationTimer_ = std::fmod(idleAnimationTimer_, cycleDuration); }
	if (phaseTransitionActive_ || activeAnimation_ != AnimationType::kNone) { return; }

	const float breath = std::sin(idleAnimationTimer_ / cycleDuration * 2.0f * std::numbers::pi_v<float>);
	joints_[kRoot].translation.y = defaultTranslations_[kRoot].y + breath * idleMoveAmount_;
	joints_[kRoot].scale = {
	    1.0f - breath * idleScaleAmount_ * 0.45f,
	    1.0f + breath * idleScaleAmount_,
	    1.0f - breath * idleScaleAmount_ * 0.45f};
}

void BossArmature::BeginPhaseTransition() {
	if (activeAnimation_ != AnimationType::kNone) { StopAnimation(); }
	ClearIdlePose();
	phaseTransitionActive_ = true;
}

void BossArmature::SetPhaseTransitionProgress(float progress) {
	if (!phaseTransitionActive_) { return; }
	progress = std::clamp(progress, 0.0f, 1.0f);

	float horizontalScale = 1.0f;
	float verticalScale = 1.0f;
	float verticalOffset = 0.0f;
	float armRaise = 0.0f;
	if (progress < 0.32f) {
		const float t = SmoothStep(progress / 0.32f);
		horizontalScale = std::lerp(1.0f, 1.12f, t);
		verticalScale = std::lerp(1.0f, 0.82f, t);
		verticalOffset = std::lerp(0.0f, -0.28f, t);
		armRaise = std::lerp(0.0f, 0.10f, t);
	} else if (progress < 0.72f) {
		const float t = SmoothStep((progress - 0.32f) / 0.40f);
		horizontalScale = std::lerp(1.12f, 0.90f, t);
		verticalScale = std::lerp(0.82f, 1.18f, t);
		verticalOffset = std::lerp(-0.28f, 0.18f, t);
		armRaise = std::lerp(0.10f, 0.50f, t);
	} else {
		const float t = SmoothStep((progress - 0.72f) / 0.28f);
		horizontalScale = std::lerp(0.90f, 1.0f, t);
		verticalScale = std::lerp(1.18f, 1.0f, t);
		verticalOffset = std::lerp(0.18f, 0.0f, t);
		armRaise = std::lerp(0.50f, 0.0f, t);
	}

	joints_[kRoot].translation.y = defaultTranslations_[kRoot].y + verticalOffset;
	joints_[kRoot].scale = {horizontalScale, verticalScale, horizontalScale};
	joints_[kLeftShoulder].rotation.z = defaultRotations_[kLeftShoulder].z + armRaise;
	joints_[kRightShoulder].rotation.z = defaultRotations_[kRightShoulder].z - armRaise;
	const float brightness = 1.0f + std::sin(progress * std::numbers::pi_v<float>) * 0.75f;
	defeatColor_.SetColor({brightness, brightness, brightness, 1.0f});
}

void BossArmature::EndPhaseTransition() {
	ClearIdlePose();
	joints_[kLeftShoulder].rotation.z = defaultRotations_[kLeftShoulder].z;
	joints_[kRightShoulder].rotation.z = defaultRotations_[kRightShoulder].z;
	defeatColor_.SetColor({1.0f, 1.0f, 1.0f, 1.0f});
	phaseTransitionActive_ = false;
}

void BossArmature::SetControlMode(ControlMode mode) {
	if (controlMode_ == mode) { return; }
	StopAnimation();
	controlMode_ = mode;
	aiState_ = AIState::kWaiting;
	aiWaitTimer_ = mode == ControlMode::kPlayTest && aiEnabled_ ? 0.25f : 0.0f;
	retreatTimer_ = 0.0f;
	lastAIRoll_ = -1;
	loopAnimation_ = false;
	pauseAnimation_ = false;
	showDebugArmature_ = mode == ControlMode::kAnimationDebug;
	showDebugScythe_ = true;
	FacePlayer();
}

void BossArmature::SetAIEnabled(bool enabled) {
	if (aiEnabled_ == enabled) { return; }
	if (controlMode_ == ControlMode::kPlayTest && activeAnimation_ != AnimationType::kNone) { StopAnimation(); }
	aiEnabled_ = enabled;
	aiState_ = AIState::kWaiting;
	aiWaitTimer_ = enabled ? 0.45f : 0.0f;
	retreatTimer_ = 0.0f;
	lastAIRoll_ = -1;
	FacePlayer();
}

void BossArmature::SetHorizontalBounds(float minX, float maxX) {
	movementMinX_ = (std::min)(minX, maxX);
	movementMaxX_ = (std::max)(minX, maxX);
	MoveBossX(0.0f);
}

void BossArmature::SetDefeatBrightness(float brightness) {
	brightness = std::clamp(brightness, 1.0f, 5.0f);
	defeatColor_.SetColor({brightness, brightness, brightness, 1.0f});
}

BossArmature::CollisionBox BossArmature::GetBodyHitbox() const {
	const Vector3& root = joints_[kRoot].worldPosition;
	const Vector3& head = joints_[kHead].worldPosition;
	return {
	    {root.x - bodyHitboxHalfWidth_, root.y + bodyHitboxBottomOffset_, root.z - bodyHitboxHalfDepth_},
	    {root.x + bodyHitboxHalfWidth_, head.y + bodyHitboxTopPadding_, root.z + bodyHitboxHalfDepth_}};
}

BossArmature::CollisionBox BossArmature::GetScytheHitbox() const {
	Vector3 gripA = joints_[kRightHand].worldPosition;
	Vector3 gripB = joints_[kLeftHand].worldPosition;
	if (useExplicitScythePose_) {
		const Vector3 explicitAxis = {std::cos(explicitScytheRotation_), std::sin(explicitScytheRotation_), 0.0f};
		gripA = {
		    explicitScytheCenter_.x - explicitAxis.x * 0.5f,
		    explicitScytheCenter_.y - explicitAxis.y * 0.5f,
		    explicitScytheCenter_.z};
		gripB = {
		    explicitScytheCenter_.x + explicitAxis.x * 0.5f,
		    explicitScytheCenter_.y + explicitAxis.y * 0.5f,
		    explicitScytheCenter_.z};
	}

	const Vector3 difference = {gripB.x - gripA.x, gripB.y - gripA.y, gripB.z - gripA.z};
	const float length = std::sqrt(difference.x * difference.x + difference.y * difference.y + difference.z * difference.z);
	if (length <= 0.001f) {
		return {
		    {gripA.x - scytheHitboxPadding_.x, gripA.y - scytheHitboxPadding_.y, gripA.z - scytheHitboxPadding_.z},
		    {gripA.x + scytheHitboxPadding_.x, gripA.y + scytheHitboxPadding_.y, gripA.z + scytheHitboxPadding_.z}};
	}

	const Vector3 axis = {difference.x / length, difference.y / length, difference.z / length};
	Vector3 perpendicular = {-axis.y, axis.x, 0.0f};
	const float perpendicularLength = std::sqrt(
	    perpendicular.x * perpendicular.x + perpendicular.y * perpendicular.y + perpendicular.z * perpendicular.z);
	if (perpendicularLength > 0.001f) {
		perpendicular = {
		    perpendicular.x / perpendicularLength,
		    perpendicular.y / perpendicularLength,
		    perpendicular.z / perpendicularLength};
	} else {
		perpendicular = {0.0f, 1.0f, 0.0f};
	}

	const Vector3 handleStart = {gripA.x - axis.x * 0.8f, gripA.y - axis.y * 0.8f, gripA.z - axis.z * 0.8f};
	const Vector3 handleEnd = {gripB.x + axis.x * 2.3f, gripB.y + axis.y * 2.3f, gripB.z + axis.z * 2.3f};
	Vector3 minimum = {
	    (std::min)(handleStart.x, handleEnd.x),
	    (std::min)(handleStart.y, handleEnd.y),
	    (std::min)(handleStart.z, handleEnd.z)};
	Vector3 maximum = {
	    (std::max)(handleStart.x, handleEnd.x),
	    (std::max)(handleStart.y, handleEnd.y),
	    (std::max)(handleStart.z, handleEnd.z)};
	for (int segment = 1; segment <= 8; ++segment) {
		const float t = static_cast<float>(segment) / 8.0f;
		const float curve = std::sin(t * std::numbers::pi_v<float> * 0.75f);
		const Vector3 point = {
		    handleEnd.x - axis.x * (1.8f * t) + perpendicular.x * (2.2f * curve),
		    handleEnd.y - axis.y * (1.8f * t) + perpendicular.y * (2.2f * curve),
		    handleEnd.z - axis.z * (1.8f * t)};
		minimum.x = (std::min)(minimum.x, point.x);
		minimum.y = (std::min)(minimum.y, point.y);
		minimum.z = (std::min)(minimum.z, point.z);
		maximum.x = (std::max)(maximum.x, point.x);
		maximum.y = (std::max)(maximum.y, point.y);
		maximum.z = (std::max)(maximum.z, point.z);
	}
	return {
	    {minimum.x - scytheHitboxPadding_.x, minimum.y - scytheHitboxPadding_.y, minimum.z - scytheHitboxPadding_.z},
	    {maximum.x + scytheHitboxPadding_.x, maximum.y + scytheHitboxPadding_.y, maximum.z + scytheHitboxPadding_.z}};
}

bool BossArmature::IsScytheAttackActive() const {
	switch (activeAnimation_) {
	case AnimationType::kNormalAttack:
		return animationTime_ >= 0.30f && animationTime_ <= 0.82f;
	case AnimationType::kScytheThrow:
		return animationTime_ >= 0.58f && animationTime_ <= 1.92f;
	case AnimationType::kSpinAttack:
		return animationTime_ >= 0.20f && animationTime_ <= 1.72f;
	case AnimationType::kVerticalHook:
		return animationTime_ >= 0.32f && animationTime_ <= 0.92f;
	case AnimationType::kNone:
		return false;
	}
	return false;
}

void BossArmature::UpdateAI() {
	if (controlMode_ != ControlMode::kPlayTest || !aiEnabled_) { return; }

	switch (aiState_) {
	case AIState::kWaiting:
		FacePlayer();
		aiWaitTimer_ = (std::max)(0.0f, aiWaitTimer_ - kFrameTime);
		if (aiWaitTimer_ <= 0.0f) { PickNextAIAction(); }
		break;
	case AIState::kRetreat:
		FacePlayer();
		MoveBossX(-facingDirection_ * retreatSpeed_ * kFrameTime);
		retreatTimer_ = (std::max)(0.0f, retreatTimer_ - kFrameTime);
		if (retreatTimer_ <= 0.0f) { EnterAIState(AIState::kWaiting); }
		break;
	case AIState::kSpinAttack:
		if (activeAnimation_ == AnimationType::kNone) {
			EnterAIState(AIState::kWaiting);
			break;
		}
		{
			const float difference = playerTargetPosition_.x - defaultTranslations_[kRoot].x;
			const float distance = std::abs(difference);
			if (distance > spinDashStopDistance_) {
				const float direction = difference < 0.0f ? -1.0f : 1.0f;
				const float movement = (std::min)(spinDashSpeed_ * kFrameTime, distance - spinDashStopDistance_);
				MoveBossX(direction * movement);
			}
		}
		break;
	case AIState::kMeleeAttack:
	case AIState::kVerticalHook:
	case AIState::kScytheThrow:
		if (activeAnimation_ == AnimationType::kNone) { EnterAIState(AIState::kWaiting); }
		break;
	}
}

void BossArmature::PickNextAIAction() {
	FacePlayer();
	lastAIRoll_ = NextRandomPercent();
	if (playerDistance_ < closeDistance_) {
		EnterAIState(lastAIRoll_ < closeMeleeChance_ ? AIState::kMeleeAttack : AIState::kRetreat);
		return;
	}
	if (playerDistance_ < midDistance_) {
		EnterAIState(lastAIRoll_ < midSpinChance_ ? AIState::kSpinAttack : AIState::kVerticalHook);
		return;
	}
	EnterAIState(lastAIRoll_ < farThrowChance_ ? AIState::kScytheThrow : AIState::kWaiting);
}

void BossArmature::EnterAIState(AIState state) {
	if (activeAnimation_ != AnimationType::kNone) { StopAnimation(); }
	aiState_ = state;
	actionTargetPosition_ = playerTargetPosition_;
	FacePlayer();
	loopAnimation_ = false;
	pauseAnimation_ = false;

	switch (state) {
	case AIState::kWaiting:
		aiWaitTimer_ = aiDecisionDelay_;
		break;
	case AIState::kMeleeAttack:
		StartAnimation(AnimationType::kNormalAttack);
		break;
	case AIState::kRetreat:
		retreatTimer_ = retreatDuration_;
		break;
	case AIState::kSpinAttack:
		StartAnimation(AnimationType::kSpinAttack);
		break;
	case AIState::kVerticalHook:
		StartAnimation(AnimationType::kVerticalHook);
		break;
	case AIState::kScytheThrow:
		StartAnimation(AnimationType::kScytheThrow);
		break;
	}
}

void BossArmature::FacePlayer() {
	const float difference = playerTargetPosition_.x - defaultTranslations_[kRoot].x;
	if (std::abs(difference) > 0.001f) { facingDirection_ = difference < 0.0f ? -1.0f : 1.0f; }
	const float facingRotation = facingDirection_ < 0.0f ? 0.0f : std::numbers::pi_v<float>;
	joints_[kRoot].rotation.y = facingRotation;
	defaultRotations_[kRoot].y = facingRotation;
}

void BossArmature::MoveBossX(float movement) {
	const float oldX = defaultTranslations_[kRoot].x;
	const float newX = std::clamp(oldX + movement, movementMinX_, movementMaxX_);
	const float appliedMovement = newX - oldX;
	defaultTranslations_[kRoot].x = newX;
	joints_[kRoot].translation.x += appliedMovement;
	animationBaseTranslations_[kRoot].x += appliedMovement;
	playerDistance_ = std::abs(playerTargetPosition_.x - newX);
}

void BossArmature::ResetBossPosition() {
	StopAnimation();
	MoveBossX(kInitialBossX - defaultTranslations_[kRoot].x);
	ResetPose();
	aiState_ = AIState::kWaiting;
	aiWaitTimer_ = 0.25f;
	retreatTimer_ = 0.0f;
	lastAIRoll_ = -1;
	FacePlayer();
}

int BossArmature::NextRandomPercent() {
	randomState_ = randomState_ * 1664525u + 1013904223u;
	return static_cast<int>((randomState_ >> 16u) % 100u);
}

void BossArmature::InitializeNormalAttackClip() {
	for (AttackKeyframe& keyframe : normalAttackKeyframes_) { keyframe = {}; }
	normalAttackKeyframes_[0].time = 0.00f;
	normalAttackKeyframes_[1].time = 0.32f;
	normalAttackKeyframes_[2].time = 0.50f;
	normalAttackKeyframes_[3].time = 0.69f;
	normalAttackKeyframes_[4].time = 0.88f;
	normalAttackKeyframes_[5].time = kNormalAttackDuration;

	// Wind-up: twist the cloak and lift the two-handed grip over the right shoulder.
	AttackPose& windUp = normalAttackKeyframes_[1].pose;
	windUp.rotationOffsets[kBody].z = 0.12f;
	windUp.rotationOffsets[kChest].z = 0.34f;
	windUp.rotationOffsets[kNeck].z = -0.12f;
	windUp.rotationOffsets[kHead].z = -0.18f;
	windUp.rotationOffsets[kLeftShoulder].z = 0.04f;
	windUp.rotationOffsets[kLeftElbow].z = 2.02f;
	windUp.rotationOffsets[kRightShoulder].z = -3.44f;
	windUp.rotationOffsets[kRightElbow].z = 1.17f;

	// Brief anticipation before the cut, with a little extra torso coil.
	AttackPose& anticipation = normalAttackKeyframes_[2].pose;
	anticipation = windUp;
	anticipation.rotationOffsets[kBody].z = 0.17f;
	anticipation.rotationOffsets[kChest].z = 0.44f;
	anticipation.rotationOffsets[kHead].z = -0.22f;
	anticipation.rotationOffsets[kLeftShoulder].z = -0.02f;
	anticipation.rotationOffsets[kLeftElbow].z = 2.08f;
	anticipation.rotationOffsets[kRightShoulder].z = -3.52f;
	anticipation.rotationOffsets[kRightElbow].z = 1.24f;
	anticipation.translationOffsets[kBody].y = -0.08f;

	// Fast diagonal cut from upper-right to lower-left.
	AttackPose& strike = normalAttackKeyframes_[3].pose;
	strike.translationOffsets[kRoot].x = -0.18f;
	strike.rotationOffsets[kBody].z = -0.18f;
	strike.rotationOffsets[kChest].z = -0.40f;
	strike.rotationOffsets[kNeck].z = 0.10f;
	strike.rotationOffsets[kHead].z = 0.18f;
	strike.rotationOffsets[kLeftShoulder].z = -3.03f;
	strike.rotationOffsets[kLeftElbow].z = 1.13f;
	strike.rotationOffsets[kRightShoulder].z = -5.20f;
	strike.rotationOffsets[kRightElbow].z = 2.07f;

	// Follow-through lets the weapon weight pull the shoulders past the target.
	AttackPose& followThrough = normalAttackKeyframes_[4].pose;
	followThrough.translationOffsets[kRoot].x = -0.24f;
	followThrough.rotationOffsets[kBody].z = -0.25f;
	followThrough.rotationOffsets[kChest].z = -0.52f;
	followThrough.rotationOffsets[kNeck].z = 0.14f;
	followThrough.rotationOffsets[kHead].z = 0.24f;
	followThrough.rotationOffsets[kLeftShoulder].z = -2.82f;
	followThrough.rotationOffsets[kLeftElbow].z = 0.94f;
	followThrough.rotationOffsets[kRightShoulder].z = -5.02f;
	followThrough.rotationOffsets[kRightElbow].z = 1.68f;

	// Recover through the equivalent -2pi angle to avoid spinning the right arm the long way around.
	normalAttackKeyframes_[5].pose.rotationOffsets[kRightShoulder].z = -2.0f * std::numbers::pi_v<float>;
}

void BossArmature::InitializeScytheThrowClip() {
	for (AttackKeyframe& keyframe : scytheThrowKeyframes_) { keyframe = {}; }
	scytheThrowKeyframes_[0].time = 0.00f;
	scytheThrowKeyframes_[1].time = 0.18f;
	scytheThrowKeyframes_[2].time = 0.42f;
	scytheThrowKeyframes_[3].time = kScytheReleaseTime;
	scytheThrowKeyframes_[4].time = 0.82f;
	scytheThrowKeyframes_[5].time = 1.55f;
	scytheThrowKeyframes_[6].time = kScytheCatchTime;
	scytheThrowKeyframes_[7].time = kScytheThrowDuration;

	// First transfer the shaft into the throwing-side hand. The support arm
	// folds toward the chest instead of beginning the normal two-handed slash.
	AttackPose& gripTransfer = scytheThrowKeyframes_[1].pose;
	gripTransfer.rotationOffsets[kBody].z = 0.04f;
	gripTransfer.rotationOffsets[kChest].z = 0.10f;
	gripTransfer.rotationOffsets[kLeftShoulder].z = 0.12f;
	gripTransfer.rotationOffsets[kLeftElbow].z = 2.15f;
	gripTransfer.rotationOffsets[kRightShoulder].z = 0.55f;
	gripTransfer.rotationOffsets[kRightElbow].z = -0.35f;

	// Rear-loaded one-handed wind-up. Rotating the right shoulder from pi toward
	// 2pi makes its naturally left-facing arm travel over the head to release.
	AttackPose& windUp = scytheThrowKeyframes_[2].pose;
	windUp.translationOffsets[kRoot].x = 0.10f;
	windUp.translationOffsets[kBody].y = -0.06f;
	windUp.rotationOffsets[kBody].z = 0.16f;
	windUp.rotationOffsets[kChest].z = 0.34f;
	windUp.rotationOffsets[kNeck].z = -0.10f;
	windUp.rotationOffsets[kHead].z = -0.16f;
	windUp.rotationOffsets[kLeftShoulder].z = 0.30f;
	windUp.rotationOffsets[kLeftElbow].z = 2.42f;
	windUp.rotationOffsets[kRightShoulder].z = 3.25f;
	windUp.rotationOffsets[kRightElbow].z = -1.10f;

	// Lead with the cloak and chest, straighten the throwing arm, and release.
	AttackPose& release = scytheThrowKeyframes_[3].pose;
	release.translationOffsets[kRoot].x = -0.16f;
	release.rotationOffsets[kBody].z = -0.14f;
	release.rotationOffsets[kChest].z = -0.30f;
	release.rotationOffsets[kNeck].z = 0.08f;
	release.rotationOffsets[kHead].z = 0.14f;
	release.rotationOffsets[kLeftShoulder].z = 0.18f;
	release.rotationOffsets[kLeftElbow].z = 0.42f;
	release.rotationOffsets[kRightShoulder].z = 6.05f;
	release.rotationOffsets[kRightElbow].z = 0.02f;

	// The empty throwing hand continues past release while the free arm opens in
	// the opposite direction to balance the blade-heavy weapon's momentum.
	AttackPose& followThrough = scytheThrowKeyframes_[4].pose;
	followThrough.translationOffsets[kRoot].x = -0.22f;
	followThrough.rotationOffsets[kBody].z = -0.20f;
	followThrough.rotationOffsets[kChest].z = -0.40f;
	followThrough.rotationOffsets[kNeck].z = 0.12f;
	followThrough.rotationOffsets[kHead].z = 0.20f;
	followThrough.rotationOffsets[kLeftShoulder].z = 0.32f;
	followThrough.rotationOffsets[kLeftElbow].z = 0.18f;
	followThrough.rotationOffsets[kRightShoulder].z = 6.38f;
	followThrough.rotationOffsets[kRightElbow].z = 0.28f;

	// Recover upright and raise the throwing hand to track the returning scythe.
	AttackPose& trackReturn = scytheThrowKeyframes_[5].pose;
	trackReturn.rotationOffsets[kBody].z = -0.04f;
	trackReturn.rotationOffsets[kChest].z = -0.10f;
	trackReturn.rotationOffsets[kHead].z = 0.06f;
	trackReturn.rotationOffsets[kLeftShoulder].z = 0.12f;
	trackReturn.rotationOffsets[kLeftElbow].z = 1.72f;
	trackReturn.rotationOffsets[kRightShoulder].z = 5.98f;
	trackReturn.rotationOffsets[kRightElbow].z = -0.22f;

	// Catch with the throwing arm extended, then let the elbow and torso absorb
	// the returning weapon before the support hand takes the shaft again.
	AttackPose& catchPose = scytheThrowKeyframes_[6].pose;
	catchPose.translationOffsets[kRoot].x = -0.08f;
	catchPose.rotationOffsets[kBody].z = -0.10f;
	catchPose.rotationOffsets[kChest].z = -0.18f;
	catchPose.rotationOffsets[kHead].z = 0.10f;
	catchPose.rotationOffsets[kLeftShoulder].z = 0.08f;
	catchPose.rotationOffsets[kLeftElbow].z = 1.80f;
	catchPose.rotationOffsets[kRightShoulder].z = 6.12f;
	catchPose.rotationOffsets[kRightElbow].z = 0.05f;

	// End on an equivalent full rotation to prevent a visible shoulder snap.
	scytheThrowKeyframes_[7].pose.rotationOffsets[kRightShoulder].z = 2.0f * std::numbers::pi_v<float>;
}

void BossArmature::InitializeSpinAttackClip() {
	for (AttackKeyframe& keyframe : spinAttackKeyframes_) { keyframe = {}; }
	spinAttackKeyframes_[0].time = 0.00f;
	spinAttackKeyframes_[1].time = 0.28f;
	spinAttackKeyframes_[2].time = 0.48f;
	spinAttackKeyframes_[3].time = 0.72f;
	spinAttackKeyframes_[4].time = 0.96f;
	spinAttackKeyframes_[5].time = 1.20f;
	spinAttackKeyframes_[6].time = 1.44f;
	spinAttackKeyframes_[7].time = kSpinAttackDuration;

	// Coil low and pull the scythe behind the body.
	AttackPose& windUp = spinAttackKeyframes_[1].pose;
	windUp.translationOffsets[kBody].y = -0.10f;
	windUp.rotationOffsets[kBody].z = -0.12f;
	windUp.rotationOffsets[kChest].z = -0.25f;
	windUp.rotationOffsets[kNeck].z = 0.10f;
	windUp.rotationOffsets[kHead].z = 0.14f;
	windUp.rotationOffsets[kLeftShoulder].z = -0.55f;
	windUp.rotationOffsets[kLeftElbow].z = 1.15f;
	windUp.rotationOffsets[kRightShoulder].z = 2.50f;
	windUp.rotationOffsets[kRightElbow].z = -0.95f;

	// Keep both hands and the scythe extended to one side while the entire boss
	// yaws around the vertical Y axis. This is a horizontal Zelda-style sweep,
	// not a windmill rotation in the screen's XY plane.
	auto setSpinPose = [this](std::size_t index, float yaw, float verticalOffset) {
		AttackPose& pose = spinAttackKeyframes_[index].pose;
		pose.translationOffsets[kRoot].y = verticalOffset;
		pose.rotationOffsets[kRoot].y = yaw;
		pose.rotationOffsets[kLeftShoulder].z = 0.08f;
		pose.rotationOffsets[kLeftElbow].z = 0.10f;
		pose.rotationOffsets[kRightShoulder].z = std::numbers::pi_v<float> + 0.08f;
		pose.rotationOffsets[kRightElbow].z = -0.10f;
	};
	setSpinPose(2, 0.0f, 0.00f);
	setSpinPose(3, 0.5f * std::numbers::pi_v<float>, 0.04f);
	setSpinPose(4, std::numbers::pi_v<float>, -0.03f);
	setSpinPose(5, 1.5f * std::numbers::pi_v<float>, 0.04f);
	setSpinPose(6, 2.0f * std::numbers::pi_v<float>, 0.00f);

	// 2pi is visually identical to the idle root rotation. The right shoulder
	// also finishes at 2pi while releasing the one-sided two-handed grip.
	spinAttackKeyframes_[7].pose.rotationOffsets[kRoot].y = 2.0f * std::numbers::pi_v<float>;
	spinAttackKeyframes_[7].pose.rotationOffsets[kRightShoulder].z = 2.0f * std::numbers::pi_v<float>;
}

void BossArmature::InitializeVerticalHookClip() {
	for (AttackKeyframe& keyframe : verticalHookKeyframes_) { keyframe = {}; }
	verticalHookKeyframes_[0].time = 0.00f;
	verticalHookKeyframes_[1].time = 0.32f;
	verticalHookKeyframes_[2].time = 0.58f;
	verticalHookKeyframes_[3].time = 0.82f;
	verticalHookKeyframes_[4].time = 1.06f;
	verticalHookKeyframes_[5].time = kVerticalHookDuration;

	// Keep the hand motion deliberately simple. The right arm points toward the
	// player while the left arm only folds enough to let go of the shaft.
	AttackPose& prepare = verticalHookKeyframes_[1].pose;
	prepare.translationOffsets[kRoot].x = 0.03f;
	prepare.rotationOffsets[kLeftShoulder].z = 0.06f;
	prepare.rotationOffsets[kLeftElbow].z = 1.20f;
	prepare.rotationOffsets[kRightShoulder].z = -0.10f;
	prepare.rotationOffsets[kRightElbow].z = -0.18f;

	// Release with the throwing arm straight. The supporting hand stays near the
	// chest and does not perform a second, competing swing.
	AttackPose& release = verticalHookKeyframes_[2].pose;
	release.translationOffsets[kRoot].x = -0.04f;
	release.rotationOffsets[kLeftShoulder].z = 0.08f;
	release.rotationOffsets[kLeftElbow].z = 1.60f;
	release.rotationOffsets[kRightShoulder].z = 0.02f;
	release.rotationOffsets[kRightElbow].z = 0.00f;

	// Pull with one shallow elbow bend. Keeping this well below a full fold stops
	// the lower-arm model from crossing through the boss.
	AttackPose& pull = verticalHookKeyframes_[3].pose;
	pull.translationOffsets[kRoot].x = 0.05f;
	pull.rotationOffsets[kLeftShoulder].z = 0.08f;
	pull.rotationOffsets[kLeftElbow].z = 1.60f;
	pull.rotationOffsets[kRightShoulder].z = 0.02f;
	pull.rotationOffsets[kRightElbow].z = 0.85f;

	// Relax both elbows smoothly before returning to the unchanged idle pose.
	AttackPose& recoil = verticalHookKeyframes_[4].pose;
	recoil.rotationOffsets[kLeftShoulder].z = 0.04f;
	recoil.rotationOffsets[kLeftElbow].z = 0.75f;
	recoil.rotationOffsets[kRightShoulder].z = 0.01f;
	recoil.rotationOffsets[kRightElbow].z = 0.40f;
}

void BossArmature::StartAnimation(AnimationType animation) {
	ClearIdlePose();
	if (activeAnimation_ != AnimationType::kNone) {
		for (uint32_t index = 0; index < kJointCount; ++index) {
			joints_[index].translation = animationBaseTranslations_[index];
			joints_[index].rotation = animationBaseRotations_[index];
		}
	}
	actionTargetPosition_ = playerTargetPosition_;
	FacePlayer();
	for (uint32_t index = 0; index < kJointCount; ++index) {
		animationBaseTranslations_[index] = joints_[index].translation;
		animationBaseRotations_[index] = joints_[index].rotation;
	}
	activeAnimation_ = animation;
	animationTime_ = 0.0f;
	isScytheDetached_ = false;
	useExplicitScythePose_ = false;
	hasScytheReleaseCenter_ = false;
	pauseAnimation_ = false;
	showDebugScythe_ = true;
}

void BossArmature::StopAnimation() {
	if (activeAnimation_ == AnimationType::kNone) { return; }
	for (uint32_t index = 0; index < kJointCount; ++index) {
		joints_[index].translation = animationBaseTranslations_[index];
		joints_[index].rotation = animationBaseRotations_[index];
	}
	activeAnimation_ = AnimationType::kNone;
	animationTime_ = 0.0f;
	isScytheDetached_ = false;
	useExplicitScythePose_ = false;
	hasScytheReleaseCenter_ = false;
	pauseAnimation_ = false;
}

void BossArmature::UpdateAnimation() {
	if (activeAnimation_ == AnimationType::kNone) { return; }

	const AttackKeyframe* keyframes = nullptr;
	std::size_t keyframeCount = 0;
	switch (activeAnimation_) {
	case AnimationType::kNormalAttack:
		keyframes = normalAttackKeyframes_.data();
		keyframeCount = normalAttackKeyframes_.size();
		break;
	case AnimationType::kScytheThrow:
		keyframes = scytheThrowKeyframes_.data();
		keyframeCount = scytheThrowKeyframes_.size();
		break;
	case AnimationType::kSpinAttack:
		keyframes = spinAttackKeyframes_.data();
		keyframeCount = spinAttackKeyframes_.size();
		break;
	case AnimationType::kVerticalHook:
		keyframes = verticalHookKeyframes_.data();
		keyframeCount = verticalHookKeyframes_.size();
		break;
	case AnimationType::kNone:
		return;
	}

	const float duration = GetActiveAnimationDuration();
	const float playbackDuration = (std::max)(GetActivePlaybackDuration(), 0.05f);
	const float playbackSpeed = (std::max)(GetActivePlaybackSpeed(), 0.01f);
	if (!pauseAnimation_) {
		const float authoredTimeScale = duration / playbackDuration;
		animationTime_ = (std::min)(
		    animationTime_ + kFrameTime * playbackSpeed * authoredTimeScale, duration);
	}
	std::size_t endIndex = 1;
	while (endIndex < keyframeCount - 1 && animationTime_ > keyframes[endIndex].time) { ++endIndex; }
	const AttackKeyframe& start = keyframes[endIndex - 1];
	const AttackKeyframe& end = keyframes[endIndex];
	const float range = end.time - start.time;
	const float rawT = range > 0.0f ? std::clamp((animationTime_ - start.time) / range, 0.0f, 1.0f) : 1.0f;
	const bool isConstantSpinSegment =
	    activeAnimation_ == AnimationType::kSpinAttack && endIndex >= 3 && endIndex <= 6;
	const float t = isConstantSpinSegment ? rawT : SmoothStep(rawT);
	ApplyAttackPose(start.pose, end.pose, t);

	if (animationTime_ >= duration) {
		if (loopAnimation_) {
			animationTime_ = 0.0f;
			isScytheDetached_ = false;
			useExplicitScythePose_ = false;
			hasScytheReleaseCenter_ = false;
			ApplyAttackPose(keyframes[0].pose, keyframes[1].pose, 0.0f);
			return;
		}
		for (uint32_t index = 0; index < kJointCount; ++index) {
			joints_[index].translation = animationBaseTranslations_[index];
			joints_[index].rotation = animationBaseRotations_[index];
		}
		activeAnimation_ = AnimationType::kNone;
		animationTime_ = 0.0f;
		isScytheDetached_ = false;
		useExplicitScythePose_ = false;
		hasScytheReleaseCenter_ = false;
		pauseAnimation_ = false;
	}
}

void BossArmature::UpdateScytheState() {
	isScytheDetached_ = false;
	useExplicitScythePose_ = false;
	if (activeAnimation_ == AnimationType::kVerticalHook) {
		constexpr float kHookLaunchTime = 0.32f;
		constexpr float kHookFullExtensionTime = 0.58f;
		constexpr float kHookRetractTime = 0.70f;
		constexpr float kHookRegripTime = 1.06f;
		if (animationTime_ < kHookLaunchTime) {
			hasScytheReleaseCenter_ = false;
			return;
		}

		useExplicitScythePose_ = true;
		const Vector3& gripA = joints_[kRightHand].worldPosition;
		const Vector3& gripB = joints_[kLeftHand].worldPosition;
		const Vector3 twoHandCenter = {
		    (gripA.x + gripB.x) * 0.5f,
		    (gripA.y + gripB.y) * 0.5f,
		    (gripA.z + gripB.z) * 0.5f,
		};
		if (!hasScytheReleaseCenter_) {
			scytheReleaseCenter_ = twoHandCenter;
			const Vector3 desiredTarget = {
			    actionTargetPosition_.x,
			    actionTargetPosition_.y + verticalHookTargetYOffset_,
			    joints_[kRoot].worldPosition.z,
			};
			const Vector3 targetOffset = {
			    desiredTarget.x - scytheReleaseCenter_.x,
			    desiredTarget.y - scytheReleaseCenter_.y,
			    desiredTarget.z - scytheReleaseCenter_.z,
			};
			const float targetDistance = std::sqrt(
			    targetOffset.x * targetOffset.x + targetOffset.y * targetOffset.y + targetOffset.z * targetOffset.z);
			if (targetDistance > 0.001f) {
				const float travelDistance = (std::min)(targetDistance, verticalHookReach_);
				scytheTargetCenter_ = {
				    scytheReleaseCenter_.x + targetOffset.x / targetDistance * travelDistance,
				    scytheReleaseCenter_.y + targetOffset.y / targetDistance * travelDistance,
				    scytheReleaseCenter_.z + targetOffset.z / targetDistance * travelDistance,
				};
			} else {
				scytheTargetCenter_ = scytheReleaseCenter_;
			}
			hasScytheReleaseCenter_ = true;
		}

		if (animationTime_ <= kHookFullExtensionTime) {
			const float extension = SmoothStep(
			    (animationTime_ - kHookLaunchTime) / (kHookFullExtensionTime - kHookLaunchTime));
			explicitScytheCenter_ = Lerp(scytheReleaseCenter_, scytheTargetCenter_, extension);
			// Turn the overhead shaft so its hook points down at maximum range.
			explicitScytheRotation_ =
			    0.5f * std::numbers::pi_v<float> + extension * std::numbers::pi_v<float>;
		} else if (animationTime_ <= kHookRetractTime) {
			// Hold briefly at the future collision position.
			explicitScytheCenter_ = scytheTargetCenter_;
			explicitScytheRotation_ = 1.5f * std::numbers::pi_v<float>;
		} else if (animationTime_ <= kHookRegripTime) {
			const float retraction = SmoothStep(
			    (animationTime_ - kHookRetractTime) / (kHookRegripTime - kHookRetractTime));
			explicitScytheCenter_ = Lerp(scytheTargetCenter_, twoHandCenter, retraction);
			explicitScytheRotation_ = 1.5f * std::numbers::pi_v<float>;
		} else {
			const float regrip = SmoothStep(std::clamp(
			    (animationTime_ - kHookRegripTime) / (kVerticalHookDuration - kHookRegripTime), 0.0f, 1.0f));
			explicitScytheCenter_ = twoHandCenter;
			explicitScytheRotation_ =
			    1.5f * std::numbers::pi_v<float> + 0.5f * std::numbers::pi_v<float> * regrip;
		}
		return;
	}
	if (activeAnimation_ != AnimationType::kScytheThrow) {
		hasScytheReleaseCenter_ = false;
		return;
	}

	useExplicitScythePose_ = true;
	const Vector3& throwingHand = joints_[kRightHand].worldPosition;
	const Vector3& supportHand = joints_[kLeftHand].worldPosition;
	const Vector3 twoHandCenter = {
	    (throwingHand.x + supportHand.x) * 0.5f,
	    (throwingHand.y + supportHand.y) * 0.5f,
	    (throwingHand.z + supportHand.z) * 0.5f,
	};
	const float twoHandAngle = std::atan2(supportHand.y - throwingHand.y, supportHand.x - throwingHand.x);

	constexpr float kGripTransferTime = 0.18f;
	constexpr float kWindUpTime = 0.42f;
	const float windUpAngle = facingDirection_ < 0.0f ? 1.35f : std::numbers::pi_v<float> - 1.35f;
	const float releaseAngle = facingDirection_ < 0.0f ? 0.10f : std::numbers::pi_v<float> - 0.10f;
	if (animationTime_ < kScytheReleaseTime) {
		if (animationTime_ <= kGripTransferTime) {
			const float transferProgress = SmoothStep(animationTime_ / kGripTransferTime);
			explicitScytheCenter_ = Lerp(twoHandCenter, throwingHand, transferProgress);
			explicitScytheRotation_ = twoHandAngle;
		} else if (animationTime_ <= kWindUpTime) {
			const float windUpProgress = SmoothStep(
			    (animationTime_ - kGripTransferTime) / (kWindUpTime - kGripTransferTime));
			explicitScytheCenter_ = throwingHand;
			explicitScytheRotation_ = twoHandAngle + (windUpAngle - twoHandAngle) * windUpProgress;
		} else {
			const float releaseProgress = SmoothStep(
			    (animationTime_ - kWindUpTime) / (kScytheReleaseTime - kWindUpTime));
			explicitScytheCenter_ = throwingHand;
			explicitScytheRotation_ = windUpAngle + (releaseAngle - windUpAngle) * releaseProgress;
		}
		return;
	}

	if (animationTime_ <= kScytheCatchTime) {
		isScytheDetached_ = true;
		if (!hasScytheReleaseCenter_) {
			scytheReleaseCenter_ = throwingHand;
			const Vector3 aimedPoint = {
			    actionTargetPosition_.x,
			    actionTargetPosition_.y + scytheThrowTargetYOffset_,
			    throwingHand.z,
			};
			const Vector3 aimOffset = {
			    aimedPoint.x - throwingHand.x,
			    aimedPoint.y - throwingHand.y,
			    aimedPoint.z - throwingHand.z,
			};
			const float aimLength = std::sqrt(
			    aimOffset.x * aimOffset.x + aimOffset.y * aimOffset.y + aimOffset.z * aimOffset.z);
			scytheFlightDirection_ = aimLength > 0.001f
			                              ? Vector3{aimOffset.x / aimLength, aimOffset.y / aimLength, aimOffset.z / aimLength}
			                              : Vector3{facingDirection_, 0.0f, 0.0f};
			hasScytheReleaseCenter_ = true;
		}

		const float flightProgress = std::clamp(
		    (animationTime_ - kScytheReleaseTime) / (kScytheCatchTime - kScytheReleaseTime), 0.0f, 1.0f);
		const Vector3 movingAnchor = Lerp(scytheReleaseCenter_, throwingHand, flightProgress);
		const float travelArc = std::sin(flightProgress * std::numbers::pi_v<float>);
		explicitScytheCenter_ = {
		    movingAnchor.x + scytheFlightDirection_.x * scytheThrowRange_ * travelArc,
		    movingAnchor.y + scytheFlightDirection_.y * scytheThrowRange_ * travelArc + scytheThrowArcHeight_ * travelArc + 0.22f * std::sin(flightProgress * 2.0f * std::numbers::pi_v<float>),
		    movingAnchor.z + scytheFlightDirection_.z * scytheThrowRange_ * travelArc,
		};
		explicitScytheRotation_ =
		    releaseAngle + flightProgress * scytheThrowSpinCount_ * 2.0f * std::numbers::pi_v<float>;
		return;
	}

	// Move the caught weapon from the throwing hand back into a two-handed grip.
	const float regripProgress = SmoothStep(std::clamp(
	    (animationTime_ - kScytheCatchTime) / (kScytheThrowDuration - kScytheCatchTime), 0.0f, 1.0f));
	explicitScytheCenter_ = Lerp(throwingHand, twoHandCenter, regripProgress);
	explicitScytheRotation_ = releaseAngle + (twoHandAngle - releaseAngle) * regripProgress;
}

float BossArmature::GetActiveAnimationDuration() const {
	switch (activeAnimation_) {
	case AnimationType::kNormalAttack:
		return kNormalAttackDuration;
	case AnimationType::kScytheThrow:
		return kScytheThrowDuration;
	case AnimationType::kSpinAttack:
		return kSpinAttackDuration;
	case AnimationType::kVerticalHook:
		return kVerticalHookDuration;
	case AnimationType::kNone:
		return 0.0f;
	}
	return 0.0f;
}

float BossArmature::GetActivePlaybackDuration() const {
	switch (activeAnimation_) {
	case AnimationType::kNormalAttack:
		return normalAttackPlaybackDuration_;
	case AnimationType::kScytheThrow:
		return scytheThrowPlaybackDuration_;
	case AnimationType::kSpinAttack:
		return spinAttackPlaybackDuration_;
	case AnimationType::kVerticalHook:
		return verticalHookPlaybackDuration_;
	case AnimationType::kNone:
		return 0.0f;
	}
	return 0.0f;
}

float BossArmature::GetActivePlaybackSpeed() const {
	switch (activeAnimation_) {
	case AnimationType::kNormalAttack:
		return normalAttackPlaybackSpeed_;
	case AnimationType::kScytheThrow:
		return scytheThrowPlaybackSpeed_;
	case AnimationType::kSpinAttack:
		return spinAttackPlaybackSpeed_;
	case AnimationType::kVerticalHook:
		return verticalHookPlaybackSpeed_;
	case AnimationType::kNone:
		return 1.0f;
	}
	return 1.0f;
}

const char* BossArmature::GetActiveAnimationName() const {
	switch (activeAnimation_) {
	case AnimationType::kNormalAttack:
		return "Normal Attack";
	case AnimationType::kScytheThrow:
		return "Scythe Throw";
	case AnimationType::kSpinAttack:
		return "Spin Attack";
	case AnimationType::kVerticalHook:
		return "Vertical Hook";
	case AnimationType::kNone:
		return "None";
	}
	return "None";
}

const char* BossArmature::GetAIStateName() const {
	switch (aiState_) {
	case AIState::kWaiting:
		return "Waiting";
	case AIState::kMeleeAttack:
		return "Melee Attack";
	case AIState::kRetreat:
		return "Retreat";
	case AIState::kSpinAttack:
		return "Spin Attack";
	case AIState::kVerticalHook:
		return "Vertical Hook";
	case AIState::kScytheThrow:
		return "Scythe Throw";
	}
	return "Unknown";
}

const char* BossArmature::GetDistanceBandName() const {
	if (playerDistance_ < closeDistance_) { return "Close"; }
	if (playerDistance_ < midDistance_) { return "Mid"; }
	return "Far";
}

void BossArmature::ApplyAttackPose(const AttackPose& start, const AttackPose& end, float t) {
	for (uint32_t index = 0; index < kJointCount; ++index) {
		const float rootHorizontalScale = index == static_cast<uint32_t>(kRoot) ? -facingDirection_ : 1.0f;
		joints_[index].translation = Lerp(
		    {animationBaseTranslations_[index].x + start.translationOffsets[index].x * rootHorizontalScale, animationBaseTranslations_[index].y + start.translationOffsets[index].y, animationBaseTranslations_[index].z + start.translationOffsets[index].z},
		    {animationBaseTranslations_[index].x + end.translationOffsets[index].x * rootHorizontalScale, animationBaseTranslations_[index].y + end.translationOffsets[index].y, animationBaseTranslations_[index].z + end.translationOffsets[index].z}, t);
		const float spinTurnScale = activeAnimation_ == AnimationType::kSpinAttack && index == static_cast<uint32_t>(kRoot)
		                                ? static_cast<float>(spinAttackTurnCount_)
		                                : 1.0f;
		joints_[index].rotation = Lerp(
		    {animationBaseRotations_[index].x + start.rotationOffsets[index].x, animationBaseRotations_[index].y + start.rotationOffsets[index].y * spinTurnScale, animationBaseRotations_[index].z + start.rotationOffsets[index].z},
		    {animationBaseRotations_[index].x + end.rotationOffsets[index].x, animationBaseRotations_[index].y + end.rotationOffsets[index].y * spinTurnScale, animationBaseRotations_[index].z + end.rotationOffsets[index].z}, t);
	}
}

void BossArmature::DrawDebugScythe(const Camera& camera) {
	PrimitiveDrawer* drawer = PrimitiveDrawer::GetInstance();
	drawer->SetCamera(&camera);
	Vector3 gripA = joints_[kRightHand].worldPosition;
	Vector3 gripB = joints_[kLeftHand].worldPosition;
	if (activeAnimation_ == AnimationType::kVerticalHook && useExplicitScythePose_) {
		const Vector3 tetherStart = {
		    (gripA.x + gripB.x) * 0.5f,
		    (gripA.y + gripB.y) * 0.5f,
		    (gripA.z + gripB.z) * 0.5f,
		};
		drawer->DrawLine3d(tetherStart, explicitScytheCenter_, {0.55f, 0.55f, 0.60f, 1.0f});
	}
	if (useExplicitScythePose_) {
		const Vector3 explicitAxis = {std::cos(explicitScytheRotation_), std::sin(explicitScytheRotation_), 0.0f};
		gripA = {
		    explicitScytheCenter_.x - explicitAxis.x * 0.5f,
		    explicitScytheCenter_.y - explicitAxis.y * 0.5f,
		    explicitScytheCenter_.z,
		};
		gripB = {
		    explicitScytheCenter_.x + explicitAxis.x * 0.5f,
		    explicitScytheCenter_.y + explicitAxis.y * 0.5f,
		    explicitScytheCenter_.z,
		};
	}
	const Vector3 difference = {gripB.x - gripA.x, gripB.y - gripA.y, gripB.z - gripA.z};
	const float length = std::sqrt(difference.x * difference.x + difference.y * difference.y + difference.z * difference.z);
	if (length <= 0.001f) { return; }
	const Vector3 axis = {difference.x / length, difference.y / length, difference.z / length};
	Vector3 perpendicular = {-axis.y, axis.x, 0.0f};
	const float perpendicularLength = std::sqrt(
	    perpendicular.x * perpendicular.x + perpendicular.y * perpendicular.y + perpendicular.z * perpendicular.z);
	if (perpendicularLength > 0.001f) {
		perpendicular = {
		    perpendicular.x / perpendicularLength,
		    perpendicular.y / perpendicularLength,
		    perpendicular.z / perpendicularLength,
		};
	} else {
		// During the horizontal spin the handle briefly points along the camera's
		// depth axis. Keep the blade vertical instead of letting it collapse.
		perpendicular = {0.0f, 1.0f, 0.0f};
	}
	const Vector3 handleStart = {gripA.x - axis.x * 0.8f, gripA.y - axis.y * 0.8f, gripA.z - axis.z * 0.8f};
	const Vector3 handleEnd = {gripB.x + axis.x * 2.3f, gripB.y + axis.y * 2.3f, gripB.z + axis.z * 2.3f};
	drawer->DrawLine3d(handleStart, handleEnd, {0.38f, 0.20f, 0.08f, 1.0f});

	Vector3 previous = handleEnd;
	for (int segment = 1; segment <= 8; ++segment) {
		const float t = static_cast<float>(segment) / 8.0f;
		const float curve = std::sin(t * std::numbers::pi_v<float> * 0.75f);
		const Vector3 point = {
		    handleEnd.x - axis.x * (1.8f * t) + perpendicular.x * (2.2f * curve),
		    handleEnd.y - axis.y * (1.8f * t) + perpendicular.y * (2.2f * curve),
		    handleEnd.z - axis.z * (1.8f * t),
		};
		drawer->DrawLine3d(previous, point, {0.80f, 0.85f, 0.90f, 1.0f});
		previous = point;
	}
}

Vector3 BossArmature::Lerp(const Vector3& start, const Vector3& end, float t) {
	return {start.x + (end.x - start.x) * t, start.y + (end.y - start.y) * t, start.z + (end.z - start.z) * t};
}

float BossArmature::SmoothStep(float t) {
	t = std::clamp(t, 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}

BossArmature::~BossArmature() {
	for (ModelPart& part : modelParts_) { delete part.model; part.model = nullptr; }
	delete jointSphereModel_;
	jointSphereModel_ = nullptr;
}
