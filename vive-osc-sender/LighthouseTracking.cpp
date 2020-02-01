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
bool LighthouseTracking::RunProcedure(int filterIndex = -1) {

	//ParseTrackingFrame(filterIndex);
    ParseTrackingFrame();

    vr::VREvent_t event;
    while (m_pHMD->PollNextEvent(&event, sizeof(event))) {
        if (!ProcessVREvent(event, filterIndex)) {
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

	char buf[1024];
	sprintf_s(buf, sizeof(buf), "\nDevice list:\n---------------------------\n");
	printf_s(buf);

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
		case vr::ETrackedDeviceClass::TrackedDeviceClass_HMD:
			// print stuff for the HMD here, see controller stuff in next case block

			char buf[2048];
			sprintf_s(buf, sizeof(buf), "Device %d: [HMD]", unDevice);
			printf_s(buf);
			break;

		case vr::ETrackedDeviceClass::TrackedDeviceClass_Controller:
			// Simliar to the HMD case block above, please adapt as you like
			// to get away with code duplication and general confusion

			//char buf[1024];
			switch (vr::VRSystem()->GetControllerRoleForTrackedDeviceIndex(unDevice)) {
			case vr::TrackedControllerRole_Invalid:
				// invalid hand...

				sprintf_s(buf, sizeof(buf), "Device %d: [Invalid Controller]", unDevice);
				printf_s(buf);
				break;

			case vr::TrackedControllerRole_LeftHand:
				sprintf_s(buf, sizeof(buf), "Device %d: [Controller - Left]", unDevice);
				printf_s(buf);
				break;

			case vr::TrackedControllerRole_RightHand:
				sprintf_s(buf, sizeof(buf), "Device %d: [Controller - Right]", unDevice);
				printf_s(buf);
				break;

			case vr::TrackedControllerRole_Treadmill:
				sprintf_s(buf, sizeof(buf), "Device %d: [Treadmill]", unDevice);
				printf_s(buf);
				break;
			}
			break;

		case vr::ETrackedDeviceClass::TrackedDeviceClass_GenericTracker:
			// print stuff for the HMD here, see controller stuff in next case block

			sprintf_s(buf, sizeof(buf), "Device %d: [GenericTracker]", unDevice);
			printf_s(buf);
			break;

		case vr::ETrackedDeviceClass::TrackedDeviceClass_TrackingReference:
			// print stuff for the HMD here, see controller stuff in next case block

			sprintf_s(buf, sizeof(buf), "Device %d: [TrackingReference]", unDevice);
			printf_s(buf);
			break;

		case vr::ETrackedDeviceClass::TrackedDeviceClass_DisplayRedirect:
			// print stuff for the HMD here, see controller stuff in next case block

			sprintf_s(buf, sizeof(buf), "Device %d: [DisplayRedirect]", unDevice);
			printf_s(buf);
			break;

		case vr::ETrackedDeviceClass::TrackedDeviceClass_Invalid:
			// print stuff for the HMD here, see controller stuff in next case block

			sprintf_s(buf, sizeof(buf), "Device %d: [Invalid]", unDevice);
			printf_s(buf);
			break;

		}

		char manufacturer[1024];
		vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, vr::ETrackedDeviceProperty::Prop_ManufacturerName_String, manufacturer, sizeof(manufacturer));

		char modelnumber[1024];
		vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, vr::ETrackedDeviceProperty::Prop_ModelNumber_String, modelnumber, sizeof(modelnumber));

		char serialnumber[1024];
		vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, vr::ETrackedDeviceProperty::Prop_SerialNumber_String, serialnumber, sizeof(serialnumber));

		sprintf_s(buf, sizeof(buf), " %s - %s [%s] class(%d)\n", manufacturer, modelnumber, serialnumber, trackedDeviceClass);
		printf_s(buf);

        // If the device is a controller, print out any axis it has
        if (trackedDeviceClass == vr::ETrackedDeviceClass::TrackedDeviceClass_Controller) {
            for (int j = 0; j < vr::k_unControllerStateAxisCount; j++) {
                auto axis = static_cast<vr::ETrackedDeviceProperty>(j + vr::Prop_Axis0Type_Int32);
                vr::TrackedPropertyError error;
                auto enumAxis = static_cast<vr::EVRControllerAxisType> (
                    vr::VRSystem()->GetInt32TrackedDeviceProperty(unDevice, axis, &error)
                    );
                if (!error) {
                    sprintf_s(buf, sizeof(buf), "\taxis: %d - type: %d (%s) \n", j, enumAxis, vr::VRSystem()->GetControllerAxisTypeNameFromEnum(enumAxis));
                    printf_s(buf);
                }

            }
        }
	}
	sprintf_s(buf, sizeof(buf), "---------------------------\n\n");
	printf_s(buf);
}



//-----------------------------------------------------------------------------
// Purpose: Processes a single VR event
//-----------------------------------------------------------------------------

bool LighthouseTracking::ProcessVREvent(const vr::VREvent_t & event, int filterOutIndex = -1) {
    // if user supplied a device filter index only show events for that device
    if (filterOutIndex != -1)
        if (event.trackedDeviceIndex == filterOutIndex)
            return true;

    // print stuff for various events (this is not a complete list). Add/remove upon your own desire...
    switch (event.eventType)
    {
    case vr::VREvent_TrackedDeviceActivated:
    {
        //SetupRenderModelForTrackedDevice(event.trackedDeviceIndex);
        char buf[1024];
        sprintf_s(buf, sizeof(buf), "(OpenVR) Device : %d attached\n", event.trackedDeviceIndex);
        printf_s(buf);
    }
    break;

    case vr::VREvent_TrackedDeviceDeactivated:
    {
        char buf[1024];
        sprintf_s(buf, sizeof(buf), "(OpenVR) Device : %d detached\n", event.trackedDeviceIndex);
        printf_s(buf);
    }
    break;

    case vr::VREvent_TrackedDeviceUpdated:
    {
        char buf[1024];
        sprintf_s(buf, sizeof(buf), "(OpenVR) Device : %d updated\n", event.trackedDeviceIndex);
        printf_s(buf);
    }
    break;

    case (vr::VREvent_DashboardActivated):
    {
        char buf[1024];
        sprintf_s(buf, sizeof(buf), "(OpenVR) Dashboard activated\n");
        printf_s(buf);
    }
    break;

    case (vr::VREvent_DashboardDeactivated):
    {
        char buf[1024];
        sprintf_s(buf, sizeof(buf), "(OpenVR) Dashboard deactivated\n");
        printf_s(buf);

    }
    break;

    case (vr::VREvent_ChaperoneDataHasChanged):
    {
        char buf[1024];
        sprintf_s(buf, sizeof(buf), "(OpenVR) Chaperone data has changed\n");
        printf_s(buf);

    }
    break;

    case (vr::VREvent_ChaperoneSettingsHaveChanged):
    {
        char buf[1024];
        sprintf_s(buf, sizeof(buf), "(OpenVR) Chaperone settings have changed\n");
        printf_s(buf);
    }
    break;

    case (vr::VREvent_ChaperoneUniverseHasChanged):
    {
        char buf[1024];
        sprintf_s(buf, sizeof(buf), "(OpenVR) Chaperone universe has changed\n");
        printf_s(buf);

    }
    break;

    case (vr::VREvent_ApplicationTransitionStarted):
    {
        char buf[1024];
        sprintf_s(buf, sizeof(buf), "(OpenVR) Application Transition: Transition has started\n");
        printf_s(buf);

    }
    break;

    case (vr::VREvent_ApplicationTransitionNewAppStarted):
    {
        char buf[1024];
        sprintf_s(buf, sizeof(buf), "(OpenVR) Application transition: New app has started\n");
        printf_s(buf);

    }
    break;

    case (vr::VREvent_Quit):
    {
        char buf[1024];
        sprintf_s(buf, sizeof(buf), "(OpenVR) Received SteamVR Quit (%d)\n", vr::VREvent_Quit);
        printf_s(buf);

        return false;
    }
    break;

    case (vr::VREvent_ProcessQuit):
    {
        char buf[1024];
        sprintf_s(buf, sizeof(buf), "(OpenVR) SteamVR Quit Process (%d)\n", vr::VREvent_ProcessQuit);
        printf_s(buf);

        return false;
    }
    break;

    case (vr::VREvent_QuitAborted_UserPrompt):
    {
        char buf[1024];
        sprintf_s(buf, sizeof(buf), "(OpenVR) SteamVR Quit Aborted UserPrompt (%d)\n", vr::VREvent_QuitAborted_UserPrompt);
        printf_s(buf);

        return false;
    }
    break;

    case (vr::VREvent_QuitAcknowledged):
    {
        char buf[1024];
        sprintf_s(buf, sizeof(buf), "(OpenVR) SteamVR Quit Acknowledged (%d)\n", vr::VREvent_QuitAcknowledged);
        printf_s(buf);

        return false;
    }
    break;

    case (vr::VREvent_TrackedDeviceRoleChanged):
    {

        char buf[1024];
        sprintf_s(buf, sizeof(buf), "(OpenVR) TrackedDeviceRoleChanged: %d\n", event.trackedDeviceIndex);
        printf_s(buf);
    }
    break;

    case (vr::VREvent_TrackedDeviceUserInteractionStarted):
    {
        char buf[1024];
        sprintf_s(buf, sizeof(buf), "(OpenVR) TrackedDeviceUserInteractionStarted: %d\n", event.trackedDeviceIndex);
        printf_s(buf);
    }
    break;

    case (vr::VREvent_Input_HapticVibration): { printf_s("(OpenVR) VREvent_Input_HapticVibration\n"); } break;
    case (vr::VREvent_Input_BindingLoadFailed): { printf_s("(OpenVR) VREvent_Input_BindingLoadFailed\n"); } break;
    case (vr::VREvent_Input_BindingLoadSuccessful): { printf_s("(OpenVR) VREvent_Input_BindingLoadSuccessful\n"); } break;
    case (vr::VREvent_Input_ActionManifestReloaded): { printf_s("(OpenVR) VREvent_Input_ActionManifestReloaded\n"); } break;
    case (vr::VREvent_Input_ActionManifestLoadFailed): { printf_s("(OpenVR) VREvent_Input_ActionManifestLoadFailed\n"); } break;
    case (vr::VREvent_Input_ProgressUpdate): { printf_s("(OpenVR) VREvent_Input_ProgressUpdate\n"); } break;
    case (vr::VREvent_Input_TrackerActivated): { printf_s("(OpenVR) VREvent_Input_TrackerActivated\n"); } break;
    case (vr::VREvent_Input_BindingsUpdated): { printf_s("(OpenVR) VREvent_Input_BindingsUpdated\n"); } break;
    case (vr::VREvent_ActionBindingReloaded): { printf_s("(OpenVR) VREvent_ActionBindingReloaded\n"); } break;

        // various events not handled/moved yet into the previous switch chunk.
    default: {
        char buf[1024];
        switch (event.eventType) {
        case vr::EVREventType::VREvent_ButtonTouch:
            sprintf_s(buf, sizeof(buf), "(OpenVR) Event: Touch Device: %d\n", event.trackedDeviceIndex);
            printf_s(buf);
            break;
        case vr::EVREventType::VREvent_ButtonUntouch:
            sprintf_s(buf, sizeof(buf), "(OpenVR) Event: Untouch Device: %d\n", event.trackedDeviceIndex);
            printf_s(buf);
            break;
        case vr::EVREventType::VREvent_ButtonPress:
            sprintf_s(buf, sizeof(buf), "(OpenVR) Event: Press Device: %d\n", event.trackedDeviceIndex);
            printf_s(buf);
            break;
        case vr::EVREventType::VREvent_ButtonUnpress:
            sprintf_s(buf, sizeof(buf), "(OpenVR) Event: Release Device: %d\n", event.trackedDeviceIndex);
            printf_s(buf);
            break;
        case vr::EVREventType::VREvent_EnterStandbyMode:
            sprintf_s(buf, sizeof(buf), "(OpenVR) Event: Enter StandbyMode: %d\n", event.trackedDeviceIndex);
            printf_s(buf);
            break;
        case vr::EVREventType::VREvent_LeaveStandbyMode:
            sprintf_s(buf, sizeof(buf), "(OpenVR) Event: Leave StandbyMode: %d\n", event.trackedDeviceIndex);
            printf_s(buf);
            break;
        case vr::EVREventType::VREvent_PropertyChanged:
            sprintf_s(buf, sizeof(buf), "(OpenVR) Event: Property Changed Device: %d ETrackedDeviceProperty(%d)\n", event.trackedDeviceIndex, event.data.property.prop);
            printf_s(buf);
            break;
        case vr::EVREventType::VREvent_SceneApplicationChanged:
            sprintf_s(buf, sizeof(buf), "(OpenVR) Event: Scene Application Changed\n");
            printf_s(buf);
            break;
        case vr::EVREventType::VREvent_SceneFocusChanged:
            sprintf_s(buf, sizeof(buf), "(OpenVR) Event: Scene Focus Changed\n");
            printf_s(buf);
            break;
        case vr::EVREventType::VREvent_TrackedDeviceUserInteractionStarted:
            sprintf_s(buf, sizeof(buf), "(OpenVR) Event: Tracked Device User Interaction Started Device: %d\n", event.trackedDeviceIndex);
            printf_s(buf);
            break;
        case vr::EVREventType::VREvent_TrackedDeviceUserInteractionEnded:
            sprintf_s(buf, sizeof(buf), "(OpenVR) Event: Tracked Device User Interaction Ended Device: %d\n", event.trackedDeviceIndex);
            printf_s(buf);
            break;
        case vr::EVREventType::VREvent_ProcessDisconnected:
            sprintf_s(buf, sizeof(buf), "(OpenVR) Event: A process was disconnected\n");
            printf_s(buf);
            break;
        case vr::EVREventType::VREvent_ProcessConnected:
            sprintf_s(buf, sizeof(buf), "(OpenVR) Event: A process was connected\n");
            printf_s(buf);
            break;

        default:
            sprintf_s(buf, sizeof(buf), "(OpenVR) Unmanaged Event: %d Device: %d\n", event.eventType, event.trackedDeviceIndex);
            printf_s(buf);
            break;
        }
    }
             break;
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
