#include <Windows.h>
#include "GameScene.h"
#include "KamataEngine.h"
#include <memory>

using namespace KamataEngine;

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	Initialize(L"Summer Game");
	DirectXCommon* directXCommon = DirectXCommon::GetInstance();
#ifdef USE_IMGUI
	ImGuiManager* imGuiManager = ImGuiManager::GetInstance();
#endif
	{
		auto gameScene = std::make_unique<GameScene>();
		gameScene->Initialize();
		while (!Update()) {
#ifdef USE_IMGUI
			imGuiManager->Begin();
#endif
			gameScene->Update();
			directXCommon->PreDraw();
			gameScene->Draw();
#ifdef USE_IMGUI
			imGuiManager->End();
			imGuiManager->Draw();
#endif
			directXCommon->PostDraw();
			if (gameScene->ShouldRestartToTitle()) {
				gameScene = std::make_unique<GameScene>();
				gameScene->Initialize();
			}
		}
	}
	Finalize();
	return 0;
}
