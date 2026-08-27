#include "DialogueSystem.h"
#include "GamepadInput.h"
#include <algorithm>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

void DialogueSystem::Initialize(
    uint32_t pageCount,
    const std::vector<std::string>& spriteFiles,
    float screenHeightRatio,
    Vector2 textureCropBase,
    Vector2 textureCropSize) {
	for (Sprite* sprite : pageSprites_) { delete sprite; }
	for (Sprite* sprite : contentSprites_) { delete sprite; }
	for (Sprite*& indicator : advanceIndicators_) {
		delete indicator;
		indicator = nullptr;
	}
	pageSprites_.clear();
	pageBaseColors_.clear();
	contentSprites_.clear();
	contentDisplaySize_ = {};
	contentOffset_ = {};

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
	const uint32_t indicatorTexture = TextureManager::Load("ui/dialogue/advance_circle.png");
	for (std::size_t index = 0; index < advanceIndicators_.size(); ++index) {
		const float horizontalOffset =
		    (static_cast<float>(index) - 1.0f) * kAdvanceIndicatorSpacing;
		advanceIndicators_[index] = Sprite::Create(
		    indicatorTexture,
		    {windowWidth - kAdvanceIndicatorRightPadding + horizontalOffset,
		     windowHeight - kAdvanceIndicatorBottomPadding},
		    {1.0f, 1.0f, 1.0f, 0.0f}, {0.5f, 0.5f});
		advanceIndicators_[index]->SetSize({kAdvanceIndicatorSize, kAdvanceIndicatorSize});
	}

	currentPage_ = 0;
	phaseTimer_ = 0.0f;
	advanceIndicatorTimer_ = 0.0f;
	currentVisibility_ = 0.0f;
	pageStarted_ = false;
	phase_ = Phase::kIdle;
}

void DialogueSystem::SetPageContentSprites(
    const std::vector<std::string>& spriteFiles, Vector2 displaySize, Vector2 offset) {
	for (Sprite* sprite : contentSprites_) { delete sprite; }
	contentSprites_.clear();
	contentDisplaySize_ = displaySize;
	contentOffset_ = offset;

	const float windowWidth = static_cast<float>(WinApp::kWindowWidth);
	const float windowHeight = static_cast<float>(WinApp::kWindowHeight);
	const float pageCenterY = windowHeight - windowHeight * screenHeightRatio_ * 0.5f;
	contentSprites_.resize(pageSprites_.size(), nullptr);
	for (size_t index = 0; index < contentSprites_.size(); ++index) {
		if (index >= spriteFiles.size() || spriteFiles[index].empty()) { continue; }
		const uint32_t textureHandle = TextureManager::Load(spriteFiles[index]);
		Sprite* sprite = Sprite::Create(
		    textureHandle,
		    {windowWidth * 0.5f + contentOffset_.x, pageCenterY + contentOffset_.y},
		    {1.0f, 1.0f, 1.0f, 0.0f},
		    {0.5f, 0.5f});
		sprite->SetSize(contentDisplaySize_);
		contentSprites_[index] = sprite;
	}
	if (static_cast<size_t>(currentPage_) < pageSprites_.size()) {
		ApplyCurrentPageVisual(currentVisibility_);
	}
}

void DialogueSystem::Start() {
	currentPage_ = 0;
	pageStarted_ = false;
	phaseTimer_ = 0.0f;
	advanceIndicatorTimer_ = 0.0f;
	for (size_t index = 0; index < pageSprites_.size(); ++index) {
		const Vector4& baseColor = pageBaseColors_[index];
		pageSprites_[index]->SetColor({baseColor.x, baseColor.y, baseColor.z, 0.0f});
		if (index < contentSprites_.size() && contentSprites_[index] != nullptr) {
			contentSprites_[index]->SetColor({1.0f, 1.0f, 1.0f, 0.0f});
		}
	}
	if (pageSprites_.empty()) {
		phase_ = Phase::kFinished;
		return;
	}
	phase_ = Phase::kAppearing;
	pageStarted_ = true;
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
		advanceIndicatorTimer_ += kFrameTime;
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
				pageStarted_ = true;
				ApplyCurrentPageVisual(0.0f);
			}
		}
		break;
	}
	}
	UpdateAdvanceIndicatorVisual();
}

bool DialogueSystem::ConsumePageStarted() {
	const bool started = pageStarted_;
	pageStarted_ = false;
	return started;
}

void DialogueSystem::ApplyCurrentPageVisual(float visibility) {
	if (static_cast<size_t>(currentPage_) >= pageSprites_.size()) { return; }
	visibility = std::clamp(visibility, 0.0f, 1.0f);
	currentVisibility_ = visibility;
	const float windowHeight = static_cast<float>(WinApp::kWindowHeight);
	const float pageHeight = windowHeight * screenHeightRatio_;
	const Vector4& baseColor = pageBaseColors_[currentPage_];
	const float slideOffset = kSlideDistance * (1.0f - visibility);
	pageSprites_[currentPage_]->SetPosition({0.0f, windowHeight - pageHeight + slideOffset});
	pageSprites_[currentPage_]->SetColor({baseColor.x, baseColor.y, baseColor.z, baseColor.w * opacity_ * visibility});
	if (static_cast<size_t>(currentPage_) < contentSprites_.size() && contentSprites_[currentPage_] != nullptr) {
		contentSprites_[currentPage_]->SetPosition({
		    static_cast<float>(WinApp::kWindowWidth) * 0.5f + contentOffset_.x,
		    windowHeight - pageHeight * 0.5f + contentOffset_.y + slideOffset});
		contentSprites_[currentPage_]->SetColor({1.0f, 1.0f, 1.0f, opacity_ * visibility});
	}
}

void DialogueSystem::UpdateAdvanceIndicatorVisual() {
	if (phase_ != Phase::kWaiting) {
		for (Sprite* indicator : advanceIndicators_) {
			if (indicator != nullptr) { indicator->SetColor({1.0f, 1.0f, 1.0f, 0.0f}); }
		}
		return;
	}

	const float cycle = (std::max)(kAdvanceIndicatorCycleDuration, kFrameTime);
	const float basePhase =
	    advanceIndicatorTimer_ / cycle * 2.0f * std::numbers::pi_v<float>;
	for (std::size_t index = 0; index < advanceIndicators_.size(); ++index) {
		Sprite* indicator = advanceIndicators_[index];
		if (indicator == nullptr) { continue; }
		const float phaseOffset =
		    static_cast<float>(index) / static_cast<float>(advanceIndicators_.size()) *
		    2.0f * std::numbers::pi_v<float>;
		const float jump = (std::max)(0.0f, std::sin(basePhase - phaseOffset));
		const float horizontalOffset =
		    (static_cast<float>(index) - 1.0f) * kAdvanceIndicatorSpacing;
		indicator->SetPosition({
		    static_cast<float>(WinApp::kWindowWidth) - kAdvanceIndicatorRightPadding +
		        horizontalOffset,
		    static_cast<float>(WinApp::kWindowHeight) - kAdvanceIndicatorBottomPadding -
		        jump * kAdvanceIndicatorMoveAmount});
		const float indicatorSize = kAdvanceIndicatorSize * (1.0f + jump * 0.10f);
		indicator->SetSize({indicatorSize, indicatorSize});
		indicator->SetColor({1.0f, 1.0f, 1.0f, opacity_ * (0.72f + jump * 0.28f)});
	}
}

void DialogueSystem::SetOpacity(float opacity) {
	opacity_ = std::clamp(opacity, 0.0f, 1.0f);
	if (static_cast<size_t>(currentPage_) < pageSprites_.size()) { ApplyCurrentPageVisual(currentVisibility_); }
	UpdateAdvanceIndicatorVisual();
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
	if (static_cast<size_t>(currentPage_) < contentSprites_.size() && contentSprites_[currentPage_] != nullptr) {
		contentSprites_[currentPage_]->Draw();
	}
	if (phase_ == Phase::kWaiting) {
		for (Sprite* indicator : advanceIndicators_) {
			if (indicator != nullptr) { indicator->Draw(); }
		}
	}
	Sprite::PostDraw();
}

DialogueSystem::~DialogueSystem() {
	for (Sprite* sprite : contentSprites_) { delete sprite; }
	contentSprites_.clear();
	for (Sprite* sprite : pageSprites_) { delete sprite; }
	pageSprites_.clear();
	for (Sprite*& indicator : advanceIndicators_) {
		delete indicator;
		indicator = nullptr;
	}
}
