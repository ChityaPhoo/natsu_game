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
	InitializeJoint(kRoot, "Root", -1, {kInitialBossX, 1.0f, 0.040f}, {1.0f, 1.0f, 0.0f, 1.0f});
	InitializeJoint(kBody, "Body", kRoot, {0.0f, 2.80f, 0.0f}, {1.0f, 0.55f, 0.0f, 1.0f});
	InitializeJoint(kChest, "Chest", kBody, {0.0f, 1.962f, 0.0f}, {1.0f, 0.30f, 0.0f, 1.0f});
	InitializeJoint(kNeck, "Neck", kChest, {0.0f, 0.717f, 0.0f}, {0.0f, 0.75f, 1.0f, 1.0f});
	InitializeJoint(kHead, "Head", kNeck, {0.0f, 0.470f, 0.0f}, {0.0f, 0.35f, 1.0f, 1.0f});
	InitializeJoint(kLeftShoulder, "Left Shoulder", kChest, {0.020f, -0.593f, 1.130f}, {1.0f, 0.0f, 0.0f, 1.0f});
	InitializeJoint(kLeftElbow, "Left Elbow", kLeftShoulder, {0.001f, -0.920f, 0.516f}, {0.0f, 1.0f, 0.0f, 1.0f});
	InitializeJoint(kLeftHand, "Left Hand", kLeftElbow, {0.210f, -1.190f, 0.630f}, {0.0f, 0.35f, 1.0f, 1.0f});
	InitializeJoint(kRightShoulder, "Right Shoulder", kChest, {-0.260f, -0.593f, -1.056f}, {1.0f, 0.0f, 0.0f, 1.0f});
	InitializeJoint(kRightElbow, "Right Elbow", kRightShoulder, {0.001f, -0.790f, -0.770f}, {0.0f, 1.0f, 0.0f, 1.0f});
	InitializeJoint(kRightHand, "Right Hand", kRightElbow, {-1.499f, -1.210f, -0.610f}, {0.0f, 0.35f, 1.0f, 1.0f});

	// Original imported-model alignment pose. This is only the mesh bind
	// reference; animation and idle use the newer pose below.
	joints_[kRoot].rotation = {0.0f, -0.410f, 0.0f};
	joints_[kLeftShoulder].rotation = {0.0f, 0.0f, -0.670f};
	joints_[kLeftShoulder].scale = {0.700f, 0.700f, 0.700f};
	joints_[kLeftElbow].rotation = {0.0f, 0.0f, -0.080f};
	joints_[kRightShoulder].rotation = {0.0f, 0.0f, 0.540f};
	joints_[kRightShoulder].scale = {0.700f, 0.700f, 0.700f};
	joints_[kRightElbow].rotation = {-0.220f, -0.240f, 0.960f};
	for (uint32_t index = 0; index < kJointCount; ++index) {
		const Matrix4x4 localBind = Matrix4x4Calculation::MakeAffineMatrix(
		    joints_[index].scale, joints_[index].rotation, joints_[index].translation);
		modelBindJointWorldMatrices_[index] = joints_[index].parentIndex < 0
		                                                ? localBind
		                                                : Matrix4x4Calculation::Multiply(
		                                                      localBind,
		                                                      modelBindJointWorldMatrices_[static_cast<uint32_t>(joints_[index].parentIndex)]);
	}

	// New authored idle/default pose. Animations are offset from these values,
	// while the arm renderer deforms from the immutable model bind above.
	joints_[kBody].rotation = {0.0f, 0.0f, 0.200f};
	joints_[kLeftShoulder].rotation = {0.0f, 0.0f, -0.950f};
	joints_[kLeftElbow].rotation = {0.0f, 0.0f, -1.270f};
	joints_[kRightElbow].rotation = {-0.220f, -0.240f, -0.630f};

	// Store the exact bind SRT and its world matrices. The articulated OBJ mesh
	// groups use these matrices as inverse-bind references.
	for (uint32_t index = 0; index < kJointCount; ++index) {
		defaultTranslations_[index] = joints_[index].translation;
		defaultRotations_[index] = joints_[index].rotation;
		defaultScales_[index] = joints_[index].scale;
		const Matrix4x4 localBind = Matrix4x4Calculation::MakeAffineMatrix(
		    defaultScales_[index], defaultRotations_[index], defaultTranslations_[index]);
		defaultJointWorldMatrices_[index] = joints_[index].parentIndex < 0
		                                         ? localBind
		                                         : Matrix4x4Calculation::Multiply(
		                                               localBind,
		                                               defaultJointWorldMatrices_[static_cast<uint32_t>(joints_[index].parentIndex)]);
	}
	InitializeNormalAttackClip();
	InitializeScytheThrowClip();
	InitializeSpinAttackClip();
	InitializeVerticalHookClip();
	InitializeJumpSlamClip();
	InitializePhaseTwoUppercutClip();
	InitializePhaseTwoGroundWaveClip();
	InitializePhaseTwoPillarsClip();

	// The float_* resources are authored around their own local origins. Attach
	// their useful pivots to the existing armature instead of treating their OBJ
	// coordinates as already assembled world-space geometry.
	InitializeLocalModelPart(
	    modelParts_[0], "float_Body", kBody, {0.0f, 0.138622f, 0.0f},
	    {2.50f, 2.50f, 2.50f}, {0.0f, -1.80f, 0.0f});
	InitializeLocalModelPart(
	    modelParts_[1], "float_Head", kHead, {0.0f, 0.097571f, 0.0f},
	    {1.80f, 1.80f, 1.80f}, {0.0f, -1.10f, 0.0f});
	InitializeLocalModelPart(
	    // The folder names are reversed: float_R_arm contains Palm_L.
	    modelParts_[2], "float_R_arm", kLeftShoulder, {0.0154f, 0.0404f, 0.1050f},
	    {3.00f, 3.00f, 3.00f}, {}, 1.0f);
	InitializeLocalModelPart(
	    // The folder names are reversed: float_L_arm contains Palm_R.
	    modelParts_[3], "float_L_arm", kRightShoulder, {0.0158f, 0.0651f, 0.0f},
	    {3.00f, 3.00f, 3.00f}, {}, -1.0f);
	// The model was authored front-on. A quarter-turn gives it a side profile;
	// the root joint's existing 0/PI facing rotation then turns that profile
	// toward whichever side of the boss the player is standing on.
	for (ModelPart& part : modelParts_) {
		part.localRotation.y = -0.5f * std::numbers::pi_v<float>;
		part.defaultLocalRotation = part.localRotation;
	}
	InitializeArticulatedArm(modelParts_[2], "float_R_arm", kLeftElbow, kLeftHand);
	InitializeArticulatedArm(modelParts_[3], "float_L_arm", kRightElbow, kRightHand);
	weaponModel_ = Model::CreateFromOBJ("hammer", true);
	weaponTransform_.Initialize();
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

void BossArmature::InitializeLocalModelPart(
	ModelPart& part, const char* modelName, JointIndex joint, const Vector3& sourcePivot,
	const Vector3& localScale, const Vector3& jointOffset, float idleShoulderCompensationSign) {
	part.model = Model::CreateFromOBJ(modelName, true);
	part.worldTransform.Initialize();
	part.joint = joint;
	part.sourcePivot = sourcePivot;
	part.localScale = localScale;
	part.jointOffset = jointOffset;
	part.defaultSourcePivot = sourcePivot;
	part.defaultLocalScale = localScale;
	part.defaultLocalRotation = part.localRotation;
	part.defaultJointOffset = jointOffset;
	part.followsJointSegment = false;
	part.usesLocalAttachment = true;
	part.idleShoulderCompensationSign = idleShoulderCompensationSign;
}

void BossArmature::InitializeArticulatedArm(
    ModelPart& part, const char* modelBaseName, JointIndex elbowJoint, JointIndex handJoint) {
	// KamataEngine merges all OBJ object groups that share one material into a
	// single Mesh. Load the mechanically split resources so the three arm
	// sections can genuinely receive different armature transforms.
	const std::string baseName = modelBaseName;
	part.articulatedModels[0] = Model::CreateFromOBJ(baseName + "_upper", true);
	part.articulatedModels[1] = Model::CreateFromOBJ(baseName + "_lower", true);
	part.articulatedModels[2] = Model::CreateFromOBJ(baseName + "_palm", true);
	part.articulatedMeshCount = part.articulatedModels.size();

	for (std::size_t pieceIndex = 0; pieceIndex < part.articulatedMeshCount; ++pieceIndex) {
		part.articulatedMeshTransforms[pieceIndex].Initialize();
	}
	part.articulatedMeshJoints[0] = part.joint;
	part.articulatedMeshEndJoints[0] = elbowJoint;
	part.articulatedMeshJoints[1] = elbowJoint;
	part.articulatedMeshEndJoints[1] = handJoint;
	part.articulatedMeshJoints[2] = handJoint;
	part.articulatedMeshEndJoints[2] = kJointCount;
	for (std::size_t pieceIndex = 0; pieceIndex < part.articulatedMeshCount; ++pieceIndex) {
		part.inverseBindJointMatrices[pieceIndex] = Matrix4x4Calculation::Inverse(
		    modelBindJointWorldMatrices_[part.articulatedMeshJoints[pieceIndex]]);
	}
	part.usesArticulatedMeshes = true;
}

void BossArmature::Update(const Vector3& playerPosition) {
	playerTargetPosition_ = playerPosition;
	playerDistance_ = std::abs(playerTargetPosition_.x - defaultTranslations_[kRoot].x);
	// Both editors freeze AI and breathing. The keyframe editor advances only
	// while its explicit preview is playing, so a selected frame stays stable.
	if (controlMode_ == ControlMode::kPlayTest) {
		UpdateAI();
		UpdateAnimation();
		UpdateIdleAnimation();
	} else if (controlMode_ == ControlMode::kAnimationDebug) {
		UpdateAnimation();
		UpdateIdleAnimation();
	} else if (controlMode_ == ControlMode::kKeyframeEditor && keyframePreviewPlaying_) {
		UpdateAnimation();
		if (activeAnimation_ == AnimationType::kNone) {
			keyframePreviewPlaying_ = false;
			LoadSelectedKeyframePose();
		}
	}
	UpdatePhaseTwoAttackState();
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
	UpdateWeaponTransform();

	for (ModelPart& part : modelParts_) { UpdateModelPart(part); }
}

void BossArmature::UpdateWeaponTransform() {
	if (weaponModel_ == nullptr) { return; }

	Vector3 center = {};
	Vector3 axis = {};
	if (useExplicitScythePose_) {
		center = explicitScytheCenter_;
		axis = {std::cos(explicitScytheRotation_), std::sin(explicitScytheRotation_), 0.0f};
	} else {
		const Vector3& gripA = joints_[kRightHand].worldPosition;
		const Vector3& gripB = joints_[kLeftHand].worldPosition;
		center = {
		    (gripA.x + gripB.x) * 0.5f,
		    (gripA.y + gripB.y) * 0.5f,
		    (gripA.z + gripB.z) * 0.5f};
		axis = {gripB.x - gripA.x, gripB.y - gripA.y, gripB.z - gripA.z};
	}

	const float axisLength = std::sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
	if (axisLength <= 0.0001f) { return; }
	axis = {axis.x / axisLength, axis.y / axisLength, axis.z / axisLength};

	// The hammer OBJ is modeled along local +Y. Build a stable orthonormal frame
	// whose Y basis follows the weapon direction. This keeps the imported hammer
	// fixed between the two animated hands and also supports the detached moves.
	const Vector3 reference = std::abs(axis.z) < 0.95f ? Vector3{0.0f, 0.0f, 1.0f} : Vector3{0.0f, 1.0f, 0.0f};
	Vector3 side = {
	    axis.y * reference.z - axis.z * reference.y,
	    axis.z * reference.x - axis.x * reference.z,
	    axis.x * reference.y - axis.y * reference.x};
	const float sideLength = std::sqrt(side.x * side.x + side.y * side.y + side.z * side.z);
	side = {side.x / sideLength, side.y / sideLength, side.z / sideLength};
	const Vector3 depth = {
	    side.y * axis.z - side.z * axis.y,
	    side.z * axis.x - side.x * axis.z,
	    side.x * axis.y - side.y * axis.x};

	Matrix4x4 weaponFrame = Matrix4x4Calculation::MakeIdentity4x4();
	weaponFrame.m[0][0] = side.x * weaponModelScale_;
	weaponFrame.m[0][1] = side.y * weaponModelScale_;
	weaponFrame.m[0][2] = side.z * weaponModelScale_;
	weaponFrame.m[1][0] = axis.x * weaponModelScale_;
	weaponFrame.m[1][1] = axis.y * weaponModelScale_;
	weaponFrame.m[1][2] = axis.z * weaponModelScale_;
	weaponFrame.m[2][0] = depth.x * weaponModelScale_;
	weaponFrame.m[2][1] = depth.y * weaponModelScale_;
	weaponFrame.m[2][2] = depth.z * weaponModelScale_;
	weaponFrame.m[3][0] = center.x;
	weaponFrame.m[3][1] = center.y;
	weaponFrame.m[3][2] = center.z;

	// The grip is near Y=2.65 in the authored mesh; the head extends toward +Y.
	const Matrix4x4 sourceGripOffset = Matrix4x4Calculation::MakeTranslateMatrix({0.0f, -2.65f, 0.0f});
	weaponTransform_.matWorld_ = Matrix4x4Calculation::Multiply(sourceGripOffset, weaponFrame);
	weaponTransform_.TransferMatrix();
}

void BossArmature::UpdateModelPart(ModelPart& part) {
	if (part.model == nullptr) { return; }
	if (part.usesArticulatedMeshes) {
		const Matrix4x4 sourcePivotOffset = Matrix4x4Calculation::MakeTranslateMatrix({
		    -part.sourcePivot.x, -part.sourcePivot.y, -part.sourcePivot.z});
		const Matrix4x4 localAdjustment = Matrix4x4Calculation::MakeAffineMatrix(
		    part.localScale, part.localRotation, part.jointOffset);
		const Matrix4x4 modelBindWorld = Matrix4x4Calculation::Multiply(
		    Matrix4x4Calculation::Multiply(sourcePivotOffset, localAdjustment),
		    modelBindJointWorldMatrices_[part.joint]);
		for (std::size_t meshIndex = 0; meshIndex < part.articulatedMeshCount; ++meshIndex) {
			const JointIndex startJoint = part.articulatedMeshJoints[meshIndex];
			const JointIndex endJoint = part.articulatedMeshEndJoints[meshIndex];
			Matrix4x4 jointDelta = {};
			if (endJoint != kJointCount) {
				// Deform each arm section between its two armature points. Unlike a
				// rigid joint attachment, this guarantees that the model elbow and
				// hand reach the animated elbow/hand even when the bone length or
				// direction differs from the imported mesh's authored pose.
				const Vector3 bindStart = Matrix4x4Calculation::TransformPoint(
				    {}, modelBindJointWorldMatrices_[startJoint]);
				const Vector3 bindEnd = Matrix4x4Calculation::TransformPoint(
				    {}, modelBindJointWorldMatrices_[endJoint]);
				const Vector3 currentStart = joints_[startJoint].worldPosition;
				const Vector3 currentEnd = joints_[endJoint].worldPosition;
				auto makeSegmentFrame = [](const Vector3& start, const Vector3& end) {
					const Vector3 difference = {
					    end.x - start.x, end.y - start.y, end.z - start.z};
					const float length = (std::max)(std::sqrt(
					    difference.x * difference.x + difference.y * difference.y +
					    difference.z * difference.z), 0.0001f);
					const float horizontalLength = std::sqrt(
					    difference.x * difference.x + difference.y * difference.y);
					const Vector3 rotation = {
					    0.0f, -std::atan2(difference.z, horizontalLength),
					    std::atan2(difference.y, difference.x)};
					return Matrix4x4Calculation::MakeAffineMatrix(
					    {length, 1.0f, 1.0f}, rotation, start);
				};
				const Matrix4x4 bindFrame = makeSegmentFrame(bindStart, bindEnd);
				const Matrix4x4 currentFrame = makeSegmentFrame(currentStart, currentEnd);
				jointDelta = Matrix4x4Calculation::Multiply(
				    Matrix4x4Calculation::Inverse(bindFrame), currentFrame);
			} else {
				// The palm is a rigid end piece; its entire transform follows the
				// hand joint after the forearm has been fitted to elbow -> hand.
				jointDelta = Matrix4x4Calculation::Multiply(
				    part.inverseBindJointMatrices[meshIndex], joints_[startJoint].worldMatrix);
			}
			WorldTransform& meshTransform = part.articulatedMeshTransforms[meshIndex];
			meshTransform.matWorld_ = Matrix4x4Calculation::Multiply(modelBindWorld, jointDelta);
			meshTransform.TransferMatrix();
		}
		return;
	}

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

	if (part.usesLocalAttachment) {
		Vector3 localRotation = part.localRotation;
		Matrix4x4 attachmentWorld = joints_[part.joint].worldMatrix;
		const bool isNaturalIdle =
		    controlMode_ != ControlMode::kPoseEditor && !phaseTransitionActive_ &&
		    activeAnimation_ == AnimationType::kNone;
		if (isNaturalIdle && part.idleShoulderCompensationSign != 0.0f) {
			// Each float arm is already modeled as one complete, naturally bent arm.
			// Building its idle attachment from the shoulder translation and chest
			// matrix avoids applying the armature's extra shoulder bend a second time.
			const Joint& shoulder = joints_[part.joint];
			const Matrix4x4 shoulderTranslation = Matrix4x4Calculation::MakeAffineMatrix(
			    {1.0f, 1.0f, 1.0f}, {}, shoulder.translation);
			attachmentWorld = shoulder.parentIndex < 0
			                      ? shoulderTranslation
			                      : Matrix4x4Calculation::Multiply(
			                            shoulderTranslation,
			                            joints_[static_cast<uint32_t>(shoulder.parentIndex)].worldMatrix);
		}
		const Matrix4x4 sourcePivotOffset = Matrix4x4Calculation::MakeTranslateMatrix({
		    -part.sourcePivot.x, -part.sourcePivot.y, -part.sourcePivot.z});
		const Matrix4x4 localAdjustment = Matrix4x4Calculation::MakeAffineMatrix(
		    part.localScale, localRotation, part.jointOffset);
		part.worldTransform.matWorld_ = Matrix4x4Calculation::Multiply(
		    Matrix4x4Calculation::Multiply(sourcePivotOffset, localAdjustment),
		    attachmentWorld);
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
		if (part.model == nullptr) { continue; }
		if (part.usesArticulatedMeshes) {
			DrawArticulatedModelPart(part, camera);
		} else {
			part.model->Draw(part.worldTransform, camera, &defeatColor_);
		}
	}
	if (weaponModel_ != nullptr) { weaponModel_->Draw(weaponTransform_, camera, &defeatColor_); }
}

void BossArmature::DrawArticulatedModelPart(const ModelPart& part, const Camera& camera) const {
	for (std::size_t pieceIndex = 0; pieceIndex < part.articulatedMeshCount; ++pieceIndex) {
		if (part.articulatedModels[pieceIndex] == nullptr) { continue; }
		part.articulatedModels[pieceIndex]->Draw(
		    part.articulatedMeshTransforms[pieceIndex], camera, &defeatColor_);
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
	ImGui::Checkbox("Show weapon debug", &showDebugScythe_);
	ImGui::DragFloat("Joint sphere radius", &jointRadius_, 0.005f, 0.02f, 0.50f);
	ImGui::DragFloat("Weapon model scale", &weaponModelScale_, 0.02f, 0.20f, 3.00f);
	if (ImGui::CollapsingHeader("Idle Animation", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::DragFloat("Idle vertical move", &idleMoveAmount_, 0.005f, 0.0f, 0.50f);
		ImGui::DragFloat("Idle breathing scale", &idleScaleAmount_, 0.001f, 0.0f, 0.15f);
		ImGui::DragFloat("Idle cycle duration", &idleCycleDuration_, 0.05f, 0.20f, 8.0f, "%.2f sec");
		ImGui::DragFloat("Relaxed shoulder drop", &idleShoulderDrop_, 0.01f, 0.0f, 1.50f, "%.2f rad");
		ImGui::DragFloat("Relaxed elbow bend", &idleElbowBend_, 0.01f, 0.0f, 1.50f, "%.2f rad");
		ImGui::DragFloat("Idle torso lean", &idleTorsoLean_, 0.002f, -0.20f, 0.20f, "%.3f rad");
		ImGui::DragFloat("Idle head counter tilt", &idleHeadCounterTilt_, 0.002f, -0.20f, 0.20f, "%.3f rad");
		ImGui::DragFloat("Idle arm sway", &idleArmSway_, 0.002f, 0.0f, 0.25f, "%.3f rad");
	}
	if (ImGui::CollapsingHeader("Damage Hitboxes", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::TextUnformatted("Enable collision boxes in Combat Debug to preview these.");
		ImGui::DragFloat("Body half width", &bodyHitboxHalfWidth_, 0.02f, 0.10f, 8.0f);
		ImGui::DragFloat("Body bottom offset", &bodyHitboxBottomOffset_, 0.02f, -4.0f, 4.0f);
		ImGui::DragFloat("Body head padding", &bodyHitboxTopPadding_, 0.02f, 0.0f, 5.0f);
		ImGui::DragFloat("Body half depth", &bodyHitboxHalfDepth_, 0.02f, 0.10f, 5.0f);
		ImGui::DragFloat3("Weapon hitbox padding", &scytheHitboxPadding_.x, 0.02f, 0.0f, 4.0f);
		ImGui::DragFloat("Weapon hitbox scale", &weaponHitboxScale_, 0.01f, 0.20f, 1.25f);
		ImGui::DragFloat("Throw hitbox half width", &throwHitboxHalfWidth_, 0.02f, 0.10f, 4.0f);
		ImGui::DragFloat("Throw hitbox minimum Y", &throwHitboxMinimumY_, 0.02f, 0.0f, 8.0f);
		ImGui::DragFloat("Throw hitbox maximum Y", &throwHitboxMaximumY_, 0.02f, 0.0f, 12.0f);
		throwHitboxMaximumY_ = (std::max)(throwHitboxMaximumY_, throwHitboxMinimumY_ + 0.10f);
		ImGui::DragFloat("Throw hitbox half depth", &throwHitboxHalfDepth_, 0.02f, 0.10f, 5.0f);
	}

	ImGui::SeparatorText("Control Mode");
	if (ImGui::RadioButton("Animation Debug", controlMode_ == ControlMode::kAnimationDebug)) {
		SetControlMode(ControlMode::kAnimationDebug);
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("Manual Pose Editor", controlMode_ == ControlMode::kPoseEditor)) {
		SetControlMode(ControlMode::kPoseEditor);
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("Keyframe Editor", controlMode_ == ControlMode::kKeyframeEditor)) {
		SetControlMode(ControlMode::kKeyframeEditor);
	}
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
			ImGui::DragFloat("Retreat speed", &retreatSpeed_, 0.10f, 0.1f, 18.0f);
			ImGui::DragFloat("Retreat duration", &retreatDuration_, 0.05f, 0.1f, 3.0f, "%.2f sec");
			ImGui::DragFloat("Retreat jump height", &retreatJumpHeight_, 0.05f, 0.0f, 6.0f);
			ImGui::DragFloat("Spin dash speed", &spinDashSpeed_, 0.10f, 0.0f, 8.0f);
			ImGui::DragFloat("Spin stop distance", &spinDashStopDistance_, 0.10f, 0.5f, 5.0f);
			ImGui::DragFloat("Jump slam move speed", &jumpSlamMoveSpeed_, 0.10f, 1.0f, 30.0f);
			ImGui::DragFloat("Jump slam stop distance", &jumpSlamStopDistance_, 0.10f, 0.25f, 5.0f);
			ImGui::DragFloat("Phase 2 uppercut dash speed", &phaseTwoUppercutDashSpeed_, 0.10f, 1.0f, 30.0f);
			ImGui::DragFloat("Phase 2 uppercut stop distance", &phaseTwoUppercutStopDistance_, 0.10f, 0.25f, 4.0f);
		}

		if (ImGui::CollapsingHeader("AI Move Chances", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::SeparatorText("Phase 1");
			ImGui::Text("Close (distance < %.1f)", closeDistance_);
			ImGui::SliderInt("Melee##P1Close", &closeMeleeChance_, 0, 100, "%d%%");
			ImGui::Text("Jump Retreat: %d%%", 100 - closeMeleeChance_);
			ImGui::Separator();
			ImGui::Text("Mid (%.1f - %.1f)", closeDistance_, midDistance_);
			ImGui::SliderInt("Hook##P1Mid", &midHookChance_, 0, 100, "%d%%");
			midSpinChance_ = std::clamp(midSpinChance_, 0, 100 - midHookChance_);
			ImGui::SliderInt("Spin##P1Mid", &midSpinChance_, 0, 100 - midHookChance_, "%d%%");
			ImGui::Text("Jump Retreat: %d%%", 100 - midHookChance_ - midSpinChance_);
			ImGui::Separator();
			ImGui::Text("Far (distance >= %.1f)", midDistance_);
			ImGui::SliderInt("Throw##P1Far", &farThrowChance_, 0, 100, "%d%%");
			ImGui::Text("Jump Slam: %d%%", 100 - farThrowChance_);

			ImGui::SeparatorText("Phase 2");
			ImGui::Text("Close (distance < %.1f)", closeDistance_);
			ImGui::SliderInt("Uppercut##P2Close", &phaseTwoCloseUppercutChance_, 0, 100, "%d%%");
			phaseTwoCloseMeleeChance_ = std::clamp(
			    phaseTwoCloseMeleeChance_, 0, 100 - phaseTwoCloseUppercutChance_);
			ImGui::SliderInt(
			    "Melee##P2Close", &phaseTwoCloseMeleeChance_, 0,
			    100 - phaseTwoCloseUppercutChance_, "%d%%");
			ImGui::Text(
			    "Jump Retreat: %d%%",
			    100 - phaseTwoCloseUppercutChance_ - phaseTwoCloseMeleeChance_);

			ImGui::Text("Mid (%.1f - %.1f)", closeDistance_, midDistance_);
			ImGui::SliderInt("Ground Wave##P2Mid", &phaseTwoMidGroundWaveChance_, 0, 100, "%d%%");
			phaseTwoMidSpinChance_ = std::clamp(
			    phaseTwoMidSpinChance_, 0, 100 - phaseTwoMidGroundWaveChance_);
			ImGui::SliderInt(
			    "Spin##P2Mid", &phaseTwoMidSpinChance_, 0,
			    100 - phaseTwoMidGroundWaveChance_, "%d%%");
			phaseTwoMidHookChance_ = std::clamp(
			    phaseTwoMidHookChance_, 0,
			    100 - phaseTwoMidGroundWaveChance_ - phaseTwoMidSpinChance_);
			ImGui::SliderInt(
			    "Hook##P2Mid", &phaseTwoMidHookChance_, 0,
			    100 - phaseTwoMidGroundWaveChance_ - phaseTwoMidSpinChance_, "%d%%");
			ImGui::Text(
			    "Jump Retreat: %d%%",
			    100 - phaseTwoMidGroundWaveChance_ - phaseTwoMidSpinChance_ - phaseTwoMidHookChance_);

			ImGui::Text("Far (distance >= %.1f)", midDistance_);
			ImGui::SliderInt("Jump Slam##P2Far", &phaseTwoFarJumpSlamChance_, 0, 100, "%d%%");
			phaseTwoFarThrowChance_ = std::clamp(
			    phaseTwoFarThrowChance_, 0, 100 - phaseTwoFarJumpSlamChance_);
			ImGui::SliderInt(
			    "Throw##P2Far", &phaseTwoFarThrowChance_, 0,
			    100 - phaseTwoFarJumpSlamChance_, "%d%%");
			ImGui::Text(
			    "Shadow Pillars: %d%%",
			    100 - phaseTwoFarJumpSlamChance_ - phaseTwoFarThrowChance_);
		}

		if (ImGui::CollapsingHeader("Force AI Move")) {
			if (ImGui::Button("Force Melee")) { EnterAIState(AIState::kMeleeAttack); }
			ImGui::SameLine();
			if (ImGui::Button("Force Jump Retreat")) { EnterAIState(AIState::kRetreat); }
			if (ImGui::Button("Force Spin")) { EnterAIState(AIState::kSpinAttack); }
			ImGui::SameLine();
			if (ImGui::Button("Force Hook")) { EnterAIState(AIState::kVerticalHook); }
			ImGui::SameLine();
			if (ImGui::Button("Force Throw")) { EnterAIState(AIState::kScytheThrow); }
			ImGui::SameLine();
			if (ImGui::Button("Force Jump Slam")) { EnterAIState(AIState::kJumpSlam); }
			if (ImGui::Button("Force P2 Uppercut")) { EnterAIState(AIState::kPhaseTwoUppercut); }
			ImGui::SameLine();
			if (ImGui::Button("Force P2 Ground Wave")) { EnterAIState(AIState::kPhaseTwoGroundWave); }
			if (ImGui::Button("Force P2 Pillars")) { EnterAIState(AIState::kPhaseTwoPillars); }
		}
		if (ImGui::Button("Reset AI and boss position")) { ResetBossPosition(); }
	} else if (controlMode_ == ControlMode::kAnimationDebug) {
		ImGui::TextUnformatted("AI inactive: use the Play buttons to inspect each move.");
	} else if (controlMode_ == ControlMode::kPoseEditor) {
		ImGui::SeparatorText("Manual Pose Editor");
		ImGui::TextWrapped(
		    "AI, breathing, and animation playback are frozen. Edit a pose, open the desired joint/model sections, then take a screenshot.");
		ImGui::SeparatorText("Debug Camera");
		ImGui::TextUnformatted("Right mouse drag: orbit");
		ImGui::TextUnformatted("Middle mouse drag: pan");
		ImGui::TextUnformatted("Mouse wheel: zoom");
	} else {
		ImGui::SeparatorText("Keyframe Editor");
		ImGui::TextWrapped(
		    "Choose a move and frame, edit its joint SRT below, then use Play Edited Move to test the changed runtime keyframes.");
		ImGui::SeparatorText("Debug Camera");
		ImGui::TextUnformatted("Right mouse drag: orbit");
		ImGui::TextUnformatted("Middle mouse drag: pan");
		ImGui::TextUnformatted("Mouse wheel: zoom");
	}

	if (controlMode_ == ControlMode::kKeyframeEditor) {
		ImGui::SeparatorText("Editable Keyframes");
		constexpr const char* kMoveNames[] = {
		    "Normal Attack", "Scythe Throw", "Spin Attack", "Vertical Hook", "Jump Slam",
		    "P2 Dash Uppercut", "P2 Ground Wave", "P2 Shadow Pillars"};
		int moveIndex = 0;
		switch (keyframeEditorAnimation_) {
		case AnimationType::kNormalAttack: moveIndex = 0; break;
		case AnimationType::kScytheThrow: moveIndex = 1; break;
		case AnimationType::kSpinAttack: moveIndex = 2; break;
		case AnimationType::kVerticalHook: moveIndex = 3; break;
		case AnimationType::kJumpSlam: moveIndex = 4; break;
		case AnimationType::kPhaseTwoUppercut: moveIndex = 5; break;
		case AnimationType::kPhaseTwoGroundWave: moveIndex = 6; break;
		case AnimationType::kPhaseTwoPillars: moveIndex = 7; break;
		case AnimationType::kNone: break;
		}
		ImGui::BeginDisabled(keyframePreviewPlaying_);
		if (ImGui::Combo("Move", &moveIndex, kMoveNames, IM_ARRAYSIZE(kMoveNames))) {
			constexpr AnimationType kMoveTypes[] = {
			    AnimationType::kNormalAttack, AnimationType::kScytheThrow,
			    AnimationType::kSpinAttack, AnimationType::kVerticalHook,
			    AnimationType::kJumpSlam, AnimationType::kPhaseTwoUppercut,
			    AnimationType::kPhaseTwoGroundWave, AnimationType::kPhaseTwoPillars};
			keyframeEditorAnimation_ = kMoveTypes[moveIndex];
			selectedKeyframeIndex_ = 0;
			LoadSelectedKeyframePose();
		}

		std::size_t keyframeCount = 0;
		AttackKeyframe* keyframes = GetEditableKeyframes(keyframeEditorAnimation_, keyframeCount);
		if (keyframes != nullptr && keyframeCount > 0) {
			ImGui::Text("Frames: %zu", keyframeCount);
			ImGui::BeginChild("KeyframeList", ImVec2(0.0f, 145.0f), true);
			for (std::size_t frameIndex = 0; frameIndex < keyframeCount; ++frameIndex) {
				char label[64];
				sprintf_s(label, "Frame %zu   (%.3f sec)", frameIndex + 1, keyframes[frameIndex].time);
				if (ImGui::Selectable(label, selectedKeyframeIndex_ == frameIndex)) {
					selectedKeyframeIndex_ = frameIndex;
					LoadSelectedKeyframePose();
				}
			}
			ImGui::EndChild();

			AttackKeyframe& selected = keyframes[selectedKeyframeIndex_];
			float minimumTime = selectedKeyframeIndex_ == 0
			                        ? 0.0f
			                        : keyframes[selectedKeyframeIndex_ - 1].time + 0.001f;
			float maximumTime = selectedKeyframeIndex_ + 1 >= keyframeCount
			                        ? selected.time
			                        : keyframes[selectedKeyframeIndex_ + 1].time - 0.001f;
			if (selectedKeyframeIndex_ == keyframeCount - 1) {
				switch (keyframeEditorAnimation_) {
				case AnimationType::kNormalAttack: maximumTime = kNormalAttackDuration; break;
				case AnimationType::kScytheThrow: maximumTime = kScytheThrowDuration; break;
				case AnimationType::kSpinAttack: maximumTime = kSpinAttackDuration; break;
				case AnimationType::kVerticalHook: maximumTime = kVerticalHookDuration; break;
				case AnimationType::kJumpSlam: maximumTime = kJumpSlamDuration; break;
				case AnimationType::kPhaseTwoUppercut: maximumTime = kPhaseTwoUppercutDuration; break;
				case AnimationType::kPhaseTwoGroundWave: maximumTime = kPhaseTwoGroundWaveDuration; break;
				case AnimationType::kPhaseTwoPillars: maximumTime = kPhaseTwoPillarsDuration; break;
				case AnimationType::kNone: break;
				}
			}
			if (ImGui::DragFloat("Selected frame time", &selected.time, 0.005f, minimumTime, maximumTime, "%.3f sec")) {
				selected.time = std::clamp(selected.time, minimumTime, maximumTime);
				animationTime_ = selected.time;
			}
			ImGui::Text("Editing Frame %zu", selectedKeyframeIndex_ + 1);
		}
		ImGui::EndDisabled();

		if (ImGui::Button("Play Edited Move")) { StartKeyframePreview(); }
		ImGui::SameLine();
		if (ImGui::Button("Stop Preview")) { StopKeyframePreview(); }
		ImGui::Checkbox("Pause preview", &pauseAnimation_);
		ImGui::SameLine();
		ImGui::Checkbox("Loop preview", &loopAnimation_);
		ImGui::Text("Preview: %s", keyframePreviewPlaying_ ? "Playing edited values" : "Selected frame frozen");
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
	if (ImGui::CollapsingHeader("Jump Slam", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (controlMode_ == ControlMode::kAnimationDebug && ImGui::Button("Play Jump Slam")) { StartAnimation(AnimationType::kJumpSlam); }
		ImGui::DragFloat("Jump slam speed", &jumpSlamPlaybackSpeed_, 0.05f, 0.10f, 3.0f);
		ImGui::DragFloat("Jump slam duration", &jumpSlamPlaybackDuration_, 0.05f, 0.50f, 5.0f, "%.2f sec");
	}
	if (ImGui::CollapsingHeader("Phase 2 Dash Uppercut", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (controlMode_ == ControlMode::kAnimationDebug && ImGui::Button("Play P2 Dash Uppercut")) { StartAnimation(AnimationType::kPhaseTwoUppercut); }
		ImGui::DragFloat("P2 uppercut speed", &phaseTwoUppercutPlaybackSpeed_, 0.05f, 0.10f, 3.0f);
		ImGui::DragFloat("P2 uppercut duration", &phaseTwoUppercutPlaybackDuration_, 0.05f, 0.40f, 4.0f, "%.2f sec");
	}
	if (ImGui::CollapsingHeader("Phase 2 Ground Wave", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (controlMode_ == ControlMode::kAnimationDebug && ImGui::Button("Play P2 Ground Wave")) { StartAnimation(AnimationType::kPhaseTwoGroundWave); }
		ImGui::DragFloat("P2 wave speed", &phaseTwoGroundWavePlaybackSpeed_, 0.05f, 0.10f, 3.0f);
		ImGui::DragFloat("P2 wave duration", &phaseTwoGroundWavePlaybackDuration_, 0.05f, 0.40f, 12.0f, "%.2f sec");
		ImGui::DragFloat("P2 wave range", &phaseTwoGroundWaveRange_, 0.10f, 3.0f, 25.0f);
		ImGui::DragFloat("P2 wave half width", &phaseTwoGroundWaveHalfWidth_, 0.05f, 0.25f, 2.0f);
		ImGui::DragFloat("P2 wave height", &phaseTwoGroundWaveHeight_, 0.05f, 0.50f, 6.0f);
		ImGui::DragFloat("Duration between waves", &phaseTwoGroundWaveInterval_, 0.05f, 0.10f, 4.5f, "%.2f sec");
	}
	if (ImGui::CollapsingHeader("Phase 2 Shadow Pillars", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (controlMode_ == ControlMode::kAnimationDebug && ImGui::Button("Play P2 Shadow Pillars")) { StartAnimation(AnimationType::kPhaseTwoPillars); }
		ImGui::DragFloat("P2 pillars speed", &phaseTwoPillarsPlaybackSpeed_, 0.05f, 0.10f, 3.0f);
		ImGui::DragFloat("P2 pillars duration", &phaseTwoPillarsPlaybackDuration_, 0.05f, 1.00f, 10.0f, "%.2f sec");
	}
	ImGui::SeparatorText("Playback Debug");
	ImGui::Text("Playing: %s", GetActiveAnimationName());
	if (isScytheDetached_) { ImGui::TextUnformatted("Scythe state: airborne"); }
	if (controlMode_ == ControlMode::kAnimationDebug || controlMode_ == ControlMode::kKeyframeEditor) {
		ImGui::Checkbox("Pause animation", &pauseAnimation_);
		ImGui::SameLine();
		ImGui::Checkbox("Loop animation", &loopAnimation_);
	}
	const float authoredDuration = GetActiveAnimationDuration();
	if (authoredDuration > 0.0f) {
		if (controlMode_ == ControlMode::kAnimationDebug ||
		    (controlMode_ == ControlMode::kKeyframeEditor && keyframePreviewPlaying_)) {
			ImGui::SliderFloat("Timeline", &animationTime_, 0.0f, authoredDuration, "%.2f");
		}
		const float effectiveSeconds = GetActivePlaybackDuration() / GetActivePlaybackSpeed();
		ImGui::Text("Effective play time: %.2f sec", effectiveSeconds);
	}
	const float progress = authoredDuration > 0.0f ? std::clamp(animationTime_ / authoredDuration, 0.0f, 1.0f) : 0.0f;
	ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f));
	if (controlMode_ == ControlMode::kAnimationDebug) {
		if (activeAnimation_ != AnimationType::kNone && ImGui::Button("Edit Current Frame")) {
			FreezeCurrentPoseForEditing();
		}
		if (activeAnimation_ != AnimationType::kNone) { ImGui::SameLine(); }
		if (ImGui::Button("Stop Animation")) { StopAnimation(); }
		ImGui::SameLine();
		if (ImGui::Button("Reset pose")) { ResetPose(); }
	}
	if (controlMode_ == ControlMode::kAnimationDebug || controlMode_ == ControlMode::kPoseEditor ||
	    controlMode_ == ControlMode::kKeyframeEditor) {
		if (controlMode_ == ControlMode::kPoseEditor) {
			if (ImGui::Button("Reset Editable Pose")) { ResetPose(); }
			ImGui::SameLine();
			if (ImGui::Button("Reset Model Attachments")) {
				for (ModelPart& part : modelParts_) {
					part.sourcePivot = part.defaultSourcePivot;
					part.localScale = part.defaultLocalScale;
					part.localRotation = part.defaultLocalRotation;
					part.jointOffset = part.defaultJointOffset;
				}
			}
			ImGui::SeparatorText("Imported Model Parts");
			ImGui::TextWrapped(
			    "Each imported arm is split by its OBJ object groups: upper pieces follow the shoulder, the lower piece follows the elbow, and Palm follows the hand joint.");
			constexpr const char* kModelPartNames[] = {"Body Model", "Head Model", "Left Arm Model", "Right Arm Model"};
			for (std::size_t index = 0; index < modelParts_.size(); ++index) {
				ModelPart& part = modelParts_[index];
				ImGui::PushID(static_cast<int>(1000u + index));
				if (ImGui::TreeNode(kModelPartNames[index])) {
					ImGui::DragFloat3("Joint offset", &part.jointOffset.x, 0.01f);
					ImGui::DragFloat3("Model rotation", &part.localRotation.x, 0.01f);
					ImGui::DragFloat3("Model scale", &part.localScale.x, 0.01f, 0.05f, 10.0f);
					ImGui::DragFloat3("Source pivot", &part.sourcePivot.x, 0.005f);
					ImGui::TreePop();
				}
				ImGui::PopID();
			}
		}
		ImGui::Separator();
		ImGui::TextUnformatted("Local SRT (radians for rotation)");
		ImGui::BeginDisabled(
		    (controlMode_ == ControlMode::kAnimationDebug && activeAnimation_ != AnimationType::kNone) ||
		    (controlMode_ == ControlMode::kKeyframeEditor && keyframePreviewPlaying_));
		for (uint32_t index = 0; index < kJointCount; ++index) {
			Joint& joint = joints_[index];
			ImGui::PushID(static_cast<int>(index));
			if (ImGui::TreeNode(joint.name)) {
				ImGui::Text("Parent: %s", joint.parentIndex < 0 ? "None" : joints_[static_cast<uint32_t>(joint.parentIndex)].name);
				bool changed = ImGui::DragFloat3("Translation", &joint.translation.x, 0.01f);
				changed |= ImGui::DragFloat3("Rotation", &joint.rotation.x, 0.01f);
				changed |= ImGui::DragFloat3("Scale", &joint.scale.x, 0.01f, 0.01f, 10.0f);
				if (changed && controlMode_ == ControlMode::kKeyframeEditor) {
					StoreJointInSelectedKeyframe(static_cast<JointIndex>(index));
				}
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
		joints_[index].scale = defaultScales_[index];
	}
}

void BossArmature::ClearIdlePose() {
	joints_[kRoot].translation.y = defaultTranslations_[kRoot].y;
	joints_[kRoot].translation.z = defaultTranslations_[kRoot].z;
	joints_[kRoot].scale = defaultScales_[kRoot];
	joints_[kBody].rotation = defaultRotations_[kBody];
	joints_[kHead].rotation = defaultRotations_[kHead];
	joints_[kLeftShoulder].rotation = defaultRotations_[kLeftShoulder];
	joints_[kLeftElbow].rotation = defaultRotations_[kLeftElbow];
	joints_[kRightShoulder].rotation = defaultRotations_[kRightShoulder];
	joints_[kRightElbow].rotation = defaultRotations_[kRightElbow];
	joints_[kLeftShoulder].scale = defaultScales_[kLeftShoulder];
	joints_[kRightShoulder].scale = defaultScales_[kRightShoulder];
}

void BossArmature::UpdateIdleAnimation() {
	idleAnimationTimer_ += kFrameTime;
	const float cycleDuration = (std::max)(idleCycleDuration_, kFrameTime);
	if (idleAnimationTimer_ >= cycleDuration) { idleAnimationTimer_ = std::fmod(idleAnimationTimer_, cycleDuration); }
	if (phaseTransitionActive_ || activeAnimation_ != AnimationType::kNone) {
		idleBlendTimer_ = 0.0f;
		return;
	}

	// An attack finishes on the exact authored idle pose. Ease the breathing and
	// relaxed guard offsets back in instead of applying the current sine value in
	// one frame, which previously looked like the animation snapped at its end.
	idleBlendTimer_ = (std::min)(idleBlendTimer_ + kFrameTime, kIdleBlendInDuration);
	const float idleBlend = SmootherStep(idleBlendTimer_ / kIdleBlendInDuration);

	const float breath =
	    std::sin(idleAnimationTimer_ / cycleDuration * 2.0f * std::numbers::pi_v<float>) * idleBlend;
	float retreatJump = 0.0f;
	if (controlMode_ == ControlMode::kPlayTest && aiState_ == AIState::kRetreat) {
		const float progress = 1.0f - std::clamp(
		    retreatTimer_ / (std::max)(retreatDuration_, kFrameTime), 0.0f, 1.0f);
		retreatJump = std::sin(progress * std::numbers::pi_v<float>) * retreatJumpHeight_;
	}
	joints_[kRoot].translation.y =
	    defaultTranslations_[kRoot].y + breath * idleMoveAmount_ + retreatJump;
	joints_[kRoot].scale = {
	    defaultScales_[kRoot].x * (1.0f - breath * idleScaleAmount_ * 0.45f),
	    defaultScales_[kRoot].y * (1.0f + breath * idleScaleAmount_),
	    defaultScales_[kRoot].z * (1.0f - breath * idleScaleAmount_ * 0.45f)};

	// The waiting pose is a relaxed scythe guard rather than the authored bind
	// T-pose. StartAnimation() calls ClearIdlePose(), so attack clips continue to
	// use their original rotations and return naturally to this pose afterward.
	const float sway = breath * idleArmSway_;
	joints_[kBody].rotation.z =
	    defaultRotations_[kBody].z + facingDirection_ * idleTorsoLean_ * idleBlend;
	joints_[kHead].rotation.z =
	    defaultRotations_[kHead].z - facingDirection_ * idleHeadCounterTilt_ * idleBlend;
	joints_[kLeftShoulder].rotation.z =
	    defaultRotations_[kLeftShoulder].z - idleShoulderDrop_ * idleBlend + sway;
	joints_[kLeftElbow].rotation.z =
	    defaultRotations_[kLeftElbow].z - idleElbowBend_ * idleBlend + sway * 0.35f;
	joints_[kRightShoulder].rotation.z =
	    defaultRotations_[kRightShoulder].z + idleShoulderDrop_ * idleBlend - sway;
	joints_[kRightElbow].rotation.z =
	    defaultRotations_[kRightElbow].z + idleElbowBend_ * idleBlend - sway * 0.35f;
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
	const bool leavingEditor =
	    controlMode_ == ControlMode::kPoseEditor || controlMode_ == ControlMode::kKeyframeEditor;
	const bool enteringEditor =
	    mode == ControlMode::kPoseEditor || mode == ControlMode::kKeyframeEditor;
	StopAnimation();
	if (enteringEditor || leavingEditor) { ResetPose(); }
	controlMode_ = mode;
	keyframePreviewPlaying_ = false;
	aiState_ = AIState::kWaiting;
	aiWaitTimer_ = mode == ControlMode::kPlayTest && aiEnabled_ ? 0.25f : 0.0f;
	retreatTimer_ = 0.0f;
	lastAIRoll_ = -1;
	loopAnimation_ = false;
	pauseAnimation_ = false;
	showDebugArmature_ = mode != ControlMode::kPlayTest;
	showDebugScythe_ = mode != ControlMode::kPlayTest;
	FacePlayer();
	if (mode == ControlMode::kKeyframeEditor) { LoadSelectedKeyframePose(); }
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

void BossArmature::StartPhaseTwoAI() {
	// The Phase 2 opener is deterministic: begin the pillar sequence as soon as
	// the transition finishes. Its normal completion returns the state machine
	// to Waiting, where the regular Phase 2 distance decisions resume.
	isPhaseTwo_ = true;
	aiEnabled_ = true;
	aiWaitTimer_ = 0.0f;
	retreatTimer_ = 0.0f;
	lastAIRoll_ = -1;
	EnterAIState(AIState::kPhaseTwoPillars);
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
	if (activeAnimation_ == AnimationType::kScytheThrow && useExplicitScythePose_) {
		// Throw uses a compact 2D gameplay band rather than the large rotating
		// weapon bounds. Grounded and single-jump players remain inside this band,
		// while the double-jump peak clears its fixed top edge.
		return {
		    {explicitScytheCenter_.x - throwHitboxHalfWidth_, throwHitboxMinimumY_,
		     explicitScytheCenter_.z - throwHitboxHalfDepth_},
		    {explicitScytheCenter_.x + throwHitboxHalfWidth_, throwHitboxMaximumY_,
		     explicitScytheCenter_.z + throwHitboxHalfDepth_}};
	}

	auto scaleBox = [this](const CollisionBox& box) {
		const Vector3 center = {
		    (box.min.x + box.max.x) * 0.5f,
		    (box.min.y + box.max.y) * 0.5f,
		    (box.min.z + box.max.z) * 0.5f};
		const Vector3 half = {
		    (box.max.x - box.min.x) * 0.5f * weaponHitboxScale_,
		    (box.max.y - box.min.y) * 0.5f * weaponHitboxScale_,
		    (box.max.z - box.min.z) * 0.5f * weaponHitboxScale_};
		CollisionBox result = {
		    {center.x - half.x, center.y - half.y, center.z - half.z},
		    {center.x + half.x, center.y + half.y, center.z + half.z}};
		return result;
	};
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
		return scaleBox({
		    {gripA.x - scytheHitboxPadding_.x, gripA.y - scytheHitboxPadding_.y, gripA.z - scytheHitboxPadding_.z},
		    {gripA.x + scytheHitboxPadding_.x, gripA.y + scytheHitboxPadding_.y, gripA.z + scytheHitboxPadding_.z}});
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
	return scaleBox({
	    {minimum.x - scytheHitboxPadding_.x, minimum.y - scytheHitboxPadding_.y, minimum.z - scytheHitboxPadding_.z},
	    {maximum.x + scytheHitboxPadding_.x, maximum.y + scytheHitboxPadding_.y, maximum.z + scytheHitboxPadding_.z}});
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
	case AnimationType::kJumpSlam:
		return animationTime_ >= 0.96f && animationTime_ <= 1.16f;
	case AnimationType::kPhaseTwoUppercut:
		return animationTime_ >= 0.18f && animationTime_ <= 0.68f;
	case AnimationType::kPhaseTwoGroundWave:
	case AnimationType::kPhaseTwoPillars:
		return false;
	case AnimationType::kNone:
		return false;
	}
	return false;
}

bool BossArmature::IsBodyAttackActive() const {
	// The boss body is a hurtbox and a solid obstacle only. Damage comes from
	// the weapon and special attacks, so touching the boss during wind-up never
	// damages the player.
	return false;
}

bool BossArmature::IsVerticalHookAttackActive() const {
	return activeAnimation_ == AnimationType::kVerticalHook && animationTime_ >= 0.32f && animationTime_ <= 0.92f;
}

bool BossArmature::IsJumpSlamImpactActive() const {
	return activeAnimation_ == AnimationType::kJumpSlam && animationTime_ >= 0.96f && animationTime_ <= 1.16f;
}

bool BossArmature::GetGroundWaveHitbox(std::size_t index, CollisionBox& hitbox) const {
	if (index >= kGroundWaveCount || activeAnimation_ != AnimationType::kPhaseTwoGroundWave) {
		return false;
	}
	const float waveStartTime = kPhaseTwoGroundWaveImpactTime +
	                            static_cast<float>(index) * phaseTwoGroundWaveInterval_;
	const float elapsed = animationTime_ - waveStartTime;
	if (elapsed < 0.0f || elapsed > kPhaseTwoGroundWaveTravelDuration) { return false; }
	const float progress = SmoothStep(elapsed / kPhaseTwoGroundWaveTravelDuration);
	const float direction = facingDirection_ < 0.0f ? -1.0f : 1.0f;
	const float centerX = animationBaseTranslations_[kRoot].x +
	                      direction * (1.25f + phaseTwoGroundWaveRange_ * progress);
	hitbox = {
	    {centerX - phaseTwoGroundWaveHalfWidth_, 2.00f, -1.50f},
	    {centerX + phaseTwoGroundWaveHalfWidth_, 2.00f + phaseTwoGroundWaveHeight_, 1.50f}};
	return true;
}

bool BossArmature::GetShadowPillarState(
    std::size_t index, CollisionBox& hitbox, float& telegraphProgress,
    bool& damaging) const {
	if (index >= kShadowPillarCount || activeAnimation_ != AnimationType::kPhaseTwoPillars ||
	    !shadowPillarTargetLocked_[index]) {
		return false;
	}
	const float startTime = kShadowPillarFirstTelegraphTime +
	                        static_cast<float>(index) * kShadowPillarInterval;
	const float elapsed = animationTime_ - startTime;
	if (elapsed < 0.0f || elapsed > kShadowPillarTelegraphDuration + kShadowPillarActiveDuration) {
		return false;
	}
	telegraphProgress = std::clamp(elapsed / kShadowPillarTelegraphDuration, 0.0f, 1.0f);
	damaging = elapsed >= kShadowPillarTelegraphDuration;
	const float centerX = shadowPillarTargetX_[index];
	const float activeElapsed = (std::max)(0.0f, elapsed - kShadowPillarTelegraphDuration);
	const float riseProgress = SmoothStep(std::clamp(activeElapsed / kShadowPillarRiseDuration, 0.0f, 1.0f));
	const float pillarTop = 2.00f + (10.50f - 2.00f) * riseProgress;
	hitbox = {{centerX - 0.62f, 2.00f, -1.50f}, {centerX + 0.62f, pillarTop, 1.50f}};
	return true;
}

void BossArmature::UpdatePhaseTwoAttackState() {
	if (activeAnimation_ != AnimationType::kPhaseTwoPillars) {
		shadowPillarTargetLocked_.fill(false);
		return;
	}
	for (std::size_t index = 0; index < kShadowPillarCount; ++index) {
		const float startTime = kShadowPillarFirstTelegraphTime +
		                        static_cast<float>(index) * kShadowPillarInterval;
		if (!shadowPillarTargetLocked_[index] && animationTime_ >= startTime) {
			shadowPillarTargetX_[index] = std::clamp(
			    playerTargetPosition_.x, movementMinX_, movementMaxX_);
			shadowPillarTargetLocked_[index] = true;
		}
	}
}

bool BossArmature::ConsumeSlamImpact() {
	const bool impact = slamImpactPending_;
	slamImpactPending_ = false;
	return impact;
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
		{
			const float difference = retreatTargetX_ - defaultTranslations_[kRoot].x;
			const float movement = std::clamp(
			    difference, -retreatSpeed_ * kFrameTime, retreatSpeed_ * kFrameTime);
			MoveBossX(movement);
		}
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
	case AIState::kJumpSlam:
		if (activeAnimation_ == AnimationType::kNone) {
			EnterAIState(AIState::kWaiting);
			break;
		}
		if (animationTime_ >= kJumpSlamLaunchTime && animationTime_ < kJumpSlamImpactTime) {
			const float difference = actionTargetPosition_.x - defaultTranslations_[kRoot].x;
			const float distance = std::abs(difference);
			if (distance > jumpSlamStopDistance_) {
				const float direction = difference < 0.0f ? -1.0f : 1.0f;
				const float movement = (std::min)(jumpSlamMoveSpeed_ * kFrameTime, distance - jumpSlamStopDistance_);
				MoveBossX(direction * movement);
			}
		}
		break;
	case AIState::kPhaseTwoUppercut:
		if (activeAnimation_ == AnimationType::kNone) {
			EnterAIState(AIState::kWaiting);
			break;
		}
		if (animationTime_ >= 0.16f && animationTime_ < 0.58f) {
			const float difference = playerTargetPosition_.x - defaultTranslations_[kRoot].x;
			const float distance = std::abs(difference);
			if (distance > phaseTwoUppercutStopDistance_) {
				const float direction = difference < 0.0f ? -1.0f : 1.0f;
				const float movement = (std::min)(
				    phaseTwoUppercutDashSpeed_ * kFrameTime,
				    distance - phaseTwoUppercutStopDistance_);
				MoveBossX(direction * movement);
			}
		}
		break;
	case AIState::kMeleeAttack:
	case AIState::kVerticalHook:
	case AIState::kScytheThrow:
	case AIState::kPhaseTwoGroundWave:
	case AIState::kPhaseTwoPillars:
		if (activeAnimation_ == AnimationType::kNone) { EnterAIState(AIState::kWaiting); }
		break;
	}
}

void BossArmature::PickNextAIAction() {
	FacePlayer();
	lastAIRoll_ = NextRandomPercent();
	if (isPhaseTwo_) {
		if (playerDistance_ < closeDistance_) {
			if (lastAIRoll_ < phaseTwoCloseUppercutChance_) {
				EnterAIState(AIState::kPhaseTwoUppercut);
			} else if (lastAIRoll_ < phaseTwoCloseUppercutChance_ + phaseTwoCloseMeleeChance_) {
				EnterAIState(AIState::kMeleeAttack);
			} else {
				EnterAIState(AIState::kRetreat);
			}
			return;
		}
		if (playerDistance_ < midDistance_) {
			if (lastAIRoll_ < phaseTwoMidGroundWaveChance_) {
				EnterAIState(AIState::kPhaseTwoGroundWave);
			} else if (lastAIRoll_ < phaseTwoMidGroundWaveChance_ + phaseTwoMidSpinChance_) {
				EnterAIState(AIState::kSpinAttack);
			} else if (
			    lastAIRoll_ < phaseTwoMidGroundWaveChance_ + phaseTwoMidSpinChance_ +
			                        phaseTwoMidHookChance_) {
				EnterAIState(AIState::kVerticalHook);
			} else {
				EnterAIState(AIState::kRetreat);
			}
			return;
		}
		if (lastAIRoll_ < phaseTwoFarJumpSlamChance_) {
			EnterAIState(AIState::kJumpSlam);
		} else if (lastAIRoll_ < phaseTwoFarJumpSlamChance_ + phaseTwoFarThrowChance_) {
			EnterAIState(AIState::kScytheThrow);
		} else {
			EnterAIState(AIState::kPhaseTwoPillars);
		}
		return;
	}

	if (playerDistance_ < closeDistance_) {
		EnterAIState(lastAIRoll_ < closeMeleeChance_ ? AIState::kMeleeAttack : AIState::kRetreat);
		return;
	}
	if (playerDistance_ < midDistance_) {
		if (lastAIRoll_ < midHookChance_) {
			EnterAIState(AIState::kVerticalHook);
		} else if (lastAIRoll_ < midHookChance_ + midSpinChance_) {
			EnterAIState(AIState::kSpinAttack);
		} else {
			EnterAIState(AIState::kRetreat);
		}
		return;
	}
	EnterAIState(lastAIRoll_ < farThrowChance_ ? AIState::kScytheThrow : AIState::kJumpSlam);
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
		{
			// Prefer a landing point one mid-range band away from the player. If
			// the arena wall blocks a conventional backward jump, leap over the
			// player to the opposite side so close-range pressure is still broken.
			const float desiredSeparation = midDistance_ + 1.0f;
			const float leftCandidate = std::clamp(
			    playerTargetPosition_.x - desiredSeparation, movementMinX_, movementMaxX_);
			const float rightCandidate = std::clamp(
			    playerTargetPosition_.x + desiredSeparation, movementMinX_, movementMaxX_);
			const float leftSeparation = std::abs(leftCandidate - playerTargetPosition_.x);
			const float rightSeparation = std::abs(rightCandidate - playerTargetPosition_.x);
			if (std::abs(leftSeparation - rightSeparation) <= 0.001f) {
				retreatTargetX_ = defaultTranslations_[kRoot].x < playerTargetPosition_.x
				                      ? leftCandidate
				                      : rightCandidate;
			} else {
				retreatTargetX_ = leftSeparation > rightSeparation ? leftCandidate : rightCandidate;
			}
		}
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
	case AIState::kJumpSlam:
		StartAnimation(AnimationType::kJumpSlam);
		break;
	case AIState::kPhaseTwoUppercut:
		StartAnimation(AnimationType::kPhaseTwoUppercut);
		break;
	case AIState::kPhaseTwoGroundWave:
		StartAnimation(AnimationType::kPhaseTwoGroundWave);
		break;
	case AIState::kPhaseTwoPillars:
		StartAnimation(AnimationType::kPhaseTwoPillars);
		break;
	}
}

void BossArmature::FacePlayer() {
	const float difference = playerTargetPosition_.x - defaultTranslations_[kRoot].x;
	if (std::abs(difference) > 0.001f) { facingDirection_ = difference < 0.0f ? -1.0f : 1.0f; }
	const float facingRotation = facingDirection_ < 0.0f
	                                 ? kIdleFacingYaw
	                                 : std::numbers::pi_v<float> - kIdleFacingYaw;
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
	normalAttackKeyframes_[1].time = 0.28f;
	normalAttackKeyframes_[2].time = 0.48f;
	normalAttackKeyframes_[3].time = 0.65f;
	normalAttackKeyframes_[4].time = 0.88f;
	normalAttackKeyframes_[5].time = kNormalAttackDuration;

	// A readable two-handed scythe swing. The hands first lift the shaft behind
	// the boss, then cross to the other side quickly so the blade traces one clear
	// arc. Keep it in the screen-facing plane to avoid twisting the split meshes.
	AttackPose& windUp = normalAttackKeyframes_[1].pose;
	windUp.translationOffsets[kRoot].x = 0.03f;
	windUp.rotationOffsets[kBody].z = 0.06f;
	windUp.rotationOffsets[kChest].z = 0.10f;
	windUp.rotationOffsets[kHead].z = -0.03f;
	windUp.rotationOffsets[kLeftShoulder].z = 0.10f;
	windUp.rotationOffsets[kLeftElbow].z = 0.10f;
	windUp.rotationOffsets[kRightShoulder].z = -0.15f;
	windUp.rotationOffsets[kRightElbow].z = 0.15f;

	// Hold the weapon back for a few frames so the direction of the attack reads.
	AttackPose& anticipation = normalAttackKeyframes_[2].pose;
	anticipation = windUp;
	anticipation.translationOffsets[kRoot].x = 0.05f;
	anticipation.rotationOffsets[kBody].z = 0.08f;
	anticipation.rotationOffsets[kChest].z = 0.14f;
	anticipation.rotationOffsets[kLeftShoulder].z = 0.14f;
	anticipation.rotationOffsets[kLeftElbow].z = 0.12f;
	anticipation.rotationOffsets[kRightShoulder].z = -0.20f;
	anticipation.rotationOffsets[kRightElbow].z = 0.18f;

	// Fast diagonal cut. Both hands stay on the weapon and the torso supplies
	// most of the apparent power instead of contorting the wrists.
	AttackPose& strike = normalAttackKeyframes_[3].pose;
	strike.translationOffsets[kRoot].x = -0.18f;
	strike.translationOffsets[kRoot].y = 0.02f;
	strike.rotationOffsets[kBody].z = -0.15f;
	strike.rotationOffsets[kChest].z = -0.24f;
	strike.rotationOffsets[kHead].z = 0.05f;
	strike.rotationOffsets[kLeftShoulder].z = -0.10f;
	strike.rotationOffsets[kLeftElbow].z = 0.18f;
	strike.rotationOffsets[kRightShoulder].z = 0.12f;
	strike.rotationOffsets[kRightElbow].z = -0.12f;

	// Let the blade's weight carry slightly past the contact pose, then settle.
	AttackPose& followThrough = normalAttackKeyframes_[4].pose;
	followThrough.translationOffsets[kRoot].x = -0.10f;
	followThrough.translationOffsets[kRoot].y = 0.01f;
	followThrough.rotationOffsets[kBody].z = -0.10f;
	followThrough.rotationOffsets[kChest].z = -0.16f;
	followThrough.rotationOffsets[kHead].z = 0.03f;
	followThrough.rotationOffsets[kLeftShoulder].z = -0.08f;
	followThrough.rotationOffsets[kLeftElbow].z = 0.12f;
	followThrough.rotationOffsets[kRightShoulder].z = 0.08f;
	followThrough.rotationOffsets[kRightElbow].z = -0.08f;

	// Keyframe 6 remains zero and returns exactly to the idle pose.
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

	// Simple one-handed throw: transfer, pull back, release, track, and catch.
	// All arm motion stays in the screen-facing plane to keep the split arm
	// model connected and readable.
	AttackPose& gripTransfer = scytheThrowKeyframes_[1].pose;
	gripTransfer.rotationOffsets[kBody].z = 0.03f;
	gripTransfer.rotationOffsets[kChest].z = 0.06f;
	gripTransfer.rotationOffsets[kLeftShoulder].z = 0.12f;
	gripTransfer.rotationOffsets[kLeftElbow].z = 0.45f;
	gripTransfer.rotationOffsets[kRightShoulder].z = -0.20f;
	gripTransfer.rotationOffsets[kRightElbow].z = 0.20f;

	// Pull the throwing hand behind the shoulder without circling it around the body.
	AttackPose& windUp = scytheThrowKeyframes_[2].pose;
	windUp.translationOffsets[kRoot].x = 0.06f;
	windUp.rotationOffsets[kBody].z = 0.10f;
	windUp.rotationOffsets[kChest].z = 0.16f;
	windUp.rotationOffsets[kHead].z = -0.05f;
	windUp.rotationOffsets[kLeftShoulder].z = 0.18f;
	windUp.rotationOffsets[kLeftElbow].z = 0.65f;
	windUp.rotationOffsets[kRightShoulder].z = -0.65f;
	windUp.rotationOffsets[kRightElbow].z = 0.55f;

	// Step toward the player and extend the throwing arm for a clear release.
	AttackPose& release = scytheThrowKeyframes_[3].pose;
	release.translationOffsets[kRoot].x = -0.12f;
	release.rotationOffsets[kBody].z = -0.12f;
	release.rotationOffsets[kChest].z = -0.20f;
	release.rotationOffsets[kHead].z = 0.06f;
	release.rotationOffsets[kLeftShoulder].z = 0.15f;
	release.rotationOffsets[kLeftElbow].z = 0.60f;
	release.rotationOffsets[kRightShoulder].z = -1.10f;
	release.rotationOffsets[kRightElbow].z = 0.95f;

	// Small follow-through while the weapon is airborne.
	AttackPose& followThrough = scytheThrowKeyframes_[4].pose;
	followThrough.translationOffsets[kRoot].x = -0.15f;
	followThrough.rotationOffsets[kBody].z = -0.10f;
	followThrough.rotationOffsets[kChest].z = -0.16f;
	followThrough.rotationOffsets[kHead].z = 0.05f;
	followThrough.rotationOffsets[kLeftShoulder].z = 0.12f;
	followThrough.rotationOffsets[kLeftElbow].z = 0.55f;
	followThrough.rotationOffsets[kRightShoulder].z = -1.20f;
	followThrough.rotationOffsets[kRightElbow].z = 0.75f;

	// Track the returning blade with the same hand instead of spinning the arm.
	AttackPose& trackReturn = scytheThrowKeyframes_[5].pose;
	trackReturn.rotationOffsets[kLeftShoulder].z = 0.10f;
	trackReturn.rotationOffsets[kLeftElbow].z = 0.45f;
	trackReturn.rotationOffsets[kRightShoulder].z = -0.70f;
	trackReturn.rotationOffsets[kRightElbow].z = 0.45f;

	// Catch with a soft elbow, then keyframe 8 returns directly to idle.
	AttackPose& catchPose = scytheThrowKeyframes_[6].pose;
	catchPose.translationOffsets[kRoot].x = -0.04f;
	catchPose.rotationOffsets[kBody].z = -0.04f;
	catchPose.rotationOffsets[kChest].z = -0.08f;
	catchPose.rotationOffsets[kHead].z = 0.03f;
	catchPose.rotationOffsets[kLeftShoulder].z = 0.08f;
	catchPose.rotationOffsets[kLeftElbow].z = 0.35f;
	catchPose.rotationOffsets[kRightShoulder].z = -0.90f;
	catchPose.rotationOffsets[kRightElbow].z = 0.70f;
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

	// Direct overhead hook: lift, launch vertically oriented blade, pull, regrip.
	// The scythe's range remains controlled by verticalHookReach_ in
	// UpdateScytheState; these poses only make the boss's intention readable.
	AttackPose& prepare = verticalHookKeyframes_[1].pose;
	prepare.translationOffsets[kRoot].x = 0.03f;
	prepare.rotationOffsets[kBody].z = 0.06f;
	prepare.rotationOffsets[kChest].z = 0.10f;
	prepare.rotationOffsets[kLeftShoulder].z = 0.12f;
	prepare.rotationOffsets[kLeftElbow].z = 0.45f;
	prepare.rotationOffsets[kRightShoulder].z = -0.55f;
	prepare.rotationOffsets[kRightElbow].z = 0.45f;

	// Extend toward the target as the scythe leaves the hands.
	AttackPose& release = verticalHookKeyframes_[2].pose;
	release.translationOffsets[kRoot].x = -0.05f;
	release.rotationOffsets[kBody].z = -0.08f;
	release.rotationOffsets[kChest].z = -0.12f;
	release.rotationOffsets[kLeftShoulder].z = 0.15f;
	release.rotationOffsets[kLeftElbow].z = 0.55f;
	release.rotationOffsets[kRightShoulder].z = -0.95f;
	release.rotationOffsets[kRightElbow].z = 0.85f;

	// Lean back and bend the throwing elbow once to sell the pull.
	AttackPose& pull = verticalHookKeyframes_[3].pose;
	pull.translationOffsets[kRoot].x = 0.06f;
	pull.rotationOffsets[kBody].z = 0.06f;
	pull.rotationOffsets[kChest].z = 0.10f;
	pull.rotationOffsets[kLeftShoulder].z = 0.12f;
	pull.rotationOffsets[kLeftElbow].z = 0.50f;
	pull.rotationOffsets[kRightShoulder].z = -0.55f;
	pull.rotationOffsets[kRightElbow].z = 0.25f;

	// Bring both hands back toward their idle grip before the final zero pose.
	AttackPose& recoil = verticalHookKeyframes_[4].pose;
	recoil.rotationOffsets[kLeftShoulder].z = 0.06f;
	recoil.rotationOffsets[kLeftElbow].z = 0.20f;
	recoil.rotationOffsets[kRightShoulder].z = -0.20f;
	recoil.rotationOffsets[kRightElbow].z = 0.15f;
}

void BossArmature::InitializeJumpSlamClip() {
	for (AttackKeyframe& keyframe : jumpSlamKeyframes_) { keyframe = {}; }
	jumpSlamKeyframes_[0].time = 0.00f;
	jumpSlamKeyframes_[1].time = 0.28f;
	jumpSlamKeyframes_[2].time = 0.62f;
	jumpSlamKeyframes_[3].time = 0.88f;
	jumpSlamKeyframes_[4].time = kJumpSlamImpactTime;
	jumpSlamKeyframes_[5].time = kJumpSlamDuration;

	// Crouch and lift the hammer. The poses stay deliberately simple so they
	// remain easy to adjust in the existing keyframe editor.
	AttackPose& crouch = jumpSlamKeyframes_[1].pose;
	crouch.translationOffsets[kRoot].y = -0.38f;
	crouch.rotationOffsets[kBody].z = 0.08f;
	crouch.rotationOffsets[kChest].z = 0.12f;
	crouch.rotationOffsets[kLeftShoulder].z = 0.35f;
	crouch.rotationOffsets[kLeftElbow].z = 0.45f;
	crouch.rotationOffsets[kRightShoulder].z = -0.35f;
	crouch.rotationOffsets[kRightElbow].z = 0.45f;

	// Rise with both hands holding the weapon above the body.
	AttackPose& apex = jumpSlamKeyframes_[2].pose;
	apex.translationOffsets[kRoot].y = 3.20f;
	apex.rotationOffsets[kBody].z = -0.04f;
	apex.rotationOffsets[kChest].z = -0.08f;
	apex.rotationOffsets[kLeftShoulder].z = 0.70f;
	apex.rotationOffsets[kLeftElbow].z = 0.50f;
	apex.rotationOffsets[kRightShoulder].z = -0.70f;
	apex.rotationOffsets[kRightElbow].z = 0.50f;

	// Start the fall while keeping the same readable two-handed overhead pose.
	AttackPose& fall = jumpSlamKeyframes_[3].pose;
	fall = apex;
	fall.translationOffsets[kRoot].y = 1.35f;
	fall.rotationOffsets[kBody].z = -0.10f;
	fall.rotationOffsets[kChest].z = -0.16f;

	// One compact impact pose: body low, torso forward, weapon driven downward.
	AttackPose& impact = jumpSlamKeyframes_[4].pose;
	impact.translationOffsets[kRoot].y = -0.12f;
	impact.rotationOffsets[kBody].z = -0.18f;
	impact.rotationOffsets[kChest].z = -0.26f;
	impact.rotationOffsets[kHead].z = 0.08f;
	impact.rotationOffsets[kLeftShoulder].z = -0.20f;
	impact.rotationOffsets[kLeftElbow].z = 0.18f;
	impact.rotationOffsets[kRightShoulder].z = 0.20f;
	impact.rotationOffsets[kRightElbow].z = -0.18f;
	// Frame 6 is the zero pose, giving the shared smoother recovery path an
	// exact idle target without a snap.
}

void BossArmature::InitializePhaseTwoUppercutClip() {
	for (AttackKeyframe& keyframe : phaseTwoUppercutKeyframes_) { keyframe = {}; }
	phaseTwoUppercutKeyframes_[0].time = 0.00f;
	phaseTwoUppercutKeyframes_[1].time = 0.18f;
	phaseTwoUppercutKeyframes_[2].time = 0.38f;
	phaseTwoUppercutKeyframes_[3].time = 0.58f;
	phaseTwoUppercutKeyframes_[4].time = 0.82f;
	phaseTwoUppercutKeyframes_[5].time = kPhaseTwoUppercutDuration;

	AttackPose& windUp = phaseTwoUppercutKeyframes_[1].pose;
	windUp.translationOffsets[kRoot].y = -0.18f;
	windUp.rotationOffsets[kBody].z = 0.10f;
	windUp.rotationOffsets[kChest].z = 0.16f;
	windUp.rotationOffsets[kLeftShoulder].z = 0.16f;
	windUp.rotationOffsets[kLeftElbow].z = 0.20f;
	windUp.rotationOffsets[kRightShoulder].z = -0.22f;
	windUp.rotationOffsets[kRightElbow].z = 0.22f;

	AttackPose& uppercut = phaseTwoUppercutKeyframes_[2].pose;
	uppercut.translationOffsets[kRoot].y = 0.65f;
	uppercut.rotationOffsets[kBody].z = -0.14f;
	uppercut.rotationOffsets[kChest].z = -0.22f;
	uppercut.rotationOffsets[kHead].z = 0.06f;
	uppercut.rotationOffsets[kLeftShoulder].z = -0.22f;
	uppercut.rotationOffsets[kLeftElbow].z = 0.18f;
	uppercut.rotationOffsets[kRightShoulder].z = 0.28f;
	uppercut.rotationOffsets[kRightElbow].z = -0.16f;

	AttackPose& followThrough = phaseTwoUppercutKeyframes_[3].pose;
	followThrough = uppercut;
	followThrough.translationOffsets[kRoot].y = 0.35f;
	followThrough.rotationOffsets[kBody].z = -0.08f;
	followThrough.rotationOffsets[kChest].z = -0.12f;

	AttackPose& land = phaseTwoUppercutKeyframes_[4].pose;
	land.translationOffsets[kRoot].y = -0.08f;
	land.rotationOffsets[kBody].z = 0.04f;
	land.rotationOffsets[kChest].z = 0.06f;
	land.rotationOffsets[kLeftShoulder].z = -0.06f;
	land.rotationOffsets[kRightShoulder].z = 0.06f;
}

void BossArmature::InitializePhaseTwoGroundWaveClip() {
	for (AttackKeyframe& keyframe : phaseTwoGroundWaveKeyframes_) { keyframe = {}; }
	phaseTwoGroundWaveKeyframes_[0].time = 0.00f;
	phaseTwoGroundWaveKeyframes_[1].time = 0.30f;
	phaseTwoGroundWaveKeyframes_[2].time = 0.52f;
	phaseTwoGroundWaveKeyframes_[3].time = kPhaseTwoGroundWaveImpactTime;
	phaseTwoGroundWaveKeyframes_[4].time = 8.35f;
	phaseTwoGroundWaveKeyframes_[5].time = kPhaseTwoGroundWaveDuration;

	AttackPose& raise = phaseTwoGroundWaveKeyframes_[1].pose;
	raise.translationOffsets[kRoot].y = -0.12f;
	raise.rotationOffsets[kBody].z = 0.10f;
	raise.rotationOffsets[kChest].z = 0.16f;
	raise.rotationOffsets[kLeftShoulder].z = 0.40f;
	raise.rotationOffsets[kLeftElbow].z = 0.32f;
	raise.rotationOffsets[kRightShoulder].z = -0.40f;
	raise.rotationOffsets[kRightElbow].z = 0.32f;

	AttackPose& hold = phaseTwoGroundWaveKeyframes_[2].pose;
	hold = raise;
	hold.translationOffsets[kRoot].y = -0.18f;
	hold.rotationOffsets[kChest].z = 0.20f;

	AttackPose& slam = phaseTwoGroundWaveKeyframes_[3].pose;
	slam.translationOffsets[kRoot].y = -0.25f;
	slam.rotationOffsets[kBody].z = -0.18f;
	slam.rotationOffsets[kChest].z = -0.28f;
	slam.rotationOffsets[kHead].z = 0.08f;
	slam.rotationOffsets[kLeftShoulder].z = -0.20f;
	slam.rotationOffsets[kLeftElbow].z = 0.18f;
	slam.rotationOffsets[kRightShoulder].z = 0.20f;
	slam.rotationOffsets[kRightElbow].z = -0.18f;

	AttackPose& recoil = phaseTwoGroundWaveKeyframes_[4].pose;
	recoil.translationOffsets[kRoot].y = -0.10f;
	recoil.rotationOffsets[kBody].z = -0.08f;
	recoil.rotationOffsets[kChest].z = -0.12f;
	recoil.rotationOffsets[kLeftShoulder].z = -0.08f;
	recoil.rotationOffsets[kRightShoulder].z = 0.08f;
}

void BossArmature::InitializePhaseTwoPillarsClip() {
	for (AttackKeyframe& keyframe : phaseTwoPillarsKeyframes_) { keyframe = {}; }
	phaseTwoPillarsKeyframes_[0].time = 0.00f;
	phaseTwoPillarsKeyframes_[1].time = 0.90f;
	phaseTwoPillarsKeyframes_[2].time = 2.20f;
	phaseTwoPillarsKeyframes_[3].time = 6.00f;
	phaseTwoPillarsKeyframes_[4].time = 9.20f;
	phaseTwoPillarsKeyframes_[5].time = kPhaseTwoPillarsDuration;

	AttackPose& cast = phaseTwoPillarsKeyframes_[1].pose;
	cast.translationOffsets[kRoot].y = 0.12f;
	cast.rotationOffsets[kBody].z = -0.04f;
	cast.rotationOffsets[kChest].z = -0.08f;
	cast.rotationOffsets[kLeftShoulder].z = 0.32f;
	cast.rotationOffsets[kLeftElbow].z = 0.24f;
	cast.rotationOffsets[kRightShoulder].z = -0.32f;
	cast.rotationOffsets[kRightElbow].z = 0.24f;

	AttackPose& hold = phaseTwoPillarsKeyframes_[2].pose;
	hold = cast;
	hold.translationOffsets[kRoot].y = 0.20f;
	hold.rotationOffsets[kChest].z = -0.12f;

	AttackPose& pulse = phaseTwoPillarsKeyframes_[3].pose;
	pulse = hold;
	pulse.scaleOffsets[kRoot] = {0.05f, 0.08f, 0.05f};
	pulse.rotationOffsets[kLeftShoulder].z = 0.40f;
	pulse.rotationOffsets[kRightShoulder].z = -0.40f;

	AttackPose& settle = phaseTwoPillarsKeyframes_[4].pose;
	settle.translationOffsets[kRoot].y = 0.06f;
	settle.rotationOffsets[kLeftShoulder].z = 0.12f;
	settle.rotationOffsets[kRightShoulder].z = -0.12f;
}

void BossArmature::StartAnimation(AnimationType animation) {
	ClearIdlePose();
	if (activeAnimation_ != AnimationType::kNone) {
		for (uint32_t index = 0; index < kJointCount; ++index) {
			joints_[index].translation = animationBaseTranslations_[index];
			joints_[index].rotation = animationBaseRotations_[index];
			joints_[index].scale = animationBaseScales_[index];
		}
	}
	actionTargetPosition_ = playerTargetPosition_;
	FacePlayer();
	for (uint32_t index = 0; index < kJointCount; ++index) {
		animationBaseTranslations_[index] = joints_[index].translation;
		animationBaseRotations_[index] = joints_[index].rotation;
		animationBaseScales_[index] = joints_[index].scale;
	}
	activeAnimation_ = animation;
	animationTime_ = 0.0f;
	slamImpactPending_ = false;
	shadowPillarTargetLocked_.fill(false);
	isScytheDetached_ = false;
	useExplicitScythePose_ = false;
	hasScytheReleaseCenter_ = false;
	pauseAnimation_ = false;
}

void BossArmature::StopAnimation() {
	if (activeAnimation_ == AnimationType::kNone) { return; }
	for (uint32_t index = 0; index < kJointCount; ++index) {
		joints_[index].translation = animationBaseTranslations_[index];
		joints_[index].rotation = animationBaseRotations_[index];
		joints_[index].scale = animationBaseScales_[index];
	}
	activeAnimation_ = AnimationType::kNone;
	animationTime_ = 0.0f;
	shadowPillarTargetLocked_.fill(false);
	isScytheDetached_ = false;
	useExplicitScythePose_ = false;
	hasScytheReleaseCenter_ = false;
	pauseAnimation_ = false;
}

void BossArmature::FreezeCurrentPoseForEditing() {
	if (activeAnimation_ == AnimationType::kNone) {
		SetControlMode(ControlMode::kPoseEditor);
		return;
	}

	// Do not call StopAnimation here: it deliberately restores the clip's base
	// pose. The editor needs the currently sampled frame exactly as displayed.
	activeAnimation_ = AnimationType::kNone;
	animationTime_ = 0.0f;
	isScytheDetached_ = false;
	useExplicitScythePose_ = false;
	hasScytheReleaseCenter_ = false;
	pauseAnimation_ = false;
	loopAnimation_ = false;
	keyframePreviewPlaying_ = false;
	controlMode_ = ControlMode::kPoseEditor;
	aiState_ = AIState::kWaiting;
	retreatTimer_ = 0.0f;
	showDebugArmature_ = true;
	showDebugScythe_ = true;
}

BossArmature::AttackKeyframe* BossArmature::GetEditableKeyframes(
    AnimationType animation, std::size_t& count) {
	switch (animation) {
	case AnimationType::kNormalAttack:
		count = normalAttackKeyframes_.size();
		return normalAttackKeyframes_.data();
	case AnimationType::kScytheThrow:
		count = scytheThrowKeyframes_.size();
		return scytheThrowKeyframes_.data();
	case AnimationType::kSpinAttack:
		count = spinAttackKeyframes_.size();
		return spinAttackKeyframes_.data();
	case AnimationType::kVerticalHook:
		count = verticalHookKeyframes_.size();
		return verticalHookKeyframes_.data();
	case AnimationType::kJumpSlam:
		count = jumpSlamKeyframes_.size();
		return jumpSlamKeyframes_.data();
	case AnimationType::kPhaseTwoUppercut:
		count = phaseTwoUppercutKeyframes_.size();
		return phaseTwoUppercutKeyframes_.data();
	case AnimationType::kPhaseTwoGroundWave:
		count = phaseTwoGroundWaveKeyframes_.size();
		return phaseTwoGroundWaveKeyframes_.data();
	case AnimationType::kPhaseTwoPillars:
		count = phaseTwoPillarsKeyframes_.size();
		return phaseTwoPillarsKeyframes_.data();
	case AnimationType::kNone:
		break;
	}
	count = 0;
	return nullptr;
}

void BossArmature::LoadSelectedKeyframePose() {
	std::size_t keyframeCount = 0;
	AttackKeyframe* keyframes = GetEditableKeyframes(keyframeEditorAnimation_, keyframeCount);
	if (keyframes == nullptr || keyframeCount == 0) { return; }
	selectedKeyframeIndex_ = (std::min)(selectedKeyframeIndex_, keyframeCount - 1);

	if (activeAnimation_ != AnimationType::kNone) { StopAnimation(); }
	ResetPose();
	FacePlayer();
	for (uint32_t index = 0; index < kJointCount; ++index) {
		animationBaseTranslations_[index] = joints_[index].translation;
		animationBaseRotations_[index] = joints_[index].rotation;
		animationBaseScales_[index] = joints_[index].scale;
	}

	// Apply the exact authored keyframe through the same path used by playback.
	// Temporarily setting activeAnimation_ also preserves the spin-turn scaling.
	activeAnimation_ = keyframeEditorAnimation_;
	ApplyAttackPose(
	    keyframes[selectedKeyframeIndex_].pose,
	    keyframes[selectedKeyframeIndex_].pose, 0.0f);
	activeAnimation_ = AnimationType::kNone;
	animationTime_ = keyframes[selectedKeyframeIndex_].time;
	isScytheDetached_ = false;
	useExplicitScythePose_ = false;
	hasScytheReleaseCenter_ = false;
}

void BossArmature::StoreJointInSelectedKeyframe(JointIndex jointIndex) {
	std::size_t keyframeCount = 0;
	AttackKeyframe* keyframes = GetEditableKeyframes(keyframeEditorAnimation_, keyframeCount);
	if (keyframes == nullptr || selectedKeyframeIndex_ >= keyframeCount) { return; }

	AttackPose& pose = keyframes[selectedKeyframeIndex_].pose;
	const uint32_t index = static_cast<uint32_t>(jointIndex);
	const float rootHorizontalScale = jointIndex == kRoot ? -facingDirection_ : 1.0f;
	pose.translationOffsets[index] = {
	    (joints_[index].translation.x - animationBaseTranslations_[index].x) / rootHorizontalScale,
	    joints_[index].translation.y - animationBaseTranslations_[index].y,
	    joints_[index].translation.z - animationBaseTranslations_[index].z,
	};
	const float spinTurnScale =
	    keyframeEditorAnimation_ == AnimationType::kSpinAttack && jointIndex == kRoot
	        ? static_cast<float>((std::max)(spinAttackTurnCount_, 1))
	        : 1.0f;
	pose.rotationOffsets[index] = {
	    joints_[index].rotation.x - animationBaseRotations_[index].x,
	    (joints_[index].rotation.y - animationBaseRotations_[index].y) / spinTurnScale,
	    joints_[index].rotation.z - animationBaseRotations_[index].z,
	};
	pose.scaleOffsets[index] = {
	    joints_[index].scale.x - animationBaseScales_[index].x,
	    joints_[index].scale.y - animationBaseScales_[index].y,
	    joints_[index].scale.z - animationBaseScales_[index].z,
	};
}

void BossArmature::StartKeyframePreview() {
	ResetPose();
	FacePlayer();
	keyframePreviewPlaying_ = true;
	StartAnimation(keyframeEditorAnimation_);
}

void BossArmature::StopKeyframePreview() {
	StopAnimation();
	keyframePreviewPlaying_ = false;
	pauseAnimation_ = false;
	LoadSelectedKeyframePose();
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
	case AnimationType::kJumpSlam:
		keyframes = jumpSlamKeyframes_.data();
		keyframeCount = jumpSlamKeyframes_.size();
		break;
	case AnimationType::kPhaseTwoUppercut:
		keyframes = phaseTwoUppercutKeyframes_.data();
		keyframeCount = phaseTwoUppercutKeyframes_.size();
		break;
	case AnimationType::kPhaseTwoGroundWave:
		keyframes = phaseTwoGroundWaveKeyframes_.data();
		keyframeCount = phaseTwoGroundWaveKeyframes_.size();
		break;
	case AnimationType::kPhaseTwoPillars:
		keyframes = phaseTwoPillarsKeyframes_.data();
		keyframeCount = phaseTwoPillarsKeyframes_.size();
		break;
	case AnimationType::kNone:
		return;
	}

	const float duration = GetActiveAnimationDuration();
	const float playbackDuration = (std::max)(GetActivePlaybackDuration(), 0.05f);
	const float playbackSpeed = (std::max)(GetActivePlaybackSpeed(), 0.01f);
	const float previousAnimationTime = animationTime_;
	if (!pauseAnimation_) {
		const float authoredTimeScale = duration / playbackDuration;
		animationTime_ = (std::min)(
		    animationTime_ + kFrameTime * playbackSpeed * authoredTimeScale, duration);
	}
	if (activeAnimation_ == AnimationType::kJumpSlam &&
	    previousAnimationTime < kJumpSlamImpactTime && animationTime_ >= kJumpSlamImpactTime) {
		slamImpactPending_ = true;
	}
	std::size_t endIndex = 1;
	while (endIndex < keyframeCount - 1 && animationTime_ > keyframes[endIndex].time) { ++endIndex; }
	const AttackKeyframe& start = keyframes[endIndex - 1];
	const AttackKeyframe& end = keyframes[endIndex];
	const float range = end.time - start.time;
	const float rawT = range > 0.0f ? std::clamp((animationTime_ - start.time) / range, 0.0f, 1.0f) : 1.0f;
	const bool isConstantSpinSegment =
	    activeAnimation_ == AnimationType::kSpinAttack && endIndex >= 3 && endIndex <= 6;
	const bool isFinalRecovery =
	    endIndex == keyframeCount - 1 && activeAnimation_ != AnimationType::kSpinAttack;
	const float t = isConstantSpinSegment ? rawT : (isFinalRecovery ? SmootherStep(rawT) : SmoothStep(rawT));
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
			joints_[index].scale = animationBaseScales_[index];
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
	const bool previewNormalKeyframe =
	    controlMode_ == ControlMode::kKeyframeEditor && !keyframePreviewPlaying_ &&
	    keyframeEditorAnimation_ == AnimationType::kNormalAttack;
	if (activeAnimation_ == AnimationType::kNormalAttack || previewNormalKeyframe) {
		const Vector3& gripA = joints_[kRightHand].worldPosition;
		const Vector3& gripB = joints_[kLeftHand].worldPosition;
		explicitScytheCenter_ = {
		    (gripA.x + gripB.x) * 0.5f,
		    (gripA.y + gripB.y) * 0.5f,
		    (gripA.z + gripB.z) * 0.5f};
		const float heldAngle = std::atan2(gripB.y - gripA.y, gripB.x - gripA.x);
		const float swingDirection = facingDirection_ < 0.0f ? 1.0f : -1.0f;
		const float loadedAngle = facingDirection_ < 0.0f
		                              ? 0.35f
		                              : std::numbers::pi_v<float> - 0.35f;
		const float contactAngle = loadedAngle + swingDirection * 3.0f;
		const float followThroughAngle = contactAngle + swingDirection * 0.35f;
		useExplicitScythePose_ = true;

		auto shortestAngleTo = [](float start, float end) {
			return std::remainder(end - start, 2.0f * std::numbers::pi_v<float>);
		};
		if (animationTime_ <= 0.28f) {
			const float t = SmoothStep(animationTime_ / 0.28f);
			explicitScytheRotation_ = heldAngle + shortestAngleTo(heldAngle, loadedAngle) * t;
		} else if (animationTime_ <= 0.48f) {
			explicitScytheRotation_ = loadedAngle;
		} else if (animationTime_ <= 0.65f) {
			const float t = SmootherStep((animationTime_ - 0.48f) / (0.65f - 0.48f));
			explicitScytheRotation_ = loadedAngle + (contactAngle - loadedAngle) * t;
		} else if (animationTime_ <= 0.88f) {
			const float t = SmoothStep((animationTime_ - 0.65f) / (0.88f - 0.65f));
			explicitScytheRotation_ = contactAngle + (followThroughAngle - contactAngle) * t;
		} else {
			const float t = SmootherStep(
			    (animationTime_ - 0.88f) / (kNormalAttackDuration - 0.88f));
			explicitScytheRotation_ =
			    followThroughAngle + shortestAngleTo(followThroughAngle, heldAngle) * t;
		}

		// Put both animated hand endpoints onto the visible handle during the main
		// arc. Blend into this grip during anticipation and release it during the
		// recovery so neither the arms nor the weapon pop at the clip boundaries.
		float gripBlend = 1.0f;
		if (animationTime_ < 0.28f) {
			gripBlend = SmoothStep(animationTime_ / 0.28f);
		} else if (animationTime_ > 0.88f) {
			gripBlend = 1.0f - SmootherStep(
			    (animationTime_ - 0.88f) / (kNormalAttackDuration - 0.88f));
		}
		const Vector3 weaponAxis = {
		    std::cos(explicitScytheRotation_), std::sin(explicitScytheRotation_), 0.0f};
		constexpr float kHalfGripSpacing = 0.42f;
		const Vector3 desiredRightGrip = {
		    explicitScytheCenter_.x - weaponAxis.x * kHalfGripSpacing,
		    explicitScytheCenter_.y - weaponAxis.y * kHalfGripSpacing,
		    explicitScytheCenter_.z};
		const Vector3 desiredLeftGrip = {
		    explicitScytheCenter_.x + weaponAxis.x * kHalfGripSpacing,
		    explicitScytheCenter_.y + weaponAxis.y * kHalfGripSpacing,
		    explicitScytheCenter_.z};
		auto blendHandToGrip = [this, gripBlend](JointIndex hand, const Vector3& target) {
			Joint& joint = joints_[hand];
			joint.worldPosition = Lerp(joint.worldPosition, target, gripBlend);
			joint.worldMatrix.m[3][0] = joint.worldPosition.x;
			joint.worldMatrix.m[3][1] = joint.worldPosition.y;
			joint.worldMatrix.m[3][2] = joint.worldPosition.z;
			joint.markerTransform.translation_ = joint.worldPosition;
			joint.markerTransform.matWorld_ = Matrix4x4Calculation::MakeAffineMatrix(
			    joint.markerTransform.scale_, joint.markerTransform.rotation_, joint.worldPosition);
			joint.markerTransform.TransferMatrix();
		};
		blendHandToGrip(kRightHand, desiredRightGrip);
		blendHandToGrip(kLeftHand, desiredLeftGrip);
		return;
	}
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
			const float regrip = SmootherStep(std::clamp(
			    (animationTime_ - kHookRegripTime) / (kVerticalHookDuration - kHookRegripTime), 0.0f, 1.0f));
			constexpr float kRegripStartAngle = 1.5f * std::numbers::pi_v<float>;
			const float twoHandAngle = std::atan2(gripB.y - gripA.y, gripB.x - gripA.x);
			const float shortestAngle = std::remainder(
			    twoHandAngle - kRegripStartAngle, 2.0f * std::numbers::pi_v<float>);
			explicitScytheCenter_ = twoHandCenter;
			explicitScytheRotation_ = kRegripStartAngle + shortestAngle * regrip;
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
			scytheThrowFlightDistance_ = (std::min)(aimLength, scytheThrowRange_);
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
		    movingAnchor.x + scytheFlightDirection_.x * scytheThrowFlightDistance_ * travelArc,
		    movingAnchor.y + scytheFlightDirection_.y * scytheThrowFlightDistance_ * travelArc + scytheThrowArcHeight_ * travelArc + 0.22f * std::sin(flightProgress * 2.0f * std::numbers::pi_v<float>),
		    movingAnchor.z + scytheFlightDirection_.z * scytheThrowFlightDistance_ * travelArc,
		};
		explicitScytheRotation_ =
		    releaseAngle + flightProgress * scytheThrowSpinCount_ * 2.0f * std::numbers::pi_v<float>;
		return;
	}

	// Move the caught weapon from the throwing hand back into a two-handed grip.
	const float regripProgress = SmootherStep(std::clamp(
	    (animationTime_ - kScytheCatchTime) / (kScytheThrowDuration - kScytheCatchTime), 0.0f, 1.0f));
	const float regripAngle = std::remainder(
	    twoHandAngle - releaseAngle, 2.0f * std::numbers::pi_v<float>);
	explicitScytheCenter_ = Lerp(throwingHand, twoHandCenter, regripProgress);
	explicitScytheRotation_ = releaseAngle + regripAngle * regripProgress;
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
	case AnimationType::kJumpSlam:
		return kJumpSlamDuration;
	case AnimationType::kPhaseTwoUppercut:
		return kPhaseTwoUppercutDuration;
	case AnimationType::kPhaseTwoGroundWave:
		return kPhaseTwoGroundWaveDuration;
	case AnimationType::kPhaseTwoPillars:
		return kPhaseTwoPillarsDuration;
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
	case AnimationType::kJumpSlam:
		return jumpSlamPlaybackDuration_;
	case AnimationType::kPhaseTwoUppercut:
		return phaseTwoUppercutPlaybackDuration_;
	case AnimationType::kPhaseTwoGroundWave:
		return phaseTwoGroundWavePlaybackDuration_;
	case AnimationType::kPhaseTwoPillars:
		return phaseTwoPillarsPlaybackDuration_;
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
	case AnimationType::kJumpSlam:
		return jumpSlamPlaybackSpeed_;
	case AnimationType::kPhaseTwoUppercut:
		return phaseTwoUppercutPlaybackSpeed_;
	case AnimationType::kPhaseTwoGroundWave:
		return phaseTwoGroundWavePlaybackSpeed_;
	case AnimationType::kPhaseTwoPillars:
		return phaseTwoPillarsPlaybackSpeed_;
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
	case AnimationType::kJumpSlam:
		return "Jump Slam";
	case AnimationType::kPhaseTwoUppercut:
		return "Phase 2 Dash Uppercut";
	case AnimationType::kPhaseTwoGroundWave:
		return "Phase 2 Ground Wave";
	case AnimationType::kPhaseTwoPillars:
		return "Phase 2 Shadow Pillars";
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
		return "Jump Retreat";
	case AIState::kSpinAttack:
		return "Spin Attack";
	case AIState::kVerticalHook:
		return "Vertical Hook";
	case AIState::kScytheThrow:
		return "Scythe Throw";
	case AIState::kJumpSlam:
		return "Jump Slam";
	case AIState::kPhaseTwoUppercut:
		return "Phase 2 Dash Uppercut";
	case AIState::kPhaseTwoGroundWave:
		return "Phase 2 Ground Wave";
	case AIState::kPhaseTwoPillars:
		return "Phase 2 Shadow Pillars";
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
		joints_[index].scale = Lerp(
		    {animationBaseScales_[index].x + start.scaleOffsets[index].x, animationBaseScales_[index].y + start.scaleOffsets[index].y, animationBaseScales_[index].z + start.scaleOffsets[index].z},
		    {animationBaseScales_[index].x + end.scaleOffsets[index].x, animationBaseScales_[index].y + end.scaleOffsets[index].y, animationBaseScales_[index].z + end.scaleOffsets[index].z}, t);
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

float BossArmature::SmootherStep(float t) {
	t = std::clamp(t, 0.0f, 1.0f);
	return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

BossArmature::~BossArmature() {
	for (ModelPart& part : modelParts_) {
		delete part.model;
		part.model = nullptr;
		for (Model*& articulatedModel : part.articulatedModels) {
			delete articulatedModel;
			articulatedModel = nullptr;
		}
	}
	delete weaponModel_;
	weaponModel_ = nullptr;
	delete jointSphereModel_;
	jointSphereModel_ = nullptr;
}
