#include "DialogueSystem.h"
#include "GamepadInput.h"
#include <algorithm>

using namespace KamataEngine;

void DialogueSystem::Initialize(
    uint32_t pageCount,
    const std::vector<std::string>& spriteFiles,
    float screenHeightRatio,
    Vector2 textureCropBase,
    Vector2 textureCropSize) {
	for (Sprite* sprite : pageSprites_) { delete sprite; }
	pageSprites_.clear();
	pageBaseColors_.clear();

	const float windowWidth = static_cast<float>(WinApp::kWindowWidth);
	const float windowHeight = static_cast<float>(WinApp::kWindowHeight);
	screenHeightRatio_ = std::clamp(screenHeightRatio, 0.05f, 1.0f);
	const Vector2 pageSize = {windowWidth, windowHeight * screenHeightRatio_};
	const Vector2 pagePosition = {0.0f, windowHeight - pageSize.y};
	for (uint32_t index = 0; index < pageCount; ++index) {
		const size_t spriteIndex = static_cast<size_t>(index);
		const bool hasCustomSprite = spriteIndex < spriteFiles.size() && !spriteFiles[spriteIndex].empty();
		const uint32_t textureHandle = TextureManager::Load(hasCustomSprite ? spriteFiles[spriteIndex] : "white1x1.png");
		const Vector4 baseColor = hasCustomSprite ? Vector4{1.0f, 1.0f, 1.0f, 1.0f} : Vector4{0.035f, 0.045f, 0.060f, 0.94f};
		Sprite* pageSprite = Sprite::Create(textureHandle, pagePosition, {baseColor.x, baseColor.y, baseColor.z, 0.0f});
		if (hasCustomSprite && textureCropSize.x > 0.0f && textureCropSize.y > 0.0f) {
			pageSprite->SetTextureRect(textureCropBase, textureCropSize);
		}
		pageSprite->SetSize(pageSize);
		pageSprites_.push_back(pageSprite);
		pageBaseColors_.push_back(baseColor);
	}

	currentPage_ = 0;
	phaseTimer_ = 0.0f;
	currentVisibility_ = 0.0f;
	phase_ = Phase::kIdle;
}

void DialogueSystem::Start() {
	currentPage_ = 0;
	phaseTimer_ = 0.0f;
	for (size_t index = 0; index < pageSprites_.size(); ++index) {
		const Vector4& baseColor = pageBaseColors_[index];
		pageSprites_[index]->SetColor({baseColor.x, baseColor.y, baseColor.z, 0.0f});
	}
	if (pageSprites_.empty()) {
		phase_ = Phase::kFinished;
		return;
	}
	phase_ = Phase::kAppearing;
	ApplyCurrentPageVisual(0.0f);
}

void DialogueSystem::Update() {
	switch (phase_) {
	case Phase::kIdle:
	case Phase::kFinished:
		return;
	case Phase::kAppearing: {
		phaseTimer_ = (std::min)(phaseTimer_ + kFrameTime, kAppearDuration);
		const float visibility = SmoothStep(phaseTimer_ / kAppearDuration);
		ApplyCurrentPageVisual(visibility);
		if (phaseTimer_ >= kAppearDuration) {
			phase_ = Phase::kWaiting;
			phaseTimer_ = 0.0f;
		}
		break;
	}
	case Phase::kWaiting:
		if (Input::GetInstance()->TriggerKey(DIK_SPACE) || Input::GetInstance()->IsTriggerMouse(0) ||
		    GamepadInput::IsTriggered(GamepadInput::ReadPlayerOne(), XINPUT_GAMEPAD_A)) {
			phase_ = Phase::kDisappearing;
			phaseTimer_ = 0.0f;
		}
		break;
	case Phase::kDisappearing: {
		phaseTimer_ = (std::min)(phaseTimer_ + kFrameTime, kDisappearDuration);
		const float visibility = 1.0f - SmoothStep(phaseTimer_ / kDisappearDuration);
		ApplyCurrentPageVisual(visibility);
		if (phaseTimer_ >= kDisappearDuration) {
			++currentPage_;
			phaseTimer_ = 0.0f;
			if (static_cast<size_t>(currentPage_) >= pageSprites_.size()) {
				phase_ = Phase::kFinished;
			} else {
				phase_ = Phase::kAppearing;
				ApplyCurrentPageVisual(0.0f);
			}
		}
		break;
	}
	}
}

void DialogueSystem::ApplyCurrentPageVisual(float visibility) {
	if (static_cast<size_t>(currentPage_) >= pageSprites_.size()) { return; }
	visibility = std::clamp(visibility, 0.0f, 1.0f);
	currentVisibility_ = visibility;
	const float windowHeight = static_cast<float>(WinApp::kWindowHeight);
	const float pageHeight = windowHeight * screenHeightRatio_;
	const Vector4& baseColor = pageBaseColors_[currentPage_];
	pageSprites_[currentPage_]->SetPosition({0.0f, windowHeight - pageHeight + kSlideDistance * (1.0f - visibility)});
	pageSprites_[currentPage_]->SetColor({baseColor.x, baseColor.y, baseColor.z, baseColor.w * opacity_ * visibility});
}

void DialogueSystem::SetOpacity(float opacity) {
	opacity_ = std::clamp(opacity, 0.0f, 1.0f);
	if (static_cast<size_t>(currentPage_) < pageSprites_.size()) { ApplyCurrentPageVisual(currentVisibility_); }
}

void DialogueSystem::SetBaseColor(const Vector4& color) {
	for (Vector4& baseColor : pageBaseColors_) { baseColor = color; }
	if (static_cast<size_t>(currentPage_) < pageSprites_.size()) { ApplyCurrentPageVisual(currentVisibility_); }
}

float DialogueSystem::SmoothStep(float t) {
	t = std::clamp(t, 0.0f, 1.0f);
	return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

bool DialogueSystem::IsActive() const {
	return phase_ != Phase::kIdle && phase_ != Phase::kFinished;
}

void DialogueSystem::Draw() const {
	if (!IsActive() || static_cast<size_t>(currentPage_) >= pageSprites_.size()) { return; }
	Sprite::PreDraw();
	pageSprites_[currentPage_]->Draw();
	Sprite::PostDraw();
}

DialogueSystem::~DialogueSystem() {
	for (Sprite* sprite : pageSprites_) { delete sprite; }
	pageSprites_.clear();
}
