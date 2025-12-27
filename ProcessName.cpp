#include <ntifs.h>
#include <ntddk.h>

#include "ProcessNameCommon.h"

void ProcessNameUnload(PDRIVER_OBJECT DriverObject);

NTSTATUS ProcessNameCreateClose(PDEVICE_OBJECT, PIRP Irp);
NTSTATUS ProcessNameDeviceControl(PDEVICE_OBJECT, PIRP Irp);

extern "C" PCHAR PsGetProcessImageFileName(PEPROCESS);

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);

	KdPrint(("Process Name Driver - Driver Entry\n"));

	DriverObject->DriverUnload = ProcessNameUnload;

	UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\ProcessName");
	PDEVICE_OBJECT deviceObject;
	NTSTATUS status = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, 0, FALSE, &deviceObject);

	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed at IoCreateDevice - Error Code: 0x%X\n", status));
		
		return status;
	}

	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\ProcessName");
	status = IoCreateSymbolicLink(&symLink, &devName);

	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed at IoCreateSymbolicLink - Error Code: 0x%X\n", status));

		IoDeleteDevice(deviceObject);
		return status;
	}

	DriverObject->MajorFunction[IRP_MJ_CREATE] = ProcessNameCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = ProcessNameCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ProcessNameDeviceControl;

	return STATUS_SUCCESS;
}

void ProcessNameUnload(PDRIVER_OBJECT DriverObject) {
	KdPrint(("Process Name Driver - Unloaded\n"));

	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\ProcessName");
	IoDeleteSymbolicLink(&symLink);
	
	IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS ProcessNameCreateClose(PDEVICE_OBJECT, PIRP Irp) {
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, 0);

	return STATUS_SUCCESS;
}

NTSTATUS ProcessNameDeviceControl(PDEVICE_OBJECT, PIRP Irp) {
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;

	switch (stack->Parameters.DeviceIoControl.IoControlCode) {
		case GET_PROCESS_NAME:
			if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(ProcessNameInput) ||
				stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ProcessNameOutput)) {
				status = STATUS_BUFFER_TOO_SMALL;

				break;
			}

			auto input = (ProcessNameInput*)Irp->AssociatedIrp.SystemBuffer;
			auto output = (ProcessNameOutput*)Irp->AssociatedIrp.SystemBuffer;

			PEPROCESS process = nullptr;
			status = PsLookupProcessByProcessId(UlongToHandle(input->ProcessId), &process);

			if (NT_SUCCESS(status)) {
				PCHAR imageName = PsGetProcessImageFileName(process);

				RtlZeroMemory(output->processName, 16);
				RtlCopyMemory(output->processName, imageName, 15);

				ObDereferenceObject(process);

				Irp->IoStatus.Information = sizeof(ProcessNameOutput);
			}
			else {
				KdPrint(("Process Lookup Failed\n"));

				Irp->IoStatus.Information = 0;
			}

			break;
	}

	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, 0);

	return status;
}