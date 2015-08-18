#include "wiInputManager.h"
#include "wiXInput.h"
#include "wiDirectInput.h"

XInput* wiInputManager::xinput=nullptr;
DirectInput* wiInputManager::dinput=nullptr;
wiInputManager::InputCollection wiInputManager::inputs;


#define KEY_DOWN(vk_code) (GetAsyncKeyState(vk_code) < 0)
#define KEY_UP(vk_code) (!KEY_DOWN(vk_code))
#define KEY_TOGGLE(vk_code) ((GetAsyncKeyState(vk_code) & 1) != 0)


void wiInputManager::addXInput(XInput* input){
	xinput=input;
}
void wiInputManager::addDirectInput(DirectInput* input){
	dinput=input;
}

void wiInputManager::CleanUp(){
	xinput->CleanUp();
	xinput=nullptr;

	dinput->Shutdown();
	dinput=nullptr;
}

void wiInputManager::Update(){
	if(dinput){
		dinput->Frame();
	}
	if(xinput){
		xinput->UpdateControllerState();
	}

	for(InputCollection::iterator iter=inputs.begin();iter!=inputs.end();){
		InputType type = iter->first.type;
		DWORD button = iter->first.button;
		short playerIndex = iter->first.playerIndex;

		bool todelete = false;

		switch (type)
		{
		case wiInputManager::DIRECTINPUT_JOYPAD:
			if(dinput!=nullptr && dinput->isButtonDown(playerIndex,button)){
				iter->second++;
			}
			else{
				todelete=true;
			}
			break;
		case wiInputManager::XINPUT_JOYPAD:
			if(xinput!=nullptr && xinput->isButtonDown(playerIndex,button)){
				iter->second++;
			}
			else{
				todelete=true;
			}
			break;
		case wiInputManager::KEYBOARD:
			if(dinput!=nullptr && dinput->IsKeyDown(button)){
				iter->second++;
			}
			else{
				todelete=true;
			}
			break;
		default:
			break;
		}

		if(todelete){
			inputs.erase(iter++);
		}
		else{
			++iter;
		}
	}
}

bool wiInputManager::down(DWORD button, InputType inputType, short playerindex)
{
	switch (inputType)
	{
	case KEYBOARD:
		return KEY_DOWN(static_cast<int>(button)) | KEY_TOGGLE(static_cast<int>(button));
		break;
	case XINPUT_JOYPAD:
		if (xinput != nullptr && xinput->isButtonDown(playerindex, button))
			return true;
		break;
	case DIRECTINPUT_JOYPAD:
		if (xinput != nullptr && xinput->isButtonDown(playerindex, button)){
			return true;
		}
		break;
	default:break;
	}
	return false;
}
bool wiInputManager::press(DWORD button){
	return press(button,0,InputType::KEYBOARD);
}
bool wiInputManager::press(DWORD button, short playerIndex, InputType inputType){

	if (!down(button, inputType, playerIndex))
		return false;

	Input input = Input();
	input.button=button;
	input.type=inputType;
	input.playerIndex=playerIndex;
	InputCollection::iterator iter = inputs.find(input);
	if(iter==inputs.end()){
		inputs.insert(InputCollection::value_type(input,0));
		return true;
	}
	return false;
}
bool wiInputManager::hold(DWORD button, DWORD frames, bool continuous){
	return hold(button, frames, 0, continuous, InputType::KEYBOARD);
}
bool wiInputManager::hold(DWORD button, DWORD frames, short playerIndex, bool continuous, InputType inputType){

	if (!down(button, inputType, playerIndex))
		return false;

	Input input = Input();
	input.button=button;
	input.type=inputType;
	input.playerIndex=playerIndex;
	InputCollection::iterator iter = inputs.find(input);
	if(iter==inputs.end()){
		inputs.insert(pair<Input,DWORD>(input,0));
		return false;
	}
	else if ((!continuous && iter->second == frames) || (continuous && iter->second >= frames)){
		return true;
	}
	return false;
}