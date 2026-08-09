#pragma once
#include "KamataEngine.h"

class Skydome {
public:
	~Skydome();
	void Initialize();
	void Update(const KamataEngine::Camera& camera);
	void Draw(const KamataEngine::Camera& camera);

private:
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* model_ = nullptr;
};
