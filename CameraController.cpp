#include "CameraController.h"
#include "Player.h"
#include <algorithm>

using namespace KamataEngine;

void CameraController::Initialize() { camera_.Initialize(); }

void CameraController::ResetCameraPosition() {
	if (target_ == nullptr) { return; }
	const Vector3& position = target_->GetWorldTransform().translation_;
	camera_.translation_ = {
	    std::clamp(position.x + offset_.x, movableArea_.left, movableArea_.right),
	    std::clamp(position.y + offset_.y, movableArea_.bottom, movableArea_.top),
	    position.z + offset_.z};
	camera_.UpdateMatrix();
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

void CameraController::Update() {
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
	camera_.UpdateMatrix();
}

Vector3 CameraController::Lerp(const Vector3& start, const Vector3& end, float t) {
	return {start.x + (end.x - start.x) * t, start.y + (end.y - start.y) * t, start.z + (end.z - start.z) * t};
}
