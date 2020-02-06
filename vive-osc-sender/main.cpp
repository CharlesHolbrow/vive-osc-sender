//
// HTC Vive Lighthouse Tracking Example
// By Peter Thor 2016, 2017, 2018, 2019
//

#include "stdafx.h"
#include "Windows.h"
#include <string>
#include <conio.h>
#include <iomanip>		// for std::setprecision
#include <iostream>

#include "LighthouseTracking.h"

// windows keyboard input
#include <conio.h>
#include <vector>

using std::vector;
using std::string;

int _tmain(int argc, _TCHAR* argv[])
{
	int shouldListDevicesAndQuit = 0;
	int port = 9999;
	char ip_address[128];
	sprintf_s(ip_address, sizeof(ip_address), "127.0.0.1");

	// very basic command line parser, from:
	// http://stackoverflow.com/questions/17144037/change-a-command-line-argument-argv
	vector<string> validArgs;
	validArgs.reserve(argc); //Avoids reallocation; it's one or two (if --item is given) too much, but safe and not pedentatic while handling rare cases where argc can be zero

	// parse all user input
	for (int i = 1; i<argc; ++i) {
		const std::string myArg(argv[i]);

		if (myArg == std::string("--listdevices")) shouldListDevicesAndQuit = atoi(argv[i + 1]);
		if (myArg == std::string("--ip")) sprintf_s(ip_address, sizeof(ip_address), argv[i + 1]);
		if (myArg == std::string("--port")) port = atoi(argv[i + 1]);

		validArgs.push_back(myArg);
	}

	// Create a new LighthouseTracking instance and parse as needed
	LighthouseTracking *lighthouseTracking = new LighthouseTracking(IpEndpointName(ip_address, port));
	if (lighthouseTracking) {

		lighthouseTracking->PrintDevices();

		if (!shouldListDevicesAndQuit) {
			printf_s("Press 'q' to quit. Starting capture of tracking data...\n");

			// This is our main loop run
			while (lighthouseTracking->RunProcedure()) {

				// Windows quit routine - adapt as you need
				if (_kbhit()) {
					char ch = _getch();
					if ('q' == ch) {
						printf_s("User pressed 'q' - exiting...");
						break;
					} else if ('l' == ch) {
						lighthouseTracking->PrintDevices();
					}
				}

				// a delay to not overheat your computer... :)
				Sleep(2);
			}
		}

		delete lighthouseTracking;
	}
	return EXIT_SUCCESS;
}

