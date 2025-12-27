#include <stdio.h>
#include <Windows.h>

#include "..\P1 - Request Process Name\ProcessNameCommon.h"

int main(int argc, const char* argv[]) {
	if (argc < 2) {
		printf("Please provide a process ID\n");

		return -1;
	}

	HANDLE hDevice = CreateFile(L"\\\\.\\ProcessName", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);

	if (hDevice == INVALID_HANDLE_VALUE) {
		printf("Failed to open the driver - Error: %u\n", GetLastError());

		return -1;
	}

	ULONG processId = atoi(argv[1]);
	ProcessNameInput input;

	input.ProcessId = processId;

	ProcessNameOutput output;
	DWORD bytes;
	BOOL ok = DeviceIoControl(hDevice, GET_PROCESS_NAME, &input, sizeof(input), &output, sizeof(output), &bytes, nullptr);

	if (!ok) {
		printf("Error %u\n", GetLastError());

		return -1;
	}

	output.processName[sizeof(output.processName) - 1] = '\0';

	printf("Success!\n");
	printf("Process Name: %s\n", output.processName);

	CloseHandle(hDevice);

	return 0;
}