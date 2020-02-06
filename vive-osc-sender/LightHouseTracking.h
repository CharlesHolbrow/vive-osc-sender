// LIGHTHOUSETRACKING.h
#ifndef _LIGHTHOUSETRACKING_H_
#define _LIGHTHOUSETRACKING_H_

// OpenVR
#include <openvr.h>
#include "ip\UdpSocket.h"
#include "osc\OscOutboundPacketStream.h"
#include "samples\shared\Matrices.h"

class LighthouseTracking {
private:

	// Basic stuff
	vr::IVRSystem *m_pHMD = NULL;
	vr::TrackedDevicePose_t m_rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
    bool m_deviceConnected[vr::k_unMaxTrackedDeviceCount];
	Matrix4 m_rmat4DevicePose[vr::k_unMaxTrackedDeviceCount];

	vr::HmdVector3_t m_vecLeftController = {};
	vr::HmdVector3_t m_vecRightController = {};

	// Position and rotation of pose
	vr::HmdVector3_t LighthouseTracking::GetPosition(vr::HmdMatrix34_t matrix);
	vr::HmdQuaternion_t LighthouseTracking::GetRotation(vr::HmdMatrix34_t matrix);

	void LighthouseTracking::StoreLeftControllerPosition(vr::HmdVector3_t vector);
	void LighthouseTracking::StoreRightControllerPosition(vr::HmdVector3_t vector);


	// If false the program will parse tracking data continously and not wait for openvr events
	bool bWaitForEventsBeforeParsing = false;

	bool printHmdTrackingData = false;
	bool printRightControllerTrackingData = false;
	bool printLeftControllerTrackingData = false;
	bool printBasestationTrackingData = false;

	// UdpTransmitSocket
	UdpTransmitSocket transmitSocket;

public:
	~LighthouseTracking();
	LighthouseTracking(IpEndpointName ip);

	// Main loop that listens for openvr events and calls process and parse routines, if false the service has quit
	bool RunProcedure();

	// Process a VR event and print some general info of what happens
	bool ProcessVREvent(const vr::VREvent_t & event);

	// Parse a tracking frame and print its position / rotation / events.
	// Supply a filterIndex different than -1 to only show data for one specific device.
	void ParseTrackingFrame();

	// calc diff between positions
	vr::HmdVector3_t LighthouseTracking::GetControllerPositionDelta();
	vr::HmdVector3_t LighthouseTracking::GetLeftControllerPosition();
	vr::HmdVector3_t LighthouseTracking::GetRightControllerPosition();


	// prints information of devices
	void PrintDevices();
};

#endif _LIGHTHOUSETRACKING_H_
