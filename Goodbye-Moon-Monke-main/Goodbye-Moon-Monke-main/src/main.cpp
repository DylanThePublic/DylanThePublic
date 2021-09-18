#include "modloader/shared/modloader.hpp"
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/RaycastHit.hpp"
#include "custom-types/shared/register.hpp"
#include "UnityEngine/Rigidbody.hpp"
#include "UnityEngine/Camera.hpp"
#include "UnityEngine/Collider.hpp"
#include "UnityEngine/CapsuleCollider.hpp"
#include "UnityEngine/SphereCollider.hpp"
#include "UnityEngine/GameObject.hpp"
#include "GorillaLocomotion/Player.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/utils/utils-functions.h"
#include "beatsaber-hook/shared/utils/typedefs.h"
#include "GlobalNamespace/OVRInput.hpp"
#include "GlobalNamespace/OVRInput_Button.hpp"
#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/ForceMode.hpp"
#include "UnityEngine/Transform.hpp"
#include "GoodbyeMoonMonkeWatchView.hpp"
#include "config.hpp"
#include "monkecomputer/shared/GorillaUI.hpp"
#include "monkecomputer/shared/Register.hpp"
#include "custom-types/shared/register.hpp"
#include "gorilla-utils/shared/GorillaUtils.hpp"
#include "gorilla-utils/shared/CustomProperties/Player.hpp"
#include "gorilla-utils/shared/Utils/Player.hpp"
#include "gorilla-utils/shared/Callbacks/InRoomCallbacks.hpp"
#include "gorilla-utils/shared/Callbacks/MatchMakingCallbacks.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"

ModInfo modInfo;

#define INFO(value...) getLogger().info(value)
#define ERROR(value...) getLogger().error(value)

using namespace UnityEngine;
using namespace UnityEngine::XR;
using namespace GorillaLocomotion;

Logger& getLogger()
{
    static Logger* logger = new Logger(modInfo, LoggerOptions(false, true));
    return *logger;
}

bool isRoom = true;
bool enabled = true;
bool lowGravModeEnabled = true;
float thrust = config.power * 1000.0;

void UpdateButton()
{

    using namespace GlobalNamespace;
    bool AInput = false;
	bool XInput = false;
    bool leftGripInput = false;
    bool rightGripInput = false;
    AInput = OVRInput::Get(OVRInput::Button::One, OVRInput::Controller::RTouch);
    XInput = OVRInput::Get(OVRInput::Button::One, OVRInput::Controller::LTouch);
    leftGripInput = OVRInput::Get(OVRInput::Button::PrimaryHandTrigger, OVRInput::Controller::LTouch);
    rightGripInput = OVRInput::Get(OVRInput::Button::PrimaryHandTrigger, OVRInput::Controller::RTouch);

    if (isRoom && config.enabled)
    {
        if (AInput && rightGripInput || XInput && leftGripInput)
        {
            enabled = true;
        }
        else
        {
            enabled = false;
        }
    }
    else return;
}

#include "GlobalNamespace/GorillaTagManager.hpp"

MAKE_HOOK_MATCH(GorillaTagManager_Update, &GlobalNamespace::GorillaTagManager::Update, void, GlobalNamespace::GorillaTagManager* self) {
    using namespace GlobalNamespace;
    using namespace GorillaLocomotion;
    GorillaTagManager_Update(self);
    UpdateButton();

    Player* playerInstance = Player::get_Instance();
    if(playerInstance == nullptr) return;

    Rigidbody* playerPhysics = playerInstance->playerRigidBody;
    if(playerPhysics == nullptr) return;

    GameObject* playerGameObject = playerPhysics->get_gameObject();
    if(playerGameObject == nullptr) return;

    if(isRoom && config.enabled) {
        if(enabled) {
            if(lowGravModeEnabled) {
                lowGravModeEnabled = false;
                playerPhysics->set_useGravity(false);
                playerPhysics->AddForce(Vector3::get_up() * thrust);
            } else if(!lowGravModeEnabled){
                lowGravModeEnabled = true;
                playerPhysics->set_useGravity(true);
            }
        } else {
            playerPhysics->set_useGravity(true);
        }
    } else {
        playerPhysics->set_useGravity(true);
    }
}

MAKE_HOOK_MATCH(Player_Awake, &GorillaLocomotion::Player::Awake, void, GorillaLocomotion::Player* self) {
    Player_Awake(self);

    GorillaUtils::MatchMakingCallbacks::onJoinedRoomEvent() += {[&]() {
        Il2CppObject* currentRoom = CRASH_UNLESS(il2cpp_utils::RunMethod("Photon.Pun", "PhotonNetwork", "get_CurrentRoom"));

        if (currentRoom)
        {
            isRoom = !CRASH_UNLESS(il2cpp_utils::RunMethod<bool>(currentRoom, "get_IsVisible"));
        } else isRoom = true;
    }
    };
}

extern "C" void setup(ModInfo& info)
{
    info.id = ID;
    info.version = VERSION;
    modInfo = info;

}

extern "C" void load()
{
    GorillaUI::Innit();

    INSTALL_HOOK(getLogger(), Player_Awake);
    INSTALL_HOOK(getLogger(), GorillaTagManager_Update);

    custom_types::Register::AutoRegister();

    GorillaUI::Register::RegisterWatchView<GoodbyeMoonMonke::GoodbyeMoonMonkeWatchView*>("Goodbye Moon Monke", VERSION);
}