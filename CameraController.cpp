#include "CameraController.h"
#include "Player.h"
#include <algorithm>
#include <cmath>
#ifdef USE_IMGUI
#include <imgui.h>
#endif

using namespace KamataEngine;

void CameraController::Initialize() { camera_.Initialize(); }

void CameraController::ResetCameraPosition() {
	if (target_ == nullptr) { return; }
	RemoveShakeOffset();
	shakeTimer_ = 0.0f;
	const Vector3& position = target_->GetWorldTransform().translation_;
	camera_.translation_ = {
	    std::clamp(position.x + offset_.x, movableArea_.left, movableArea_.right),
	    std::clamp(position.y + offset_.y, movableArea_.bottom, movableArea_.top),
	    position.z + offset_.z};
	camera_.UpdateMatrix();
}

void CameraController::StartShake(float duration, float intensity) {
	if (duration <= 0.0f || intensity <= 0.0f) { return; }
	shakeTimer_ = (std::max)(shakeTimer_, duration);
	shakeDuration_ = (std::max)(shakeDuration_, duration);
	shakeIntensity_ = (std::max)(shakeIntensity_, intensity);
	shakePhase_ = 0.0f;
}

void CameraController::RemoveShakeOffset() {
	camera_.translation_.x -= shakeOffset_.x;
	camera_.translation_.y -= shakeOffset_.y;
	camera_.translation_.z -= shakeOffset_.z;
	shakeOffset_ = {};
}

void CameraController::ApplyShake() {
	if (shakeTimer_ <= 0.0f) {
		shakeDuration_ = 0.0f;
		shakeIntensity_ = 0.0f;
		return;
	}
	shakeTimer_ = (std::max)(0.0f, shakeTimer_ - 1.0f / 60.0f);
	shakePhase_ += 1.0f;
	const float fade = shakeDuration_ > 0.0f ? shakeTimer_ / shakeDuration_ : 0.0f;
	const float amplitude = shakeIntensity_ * fade;
	shakeOffset_ = {
	    std::sin(shakePhase_ * 2.17f) * amplitude,
	    std::cos(shakePhase_ * 2.83f) * amplitude,
	    0.0f};
	camera_.translation_.x += shakeOffset_.x;
	camera_.translation_.y += shakeOffset_.y;
	camera_.translation_.z += shakeOffset_.z;
}

void CameraController::LockToPosition(const Vector3& position, float duration) {
	lockStartPosition_ = camera_.translation_;
	lockedPosition_ = position;
	isLocked_ = true;
	lockTimer_ = 0.0f;
	lockDuration_ = (std::max)(duration, 0.0f);
	isLockTransitioning_ = lockDuration_ > 0.0f;
	if (!isLockTransitioning_) {
		camera_.translation_ = lockedPosition_;
		camera_.UpdateMatrix();
	}
}

void CameraController::Unlock() {
	isLocked_ = false;
	isLockTransitioning_ = false;
}

void CameraController::SetDebugMode(bool active, const Vector3& focusPoint) {
	if (active == isDebugMode_) { return; }
	isDebugMode_ = active;
	if (!isDebugMode_) {
		camera_.translation_ = debugEntryTranslation_;
		camera_.rotation_ = debugEntryRotation_;
		camera_.UpdateMatrix();
		return;
	}

	debugEntryTranslation_ = camera_.translation_;
	debugEntryRotation_ = camera_.rotation_;
	debugFocusPoint_ = focusPoint;
	const Vector3 offset = {
	    camera_.translation_.x - debugFocusPoint_.x,
	    camera_.translation_.y - debugFocusPoint_.y,
	    camera_.translation_.z - debugFocusPoint_.z};
	debugDistance_ = std::sqrt(offset.x * offset.x + offset.y * offset.y + offset.z * offset.z);
	if (debugDistance_ < kDebugMinDistance) { debugDistance_ = 15.0f; }
	debugDistance_ = std::clamp(debugDistance_, kDebugMinDistance, kDebugMaxDistance);
	debugYaw_ = std::atan2(offset.x, offset.z);
	const float horizontalDistance = std::sqrt(offset.x * offset.x + offset.z * offset.z);
	debugPitch_ = std::atan2(offset.y, horizontalDistance);
}

void CameraController::UpdateDebugCamera() {
	Input* input = Input::GetInstance();
	bool mouseAvailable = true;
#ifdef USE_IMGUI
	mouseAvailable = !ImGui::GetIO().WantCaptureMouse;
#endif
	if (mouseAvailable) {
		const Input::MouseMove mouseMove = input->GetMouseMove();
		if (input->IsPressMouse(1)) {
			debugYaw_ += static_cast<float>(mouseMove.lX) * kDebugRotateSpeed;
			debugPitch_ += static_cast<float>(mouseMove.lY) * kDebugRotateSpeed;
			debugPitch_ = std::clamp(debugPitch_, -1.45f, 1.45f);
		}

		const int32_t wheel = input->GetWheel();
		if (wheel != 0) {
			debugDistance_ = std::clamp(
			    debugDistance_ - static_cast<float>(wheel) * kDebugZoomSpeed,
			    kDebugMinDistance, kDebugMaxDistance);
		}

		if (input->IsPressMouse(2)) {
			const float panAmount = debugDistance_ * kDebugPanSpeed;
			const Vector3 right = {std::cos(debugYaw_), 0.0f, -std::sin(debugYaw_)};
			const Vector3 up = {
			    -std::sin(debugYaw_) * std::sin(debugPitch_),
			    std::cos(debugPitch_),
			    -std::cos(debugYaw_) * std::sin(debugPitch_)};
			debugFocusPoint_.x -= right.x * static_cast<float>(mouseMove.lX) * panAmount;
			debugFocusPoint_.y -= right.y * static_cast<float>(mouseMove.lX) * panAmount;
			debugFocusPoint_.z -= right.z * static_cast<float>(mouseMove.lX) * panAmount;
			debugFocusPoint_.x += up.x * static_cast<float>(mouseMove.lY) * panAmount;
			debugFocusPoint_.y += up.y * static_cast<float>(mouseMove.lY) * panAmount;
			debugFocusPoint_.z += up.z * static_cast<float>(mouseMove.lY) * panAmount;
		}
	}

	const float cosinePitch = std::cos(debugPitch_);
	camera_.translation_ = {
	    debugFocusPoint_.x + std::sin(debugYaw_) * cosinePitch * debugDistance_,
	    debugFocusPoint_.y + std::sin(debugPitch_) * debugDistance_,
	    debugFocusPoint_.z + std::cos(debugYaw_) * cosinePitch * debugDistance_};
	const Vector3 forward = {
	    debugFocusPoint_.x - camera_.translation_.x,
	    debugFocusPoint_.y - camera_.translation_.y,
	    debugFocusPoint_.z - camera_.translation_.z};
	const float forwardHorizontal = std::sqrt(forward.x * forward.x + forward.z * forward.z);
	camera_.rotation_ = {
	    -std::atan2(forward.y, forwardHorizontal),
	    std::atan2(forward.x, forward.z),
	    0.0f};
	camera_.UpdateMatrix();
}

void CameraController::Update() {
	RemoveShakeOffset();
	if (isDebugMode_) {
		UpdateDebugCamera();
		return;
	}
	if (isLocked_) {
		if (isLockTransitioning_) {
			lockTimer_ = (std::min)(lockTimer_ + 1.0f / 60.0f, lockDuration_);
			const float t = lockDuration_ > 0.0f ? lockTimer_ / lockDuration_ : 1.0f;
			// Quintic ease-in/out starts and finishes with zero velocity, avoiding a
			// visible snap when camera follow changes into the fixed boss arena view.
			const float easedT = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
			camera_.translation_ = Lerp(lockStartPosition_, lockedPosition_, easedT);
			if (lockTimer_ >= lockDuration_) { isLockTransitioning_ = false; }
		} else {
			camera_.translation_ = lockedPosition_;
		}
		ApplyShake();
		camera_.UpdateMatrix();
		return;
	}
	if (target_ == nullptr) { return; }
	const WorldTransform& target = target_->GetWorldTransform();
	const Vector3& velocity = target_->GetVelocity();
	const Vector3 desired = {
	    target.translation_.x + offset_.x + velocity.x * kVelocityBias,
	    target.translation_.y + offset_.y + velocity.y * kVelocityBias,
	    target.translation_.z + offset_.z + velocity.z * kVelocityBias};
	camera_.translation_ = Lerp(camera_.translation_, desired, kInterpolationRate);
	camera_.translation_.x = std::clamp(camera_.translation_.x, target.translation_.x + kTargetMargin.left, target.translation_.x + kTargetMargin.right);
	camera_.translation_.y = std::clamp(camera_.translation_.y, target.translation_.y + kTargetMargin.bottom, target.translation_.y + kTargetMargin.top);
	camera_.translation_.x = std::clamp(camera_.translation_.x, movableArea_.left, movableArea_.right);
	camera_.translation_.y = std::clamp(camera_.translation_.y, movableArea_.bottom, movableArea_.top);
	ApplyShake();
	camera_.UpdateMatrix();
}

Vector3 CameraController::Lerp(const Vector3& start, const Vector3& end, float t) {
	return {start.x + (end.x - start.x) * t, start.y + (end.y - start.y) * t, start.z + (end.z - start.z) * t};
}
