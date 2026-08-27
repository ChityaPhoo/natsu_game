#include "Skydome.h"
#include "Matrix4x4Calculation.h"

using namespace KamataEngine;

void Skydome::Initialize() { worldTransform_.Initialize(); model_ = Model::CreateFromOBJ("environment_sky", true); }

void Skydome::Update(const Camera& camera) {
	// Keep the camera centered inside the dome while the level scrolls. This
	// prevents the dome's open edge from exposing the clear color at map ends.
	worldTransform_.translation_.x = camera.translation_.x;
	worldTransform_.matWorld_ = Matrix4x4Calculation::MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}
void Skydome::Draw(const Camera& camera) { if (model_ != nullptr) { model_->Draw(worldTransform_, camera); } }
Skydome::~Skydome() { delete model_; model_ = nullptr; }
