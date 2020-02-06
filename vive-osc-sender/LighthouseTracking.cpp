//
// HTC Vive Lighthouse Tracking Example
// By Peter Thor 2016, 2017, 2018, 2019
//
// Shows how to extract basic tracking data

#include "stdafx.h"
#include "LighthouseTracking.h"
#include <filesystem>
using namespace std::experimental::filesystem::v1;

// Destructor
LighthouseTracking::~LighthouseTracking() {
	if (m_pHMD != NULL)
	{
		vr::VR_Shutdown();
		m_pHMD = NULL;
	}
}

// Constructor
LighthouseTracking::LighthouseTracking(IpEndpointName ip)
	: transmitSocket(ip){
	vr::EVRInitError eError = vr::VRInitError_None;
	m_pHMD = vr::VR_Init(&eError, vr::VRApplication_Background);
	char buf[1024];

	if (eError != vr::VRInitError_None)
	{
		m_pHMD = NULL;
		sprintf_s(buf, sizeof(buf), "Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
		printf_s(buf);
		exit(EXIT_FAILURE);
	}

	char buffer[1024];
	osc::OutboundPacketStream p(buffer,1024);

	p << osc::BeginBundleImmediate
		<< osc::BeginMessage("/notice")
		<< "vive-osc-sender launched"
		<< osc::EndMessage << osc::EndBundle;
	transmitSocket.Send(p.Data(), p.Size());
}

/*
* Loop-listen for events then parses them (e.g. prints the to user)
* Supply a filterIndex other than -1 to show only info for that device in question. e.g. 0 is always the hmd.
* Returns true if success or false if openvr has quit
*/
bool LighthouseTracking::RunProcedure() {

	//ParseTrackingFrame(filterIndex);
    ParseTrackingFrame();

    vr::VREvent_t event;
    while (m_pHMD->PollNextEvent(&event, sizeof(event))) {
        if (!ProcessVREvent(event)) {
            char buf[1024];
            sprintf_s(buf, sizeof(buf), "(OpenVR) service quit\n");
            printf_s(buf);
            return false;
        }
    }

	return true;
}

/*
* Parse a Frame with data from the tracking system
*
* Handy reference:
* https://github.com/TomorrowTodayLabs/NewtonVR/blob/master/Assets/SteamVR/Scripts/SteamVR_Utils.cs
*
* Also:
* Open VR Convention (same as OpenGL)
* right-handed system
* +y is up
* +x is to the right
* -z is going away from you
* http://www.3dgep.com/understanding-the-view-matrix/
*
*/
void LighthouseTracking::ParseTrackingFrame() {
    // Iterate over devices
    int trackersFound = 0;
    int controllersFound = 0;
    printf_s("\r");
    for (vr::TrackedDeviceIndex_t i = 0; i < vr::k_unMaxTrackedDeviceCount; i++)
    {
        bool deviceConnected = m_pHMD->IsTrackedDeviceConnected(i);
        m_deviceConnected[i] = deviceConnected;
        if (!deviceConnected) continue;

        vr::TrackedDevicePose_t *devicePose = &m_rTrackedDevicePose[i];
        vr::VRControllerState_t controllerState;
        vr::VRControllerState_t *controllerState_ptr = &controllerState;
        vr::VRSystem()->GetControllerStateWithPose(vr::TrackingUniverseRawAndUncalibrated, i, controllerState_ptr, sizeof(controllerState), devicePose);
        float trigger = 0;

        // Get what type of device it is and work with its data
        vr::ETrackedDeviceClass trackedDeviceClass = vr::VRSystem()->GetTrackedDeviceClass(i);
        switch (trackedDeviceClass) {
        case vr::ETrackedDeviceClass::TrackedDeviceClass_HMD:
            break;
        case vr::ETrackedDeviceClass::TrackedDeviceClass_Controller:
            controllersFound++;
            // get controller axis
            if (devicePose->bPoseIsValid) {
                trigger = controllerState.rAxis[1].x;
            }
            // For now, I don't care about the role, but I may want it later
            switch (vr::VRSystem()->GetControllerRoleForTrackedDeviceIndex(i)) {
            case vr::TrackedControllerRole_Invalid:
                break;
            case vr::TrackedControllerRole_LeftHand:
                break;
            case vr::TrackedControllerRole_RightHand:
                break;
            case vr::TrackedControllerRole_Treadmill:
                break;
            }
            break;

        case vr::ETrackedDeviceClass::TrackedDeviceClass_GenericTracker:
            trackersFound++;
            break;
        case vr::ETrackedDeviceClass::TrackedDeviceClass_TrackingReference:
            break;
        case vr::ETrackedDeviceClass::TrackedDeviceClass_DisplayRedirect:
            break;
        case vr::ETrackedDeviceClass::TrackedDeviceClass_Invalid:
            break;
        }

        // decide what do do with the pose
        {
            if (!devicePose->bPoseIsValid || devicePose->eTrackingResult != vr::ETrackingResult::TrackingResult_Running_OK) continue;
            vr::HmdVector3_t position = GetPosition(devicePose->mDeviceToAbsoluteTracking);
            vr::HmdQuaternion_t quaternion = GetRotation(devicePose->mDeviceToAbsoluteTracking);

            vr::HmdVector3_t vVel = devicePose->vVelocity;
            vr::HmdVector3_t vAngVel = devicePose->vAngularVelocity;

            char oscAddress[1024];
            switch (trackedDeviceClass) {
            case vr::TrackedDeviceClass_Controller:
                sprintf_s(oscAddress, sizeof(oscAddress), "/controller/%d", controllersFound);
                break;
            case vr::TrackedDeviceClass_GenericTracker:
                sprintf_s(oscAddress, sizeof(oscAddress), "/tracker/%d", trackersFound);
                break;
            default:
                sprintf_s(oscAddress, sizeof(oscAddress), "/other");
                break;
            }

            // Create and send OSC message
            char oscBuffer[2048];
            osc::OutboundPacketStream pStream(oscBuffer, 2048);
            pStream << osc::BeginMessage(oscAddress);
            pStream << position.v[0] << position.v[1] << position.v[2]
                << static_cast<float>(quaternion.w) << static_cast<float>(quaternion.x)
                << static_cast<float>(quaternion.y) << static_cast<float>(quaternion.z);

            if (trackedDeviceClass == vr::TrackedDeviceClass_Controller)
                pStream << trigger;

            pStream << osc::EndMessage;

            transmitSocket.Send(pStream.Data(), pStream.Size());

            // Print stuff
            char type = 'T';
            switch (trackedDeviceClass) {
            case vr::TrackedDeviceClass_Controller:
                type = 'C';
            case vr::TrackedDeviceClass_GenericTracker:
                printf_s("%c(% .2f,  % .2f, % .2f) q(% .2f, % .2f, % .2f, % .2f) - ", type, position.v[0], position.v[1], position.v[2], quaternion.w, quaternion.x, quaternion.y, quaternion.z);
                break;
            }
        }
    }
}

void LighthouseTracking::PrintDevices() {

	printf_s("\nDevice list:\n---------------------------\n");

	// Process SteamVR device states
	for (vr::TrackedDeviceIndex_t unDevice = 0; unDevice < vr::k_unMaxTrackedDeviceCount; unDevice++)
	{
		if (!m_pHMD->IsTrackedDeviceConnected(unDevice))
			continue;

		vr::TrackedDevicePose_t trackedDevicePose;
		vr::TrackedDevicePose_t *devicePose = &trackedDevicePose;

		vr::VRControllerState_t controllerState;
		vr::VRControllerState_t *ontrollerState_ptr = &controllerState;

		// Get what type of device it is and work with its data
		vr::ETrackedDeviceClass trackedDeviceClass = vr::VRSystem()->GetTrackedDeviceClass(unDevice);
        switch (trackedDeviceClass) {
        case vr::ETrackedDeviceClass::TrackedDeviceClass_HMD: { printf_s("Device %d: [HMD]", unDevice); } break;
        case vr::ETrackedDeviceClass::TrackedDeviceClass_Controller: {
            switch (vr::VRSystem()->GetControllerRoleForTrackedDeviceIndex(unDevice)) {
            case vr::TrackedControllerRole_Invalid: printf_s("Device %d: [Invalid Controller]", unDevice); break;
            case vr::TrackedControllerRole_LeftHand: printf_s("Device %d: [Controller - Left]", unDevice); break;
            case vr::TrackedControllerRole_RightHand: printf_s("Device %d: [Controller - Right]", unDevice); break;
            case vr::TrackedControllerRole_Treadmill: printf_s("Device %d: [Treadmill]", unDevice); break;
            } break;
        } break; // Controller

        case vr::ETrackedDeviceClass::TrackedDeviceClass_GenericTracker: { printf_s("Device %d: [GenericTracker]", unDevice); } break;
        case vr::ETrackedDeviceClass::TrackedDeviceClass_TrackingReference: { printf_s("Device %d: [TrackingReference]", unDevice); } break;
        case vr::ETrackedDeviceClass::TrackedDeviceClass_DisplayRedirect: { printf_s("Device %d: [DisplayRedirect]", unDevice); } break;
        case vr::ETrackedDeviceClass::TrackedDeviceClass_Invalid: { printf_s("Device %d: [Invalid]", unDevice); } break;
		}

		char manufacturer[64];
		vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, vr::ETrackedDeviceProperty::Prop_ManufacturerName_String, manufacturer, sizeof(manufacturer));

		char modelnumber[64];
		vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, vr::ETrackedDeviceProperty::Prop_ModelNumber_String, modelnumber, sizeof(modelnumber));

		char serialnumber[64];
		vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, vr::ETrackedDeviceProperty::Prop_SerialNumber_String, serialnumber, sizeof(serialnumber));

		printf_s(" %s - %s [%s] class(%d)\n", manufacturer, modelnumber, serialnumber, trackedDeviceClass);

        // If the device is a controller, print each axis type
        if (trackedDeviceClass == vr::ETrackedDeviceClass::TrackedDeviceClass_Controller) {
            for (int j = 0; j < vr::k_unControllerStateAxisCount; j++) {
                auto axis = static_cast<vr::ETrackedDeviceProperty>(j + vr::Prop_Axis0Type_Int32);
                vr::TrackedPropertyError error;
                auto enumAxis = static_cast<vr::EVRControllerAxisType> (
                    vr::VRSystem()->GetInt32TrackedDeviceProperty(unDevice, axis, &error)
                    );
                if (!error) {
                    printf_s("\taxis: %d - type: %d (%s) \n", j, enumAxis, vr::VRSystem()->GetControllerAxisTypeNameFromEnum(enumAxis));
                }

            }
        }
	}
	printf_s("---------------------------\n\n");
}


//-----------------------------------------------------------------------------
// Purpose: Processes a single VR event
//-----------------------------------------------------------------------------

bool LighthouseTracking::ProcessVREvent(const vr::VREvent_t & event) {
    switch (event.eventType)
    {
    case vr::VREvent_TrackedDeviceActivated: { printf_s("(OpenVR) Device : %d attached\n", event.trackedDeviceIndex); } break;
    case vr::VREvent_TrackedDeviceDeactivated: { printf_s("(OpenVR) Device : %d detached\n", event.trackedDeviceIndex); } break;
    case vr::VREvent_TrackedDeviceUpdated: { printf_s("(OpenVR) Device : %d updated\n", event.trackedDeviceIndex); } break;
    case vr::VREvent_DashboardActivated: { printf_s("(OpenVR) Dashboard activated\n"); } break;
    case vr::VREvent_DashboardDeactivated: { printf_s("(OpenVR) Dashboard deactivated\n"); } break;
    case vr::VREvent_ChaperoneDataHasChanged: { printf_s("(OpenVR) Chaperone data has changed\n"); } break;
    case vr::VREvent_ChaperoneSettingsHaveChanged: { printf_s("(OpenVR) Chaperone settings have changed\n"); } break;
    case vr::VREvent_ChaperoneUniverseHasChanged: { printf_s("(OpenVR) Chaperone universe has changed\n"); } break;
    case vr::VREvent_ApplicationTransitionStarted: { printf_s("(OpenVR) Application Transition: Transition has started\n"); } break;
    case vr::VREvent_ApplicationTransitionNewAppStarted: { printf_s("(OpenVR) Application transition: New app has started\n"); } break;
    case vr::VREvent_TrackedDeviceRoleChanged: { printf_s("(OpenVR) TrackedDeviceRoleChanged: %d\n", event.trackedDeviceIndex); } break;
    case vr::VREvent_Input_HapticVibration: { printf_s("(OpenVR) VREvent_Input_HapticVibration\n"); } break;
    case vr::VREvent_Input_BindingLoadFailed: { printf_s("(OpenVR) VREvent_Input_BindingLoadFailed\n"); } break;
    case vr::VREvent_Input_BindingLoadSuccessful: { printf_s("(OpenVR) VREvent_Input_BindingLoadSuccessful\n"); } break;
    case vr::VREvent_Input_ActionManifestReloaded: { printf_s("(OpenVR) VREvent_Input_ActionManifestReloaded\n"); } break;
    case vr::VREvent_Input_ActionManifestLoadFailed: { printf_s("(OpenVR) VREvent_Input_ActionManifestLoadFailed\n"); } break;
    case vr::VREvent_Input_ProgressUpdate: { printf_s("(OpenVR) VREvent_Input_ProgressUpdate\n"); } break;
    case vr::VREvent_Input_TrackerActivated: { printf_s("(OpenVR) VREvent_Input_TrackerActivated\n"); } break;
    case vr::VREvent_Input_BindingsUpdated: { printf_s("(OpenVR) VREvent_Input_BindingsUpdated\n"); } break;
    case vr::VREvent_ActionBindingReloaded: { printf_s("(OpenVR) VREvent_ActionBindingReloaded\n"); } break;
    case vr::VREvent_ChaperoneFlushCache: { printf_s("(OpenVR) VREvent_ChaperoneFlushCache\n"); } break;
    case vr::VREvent_ButtonTouch: { printf_s("(OpenVR) Event: Touch Device: %d\n", event.trackedDeviceIndex); } break;
    case vr::VREvent_ButtonUntouch: { printf_s("(OpenVR) Event: Untouch Device: %d\n", event.trackedDeviceIndex); } break;
    case vr::VREvent_ButtonPress: { printf_s("(OpenVR) Event: Press Device: %d\n", event.trackedDeviceIndex); } break;
    case vr::VREvent_ButtonUnpress: { printf_s("(OpenVR) Event: Release Device: %d\n", event.trackedDeviceIndex); } break;
    case vr::VREvent_EnterStandbyMode: { printf_s("(OpenVR) Event: Enter StandbyMode: %d\n", event.trackedDeviceIndex); } break;
    case vr::VREvent_LeaveStandbyMode: { printf_s("(OpenVR) Event: Leave StandbyMode: %d\n", event.trackedDeviceIndex); } break;
    case vr::VREvent_PropertyChanged: { printf_s("(OpenVR) Event: Property Changed Device: %d ETrackedDeviceProperty(%d)\n", event.trackedDeviceIndex, event.data.property.prop); } break;
    case vr::VREvent_SceneApplicationChanged: { printf_s("(OpenVR) Event: Scene Application Changed\n"); } break;
    case vr::VREvent_SceneFocusChanged: { printf_s("(OpenVR) Event: Scene Focus Changed\n"); } break;
    case vr::VREvent_TrackedDeviceUserInteractionStarted: { printf_s("(OpenVR) Event: Tracked Device User Interaction Started Device: %d\n", event.trackedDeviceIndex); } break;
    case vr::VREvent_TrackedDeviceUserInteractionEnded: { printf_s("(OpenVR) Event: Tracked Device User Interaction Ended Device: %d\n", event.trackedDeviceIndex); } break;
    case vr::VREvent_ProcessDisconnected: { printf_s("(OpenVR) Event: A process was disconnected\n"); } break;
    case vr::VREvent_ProcessConnected: { printf_s("(OpenVR) Event: A process was connected\n"); } break;
    case (vr::VREvent_StatusUpdate): {
        char* status = "unkown";
        switch (event.data.status.statusState) {
        case(vr::EVRState::VRState_NotReady): { status = "not Ready"; } break;
        case(vr::EVRState::VRState_Off): { status = "off"; } break;
        case(vr::EVRState::VRState_Ready): { status = "ready"; } break;
        case(vr::EVRState::VRState_Ready_Alert): { status = "ready alert"; } break;
        case(vr::EVRState::VRState_Ready_Alert_Low): { status = "ready alert low"; } break;
        case(vr::EVRState::VRState_Searching): { status = "searching"; } break;
        case(vr::EVRState::VRState_Searching_Alert): { status = "searching alert"; } break;
        case(vr::EVRState::VRState_Standby): { status = "standby"; } break;
        case(vr::EVRState::VRState_Undefined): { status = "undefined"; } break;
        }
        printf_s("(OpenVR) Device %d status: %s\n", event.trackedDeviceIndex, status);
    } break;

    case (vr::VREvent_Quit): {
        printf_s("(OpenVR) Received SteamVR Quit (%d)\n", vr::VREvent_Quit);
        return false;
    } break;

    case (vr::VREvent_ProcessQuit): {
        printf_s("(OpenVR) SteamVR Quit Process (%d)\n", vr::VREvent_ProcessQuit);
        return false;
    } break;

    case (vr::VREvent_QuitAborted_UserPrompt): {
        printf_s("(OpenVR) SteamVR Quit Aborted UserPrompt (%d)\n", vr::VREvent_QuitAborted_UserPrompt);
        return false;
    } break;

    case (vr::VREvent_QuitAcknowledged): {
        printf_s("(OpenVR) SteamVR Quit Acknowledged (%d)\n", vr::VREvent_QuitAcknowledged);
        return false;
    } break;
 
    default: { printf_s("(OpenVR) Unmanaged Event: %d Device: %d\n", event.eventType, event.trackedDeviceIndex); } break;
    }

    return true;
}


// Get the quaternion representing the rotation
vr::HmdQuaternion_t LighthouseTracking::GetRotation(vr::HmdMatrix34_t matrix) {
    vr::HmdQuaternion_t q;

    q.w = sqrt(fmax(0, 1 + matrix.m[0][0] + matrix.m[1][1] + matrix.m[2][2])) / 2;
    q.x = sqrt(fmax(0, 1 + matrix.m[0][0] - matrix.m[1][1] - matrix.m[2][2])) / 2;
    q.y = sqrt(fmax(0, 1 - matrix.m[0][0] + matrix.m[1][1] - matrix.m[2][2])) / 2;
    q.z = sqrt(fmax(0, 1 - matrix.m[0][0] - matrix.m[1][1] + matrix.m[2][2])) / 2;
    q.x = copysign(q.x, matrix.m[2][1] - matrix.m[1][2]);
    q.y = copysign(q.y, matrix.m[0][2] - matrix.m[2][0]);
    q.z = copysign(q.z, matrix.m[1][0] - matrix.m[0][1]);
    return q;
}

// Get the vector representing the position
vr::HmdVector3_t LighthouseTracking::GetPosition(vr::HmdMatrix34_t matrix) {
    vr::HmdVector3_t vector;

    vector.v[0] = matrix.m[0][3];
    vector.v[1] = matrix.m[1][3];
    vector.v[2] = matrix.m[2][3];

    return vector;
}

void LighthouseTracking::StoreLeftControllerPosition(vr::HmdVector3_t vector) {
    m_vecLeftController.v[0] = vector.v[0];
    m_vecLeftController.v[1] = vector.v[1];
    m_vecLeftController.v[2] = vector.v[2];
}
void LighthouseTracking::StoreRightControllerPosition(vr::HmdVector3_t vector) {
    m_vecRightController.v[0] = vector.v[0];
    m_vecRightController.v[1] = vector.v[1];
    m_vecRightController.v[2] = vector.v[2];
}

vr::HmdVector3_t LighthouseTracking::GetLeftControllerPosition() {
    return m_vecLeftController;
}

vr::HmdVector3_t LighthouseTracking::GetRightControllerPosition() {
    return m_vecRightController;
}


vr::HmdVector3_t LighthouseTracking::GetControllerPositionDelta() {
    vr::HmdVector3_t vecDiff;
    vecDiff.v[0] = m_vecLeftController.v[0] - m_vecRightController.v[0];
    vecDiff.v[1] = m_vecLeftController.v[1] - m_vecRightController.v[1];
    vecDiff.v[2] = m_vecLeftController.v[2] - m_vecRightController.v[2];
    return vecDiff;
}
