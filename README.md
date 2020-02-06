#  What is this?

A modified version of [Omnifinity's OpenVR Tracking Example](https://github.com/Omnifinity/OpenVR-Tracking-Example), for Windows updated to send osc.

Uses OpenVR 1.5.17.

Compiled using Visual Studio 2017.

# Settings

If you supply the parameter "--port <number>" you can choose which port to send the OSC data to.

If you supply the parameter "--ip <number>" you can choose which ip address to send the OSC data to.


##  How do I compile it?
1. Make sure that you point your includes and library bin folder to where you have openvr installed on your machine.
2. Make sure you've got the openvr_api.dll in the same folder as the built example

To start using, clone the repo and recurse the submodules:

$ git clone --recurse-submodules <URL>

##  How do I use it?
1. Start up Steam VR
2. In order to get data from Vive Tracker, go to SteamVR->Devices->Manage Vive Trackers, and set role to camera
3. Compile and start the example - it launches as a background application


##  Troubleshooting:

*Unable to init VR runtime: Not starting vrserver for background app (121)*

Solution: Start Steam VR

