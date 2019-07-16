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

	// Prepare manifest file
	const char *manifestPath = "C:/projects-charles/vive-osc-sender/vive_debugger_actions.json";
	vr::EVRInputError inputError = vr::VRInput()->SetActionManifestPath(manifestPath);
	if (inputError != vr::VRInputError_None) {
		sprintf_s(buf, sizeof(buf), "Error: Unable to set manifest path: %d\n", inputError);
		printf_s(buf);
	}
	else {
		sprintf_s(buf, sizeof(buf), "Successfully set manifest path: %s\n", manifestPath);
		printf_s(buf);
	}

	// Handles for the new IVRInput
	inputError = vr::VRInput()->GetActionSetHandle(actionSetDemoPath, &m_actionSetDemo);
	if (inputError != vr::VRInputError_None) {
		sprintf_s(buf, sizeof(buf), "Error: Unable to get action set handle: %d\n", inputError);
		printf_s(buf);
	}
	else {
		sprintf_s(buf, sizeof(buf), "Successfully got %s action set handle: %d\n", actionSetDemoPath, m_actionSetDemo);
		printf_s(buf);
	}

	// handle for controller pose
	inputError = vr::VRInput()->GetActionHandle(actionDemoControllerPath, &m_actionDemoController);
	if (inputError != vr::VRInputError_None) {
		sprintf_s(buf, sizeof(buf), "Error: Unable to get action handle: %d\n", inputError);
		printf_s(buf);
	}
	else {
		sprintf_s(buf, sizeof(buf), "Successfully got %s handle: %d\n", actionDemoControllerPath, m_actionDemoController);
		printf_s(buf);
	}

	// handle for tracker pose
	inputError = vr::VRInput()->GetActionHandle(actionDemoTrackerPath, &m_actionDemoTracker);
	if (inputError != vr::VRInputError_None) {
		sprintf_s(buf, sizeof(buf), "Error: Unable to get action handle: %d\n", inputError);
		printf_s(buf);
	}
	else {
		sprintf_s(buf, sizeof(buf), "Successfully got %s handle: %d\n", actionDemoTrackerPath, m_actionDemoTracker);
		printf_s(buf);
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
		ParseTrackingFrame(filterIndex);
	}
	return true;
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

	char buffer[2048];
	char buf[1024];
	char oscAddress[1024];
	vr::EVRInputError inputError;

	sprintf_s(buf, sizeof(buf), "");
	printf_s(buf);

	// Process SteamVR action state
	// UpdateActionState is called each frame to update the state of the actions themselves. The application
	// controls which action sets are active with the provided array of VRActiveActionSet_t structs.
	vr::VRActiveActionSet_t actionSet = { 0 };
	actionSet.ulActionSet = m_actionSetDemo;
	inputError = vr::VRInput()->UpdateActionState(&actionSet, sizeof(actionSet), 1);
	if (inputError != vr::VRInputError_None) {
		sprintf_s(buf, sizeof(buf), "UpdateActionState(): Error: %d\n", inputError);
		printf_s(buf);
	}

	// get pose data
	vr::InputPoseActionData_t poseController;
	inputError = vr::VRInput()->GetPoseActionDataRelativeToNow(m_actionDemoController, vr::TrackingUniverseStanding, 0, &poseController, sizeof(poseController), vr::k_ulInvalidInputValueHandle);

	if (inputError == vr::VRInputError_None) {

		if (poseController.bActive) {
			vr::VRInputValueHandle_t activeOrigin = poseController.activeOrigin;
			bool bPoseIsValid = poseController.pose.bPoseIsValid;
			bool bDeviceIsConnected = poseController.pose.bDeviceIsConnected;

			// Code below is old ---> 
			vr::HmdVector3_t position;
			vr::HmdQuaternion_t quaternion;

			// get the position and rotation
			position = GetPosition(poseController.pose.mDeviceToAbsoluteTracking);
			quaternion = GetRotation(poseController.pose.mDeviceToAbsoluteTracking);

			// OSC
			// If we want to send one bundle per frame, we would have to
			// initialize the variable outside of the device loop. For now
			// I'm just doing it here instead. Starting a bundle looks like
			// this: p << osc::BeginBundleImmediate;
			osc::OutboundPacketStream pStream(buffer, 2048);
			sprintf_s(oscAddress, sizeof(oscAddress), "/controller");
			pStream << osc::BeginMessage(oscAddress);

			pStream << position.v[0] << position.v[1] << position.v[2] 
				<< static_cast<float>(quaternion.w) << static_cast<float>(quaternion.x)
				<< static_cast<float>(quaternion.y) << static_cast<float>(quaternion.z)
				<< osc::EndMessage;
			transmitSocket.Send(pStream.Data(), pStream.Size());

			sprintf_s(buf, sizeof(buf), "\rC: (% .2f,  % .2f, % .2f) q(% .2f, % .2f, % .2f, % .2f) --- ",  position.v[0], position.v[1], position.v[2], quaternion.w, quaternion.x, quaternion.y, quaternion.z);
			printf_s(buf);
		}
		else {
			sprintf_s(buf, sizeof(buf), "%s | action not avail to be bound\n", actionDemoControllerPath);
			printf_s(buf);
		}
	}
	else {
		sprintf_s(buf, sizeof(buf), "%s | GetPoseActionData() Call Not Ok. Error: %d\n", actionDemoControllerPath, inputError);
		printf_s(buf);
	}


	vr::InputPoseActionData_t poseTracker;
	inputError = vr::VRInput()->GetPoseActionDataRelativeToNow(m_actionDemoTracker, vr::TrackingUniverseStanding, 0, &poseTracker, sizeof(poseTracker), vr::k_ulInvalidInputValueHandle);
	if (inputError == vr::VRInputError_None) {

		if (poseTracker.bActive) {
			vr::VRInputValueHandle_t activeOrigin = poseTracker.activeOrigin;
			bool bPoseIsValid = poseTracker.pose.bPoseIsValid;
			bool bDeviceIsConnected = poseTracker.pose.bDeviceIsConnected;

			// Code below is old ---> 
			vr::HmdVector3_t position;
			vr::HmdQuaternion_t quaternion;

			// get the position and rotation
			position = GetPosition(poseTracker.pose.mDeviceToAbsoluteTracking);
			quaternion = GetRotation(poseTracker.pose.mDeviceToAbsoluteTracking);

			// OSC
			// If we want to send one bundle per frame, we would have to
			// initialize the variable outside of the device loop. For now
			// I'm just doing it here instead. Starting a bundle looks like
			// this: p << osc::BeginBundleImmediate;
			char buffer[2048];
			osc::OutboundPacketStream pStream(buffer, 2048);

			sprintf_s(oscAddress, sizeof(oscAddress), "/tracker");
			pStream << osc::BeginMessage(oscAddress);

			pStream << position.v[0] << position.v[1] << position.v[2] 
					<< static_cast<float>(quaternion.w) << static_cast<float>(quaternion.x)
					<< static_cast<float>(quaternion.y) << static_cast<float>(quaternion.z)
					<< osc::EndMessage;
			transmitSocket.Send(pStream.Data(), pStream.Size());

			sprintf_s(buf, sizeof(buf), "T: (% .2f,  % .2f, % .2f) q(% .2f, % .2f, % .2f, % .2f)", position.v[0], position.v[1], position.v[2], quaternion.w, quaternion.x, quaternion.y, quaternion.z);
			printf_s(buf);

		}
	
		
		else {
			sprintf_s(buf, sizeof(buf), "%s | action not avail to be bound\n", actionDemoTrackerPath);
			printf_s(buf);
			}
		}
	else {
		sprintf_s(buf, sizeof(buf), "%s | GetPoseActionData() Call Not Ok. Error: %d\n", actionDemoTrackerPath, inputError);
		printf_s(buf);
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
