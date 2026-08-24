#pragma once

#include "KamataEngine.h"
#include <cstdint>

// Digital Xbox-controller input shared by gameplay and scene UI. This helper
// intentionally reads only XInput button bits; neither analog stick affects
// movement, so D-pad movement is identical to keyboard A/D movement.
namespace GamepadInput {

struct Snapshot {
	bool connected = false;
	uint16_t heldButtons = 0;
	uint16_t triggeredButtons = 0;
};

inline Snapshot ReadPlayerOne() {
	Snapshot snapshot = {};
	KamataEngine::Input* input = KamataEngine::Input::GetInstance();
	const size_t joystickCount = input->GetNumberOfJoysticks();
	for (size_t index = 0; index < joystickCount; ++index) {
		XINPUT_STATE current = {};
		if (!input->GetJoystickState(static_cast<int32_t>(index), current)) { continue; }

		XINPUT_STATE previous = {};
		const bool hasPrevious = input->GetJoystickStatePrevious(static_cast<int32_t>(index), previous);
		snapshot.connected = true;
		snapshot.heldButtons = current.Gamepad.wButtons;
		const uint16_t previousButtons = hasPrevious ? previous.Gamepad.wButtons : 0;
		snapshot.triggeredButtons = static_cast<uint16_t>(snapshot.heldButtons & ~previousButtons);
		break;
	}
	return snapshot;
}

inline bool IsHeld(const Snapshot& snapshot, uint16_t button) {
	return (snapshot.heldButtons & button) != 0;
}

inline bool IsTriggered(const Snapshot& snapshot, uint16_t button) {
	return (snapshot.triggeredButtons & button) != 0;
}

} // namespace GamepadInput
