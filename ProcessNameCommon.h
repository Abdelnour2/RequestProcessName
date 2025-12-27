#pragma once

struct ProcessNameInput {
	ULONG ProcessId;
};

struct ProcessNameOutput {
	char processName[16];
};

#define GET_PROCESS_NAME CTL_CODE(0x8000, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)