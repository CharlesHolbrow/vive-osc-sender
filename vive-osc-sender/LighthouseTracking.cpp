//
// HTC Vive Lighthouse Tracking Example
// By Peter Thor 2016, 2017, 2018, 2019
//
// Shows how to extract basic tracking data
//

#include "stdafx.h"
#include "LighthouseTracking.h"

// Destructor
LighthouseTracking::~LighthouseTracking() {
	if (m_pHMD != NULL)
	{
		vr::VR_Shutdown();
		m_pHMD = NULL;
	}
}

// Constructor
LighthouseTracking::LighthouseTracking() {
	vr::EVRInitError eError = vr::VRInitError_None;
	m_pHMD = vr::VR_Init(&eError, vr::VRApplication_Background);

	if (eError != vr::VRInitError_None)
	{
		m_pHMD = NULL;
		char buf[1024];
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
bool LighthouseTracking::RunProcedure(bool bWaitForEvents, int filterIndex = -1) {

	// Either A) wait for events, such as hand controller button press, before parsing...
	if (bWaitForEvents) {
		// Process VREvent
		vr::VREvent_t event;
		while (m_pHMD->PollNextEvent(&event, sizeof(event)))
		{
			// Process event
			if (!ProcessVREvent(event, filterIndex)) {
				char buf[1024];
				sprintf_s(buf, sizeof(buf), "(OpenVR) service quit\n");
				printf_s(buf);
				return false;
			}

			// parse a frame
			ParseTrackingFrame(filterIndex);
		}
	}
	else {
		// ... or B) continous parsint of tracking data irrespective of events
		//std::cout << std::endl << "Parsing next frame...";

		ParseTrackingFrame(filterIndex);
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Processes a single VR event
//-----------------------------------------------------------------------------

bool LighthouseTracking::ProcessVREvent(const vr::VREvent_t & event, int filterOutIndex = -1)
{
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

		case (vr::VREvent_DashboardActivated) :
		{
			char buf[1024];
			sprintf_s(buf, sizeof(buf), "(OpenVR) Dashboard activated\n");
			printf_s(buf);
		}
		break;

		case (vr::VREvent_DashboardDeactivated) :
		{
			char buf[1024];
			sprintf_s(buf, sizeof(buf), "(OpenVR) Dashboard deactivated\n");
			printf_s(buf);

		}
		break;

		case (vr::VREvent_ChaperoneDataHasChanged) :
		{
			char buf[1024];
			sprintf_s(buf, sizeof(buf), "(OpenVR) Chaperone data has changed\n");
			printf_s(buf);

		}
		break;

		case (vr::VREvent_ChaperoneSettingsHaveChanged) :
		{
			char buf[1024];
			sprintf_s(buf, sizeof(buf), "(OpenVR) Chaperone settings have changed\n");
			printf_s(buf);
		}
		break;

		case (vr::VREvent_ChaperoneUniverseHasChanged) :
		{
			char buf[1024];
			sprintf_s(buf, sizeof(buf), "(OpenVR) Chaperone universe has changed\n");
			printf_s(buf);

		}
		break;

		case (vr::VREvent_ApplicationTransitionStarted) :
		{
			char buf[1024];
			sprintf_s(buf, sizeof(buf), "(OpenVR) Application Transition: Transition has started\n");
			printf_s(buf);

		}
		break;

		case (vr::VREvent_ApplicationTransitionNewAppStarted) :
		{
			char buf[1024];
			sprintf_s(buf, sizeof(buf), "(OpenVR) Application transition: New app has started\n");
			printf_s(buf);

		}
		break;

		case (vr::VREvent_Quit) :
		{
			char buf[1024];
			sprintf_s(buf, sizeof(buf), "(OpenVR) Received SteamVR Quit (%d)\n", vr::VREvent_Quit);
			printf_s(buf);

			return false;
		}
		break;

		case (vr::VREvent_ProcessQuit) :
		{
			char buf[1024];
			sprintf_s(buf, sizeof(buf), "(OpenVR) SteamVR Quit Process (%d)\n", vr::VREvent_ProcessQuit);
			printf_s(buf);

			return false;
		}
		break;

		case (vr::VREvent_QuitAborted_UserPrompt) :
		{
			char buf[1024];
			sprintf_s(buf, sizeof(buf), "(OpenVR) SteamVR Quit Aborted UserPrompt (%d)\n", vr::VREvent_QuitAborted_UserPrompt);
			printf_s(buf);

			return false;
		}
		break;

		case (vr::VREvent_QuitAcknowledged) :
		{
			char buf[1024];
			sprintf_s(buf, sizeof(buf), "(OpenVR) SteamVR Quit Acknowledged (%d)\n", vr::VREvent_QuitAcknowledged);
			printf_s(buf);

			return false;
		}
		break;

		case (vr::VREvent_TrackedDeviceRoleChanged) :
		{

			char buf[1024];
			sprintf_s(buf, sizeof(buf), "(OpenVR) TrackedDeviceRoleChanged: %d\n", event.trackedDeviceIndex);
			printf_s(buf);
		}
		break;

		case (vr::VREvent_TrackedDeviceUserInteractionStarted) :
		{
			char buf[1024];
			sprintf_s(buf, sizeof(buf), "(OpenVR) TrackedDeviceUserInteractionStarted: %d\n", event.trackedDeviceIndex);
			printf_s(buf);
		}
		break;

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
				sprintf_s(buf, sizeof(buf), "(OpenVR) Event: Property Changed Device: %d\n", event.trackedDeviceIndex);
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
void LighthouseTracking::ParseTrackingFrame(int filterIndex) {

	// Process SteamVR device states
	for (vr::TrackedDeviceIndex_t unDevice = 0; unDevice < vr::k_unMaxTrackedDeviceCount; unDevice++)
	{
		// if not connected just skip the rest of the routine
		if (!m_pHMD->IsTrackedDeviceConnected(unDevice)) {
			continue;
		}

		if (filterIndex != unDevice && filterIndex != -1)
			continue;

		vr::TrackedDevicePose_t trackedDevicePose;
		vr::TrackedDevicePose_t *devicePose = &trackedDevicePose;

		vr::VRControllerState_t controllerState;
		vr::VRControllerState_t *ontrollerState_ptr = &controllerState;

		vr::HmdVector3_t position;
		vr::HmdQuaternion_t quaternion;

		bool bPoseValid; //= trackedDevicePose.bPoseIsValid;
		vr::HmdVector3_t vVel;
		vr::HmdVector3_t vAngVel;
		vr::ETrackingResult eTrackingResult;

		// Charles: I'm adding these to the sample because it helps me understand what's going on
		vr::ETrackedControllerRole role; // for controllers, what role 
		char cRole = '?'; // for controllers, which hand is it?
		char cClass = '?';

		// Get what type of device it is and work with its data. The results returned
		// by GetTrackedDeviceClass automatically change when openvr believes the
		// device switched hands. It's trying to help.
		vr::ETrackedDeviceClass trackedDeviceClass = vr::VRSystem()->GetTrackedDeviceClass(unDevice);
		// It appears that this method of getting the tracked device class is not
		// automatically re-assigned when the role of the device changes.
		vr::ETrackedDeviceClass iDeviceClass = static_cast<vr::ETrackedDeviceClass>(vr::VRSystem()->GetInt32TrackedDeviceProperty(unDevice, vr::Prop_DeviceClass_Int32));
		// This is pretty annoying, but it looks like this allows us to get the correct device class
		if (iDeviceClass != trackedDeviceClass && iDeviceClass != vr::ETrackedDeviceClass::TrackedDeviceClass_Invalid) {
			trackedDeviceClass = iDeviceClass;
		}

		switch (trackedDeviceClass) {
		case vr::ETrackedDeviceClass::TrackedDeviceClass_HMD:
			break;
		case vr::ETrackedDeviceClass::TrackedDeviceClass_Controller:
		case vr::ETrackedDeviceClass::TrackedDeviceClass_GenericTracker: {
			char buf[1024];

			if (!vr::VRSystem()->GetControllerStateWithPose(vr::TrackingUniverseStanding, unDevice,	&controllerState, sizeof(controllerState), devicePose))
			{
				sprintf_s(buf, sizeof(buf), "Generic Tracker/Controller - failed to get device state\n");
				printf_s(buf);
				fflush(stdout);
			}
			else {
				// get pose relative to the safe bounds defined by the user
				//vr::VRSystem()->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding, 0, &trackedDevicePose, 1);
				// 
				role = vr::VRSystem()->GetControllerRoleForTrackedDeviceIndex(unDevice);
				// get the position and rotation
				position = GetPosition(devicePose->mDeviceToAbsoluteTracking);
				quaternion = GetRotation(devicePose->mDeviceToAbsoluteTracking);

				vVel = devicePose->vVelocity;
				vAngVel = devicePose->vAngularVelocity;
				eTrackingResult = devicePose->eTrackingResult;
				bPoseValid = devicePose->bPoseIsValid;

				// This slightly weird way of getting trigger presses. See:
				// https://github.com/ValveSoftware/openvr/issues/56
				vr::VRControllerAxis_t t = controllerState.rAxis[vr::k_EButton_SteamVR_Trigger - vr::k_EButton_Axis0];

				// OSC
				// If we want to send one bundle per frame, we would have to
				// initialize the variable outside of the device loop. For now
				// I'm just doing it here instead. Starting a bundle looks like
				// this: p << osc::BeginBundleImmediate;
				char buffer[2048];
				osc::OutboundPacketStream pStream(buffer, 2048);

				switch (eTrackingResult) {
				case vr::ETrackingResult::TrackingResult_Running_OK:
					// At this point, we are definitely going to send an osc.

					// Get a char representing device class
					switch (trackedDeviceClass) {
					case vr::ETrackedDeviceClass::TrackedDeviceClass_Controller:
						pStream << osc::BeginMessage("/controller");
						cClass = 'C';
						break;
					case vr::ETrackedDeviceClass::TrackedDeviceClass_GenericTracker:
						cClass = 'T';
						pStream << osc::BeginMessage("/tracker");
						break;
					}
					// Get a char representing the device role
					switch (role) {
					case vr::TrackedControllerRole_Invalid:
						cRole = 'I';
						break;
					case vr::TrackedControllerRole_RightHand:
						cRole = 'R';
						break;
					case vr::TrackedControllerRole_LeftHand:
						cRole = 'L';
						break;
					}

					pStream << position.v[0] << position.v[1] << position.v[2] 
						<< static_cast<float>(quaternion.w) << static_cast<float>(quaternion.x)
						<< static_cast<float>(quaternion.y) << static_cast<float>(quaternion.z)
						<< t.x
						<< osc::EndMessage;
					transmitSocket.Send(pStream.Data(), pStream.Size());

					sprintf_s(buf, sizeof(buf), "\rTracked Device class-role:(%c-%c) xyz:(% .2f,  % .2f, % .2f) q(% .2f, % .2f, % .2f, % .2f) trigger(% .2f,  % .2f)",
						cClass, cRole, position.v[0], position.v[1], position.v[2], quaternion.w, quaternion.x, quaternion.y, quaternion.z, t.x, t.y);
					printf_s(buf);
					fflush(stdout);

					break;
				case vr::ETrackingResult::TrackingResult_Uninitialized:
					sprintf_s(buf, sizeof(buf), "\rInvalid tracking result       ");
					printf_s(buf);
					fflush(stdout);
					break;
				case vr::ETrackingResult::TrackingResult_Calibrating_InProgress:
					sprintf_s(buf, sizeof(buf), "\rCalibrating in progress       ");
					printf_s(buf);
					fflush(stdout);
					break;
				case vr::ETrackingResult::TrackingResult_Calibrating_OutOfRange:
					sprintf_s(buf, sizeof(buf), "\rCalibrating Out of range      ");
					printf_s(buf);
					fflush(stdout);
					break;
				case vr::ETrackingResult::TrackingResult_Running_OutOfRange:
					sprintf_s(buf, sizeof(buf), "\rWARNING: Running Out of Range ");
					printf_s(buf);
					fflush(stdout);
					break;
				default:
					sprintf_s(buf, sizeof(buf), "\rUnknown Tracking Result       ");
					printf_s(buf);
					fflush(stdout);
					break;
				}

				if (!bPoseValid || eTrackingResult != vr::ETrackingResult::TrackingResult_Running_OK) {
					continue;
				}
			}
			break;
		} // tracked device == controller || generic tracker

		// Lighthouse base station
		case vr::TrackedDeviceClass_TrackingReference: {
			break;
		}

		default: {
			char buf[1024];
			sprintf_s(buf, sizeof(buf), "Unsupported class: %d\n", trackedDeviceClass);
			printf_s(buf);
			}
			break;
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
		// if no HMD is connected in slot 0 just skip the rest of the code
		// since a HMD must be present.
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

			char buf[1024];
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

		int32_t iDeviceClass = vr::VRSystem()->GetInt32TrackedDeviceProperty(unDevice, vr::Prop_DeviceClass_Int32);

		char manufacturer[1024];
		vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, vr::ETrackedDeviceProperty::Prop_ManufacturerName_String, manufacturer, sizeof(manufacturer));

		char modelnumber[1024];
		vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, vr::ETrackedDeviceProperty::Prop_ModelNumber_String, modelnumber, sizeof(modelnumber));

		char serialnumber[1024];
		vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, vr::ETrackedDeviceProperty::Prop_SerialNumber_String, serialnumber, sizeof(serialnumber));

		sprintf_s(buf, sizeof(buf), " %s - %s [%s] class(%d)\n", manufacturer, modelnumber, serialnumber, iDeviceClass);
		printf_s(buf);
	}
	sprintf_s(buf, sizeof(buf), "---------------------------\nEnd of device list\n\n");
	printf_s(buf);

}
