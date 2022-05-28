#include"driver.h"
#include"Wdf.h"
#include"print_module.h"

//extern void  _set_ret();

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD AMTHIEvtDeviceAdd;
typedef UINT64(*FuncPtr)(UINT64, UINT64);
typedef UINT64(*FuncPtr2)(UINT64, UINT64, UINT64);
typedef UINT64(*FuncPtr3)(UINT64, UINT64, UINT64, UINT64, UINT64);
typedef UINT64(*FuncPtr4)(UINT64, UINT32); 
FuncPtr OriginFunc_GetDevice;
FuncPtr OriginFunc_ContextWorker;
FuncPtr2 OriginFunc_WdfRequestSetInfo;
FuncPtr OriginFunc_WdfRequestGetInfo;
FuncPtr3 OriginFunc_CientsEvt;
FuncPtr4 OriginFunc_ExFreePoolWithTag;
FuncPtr2 OriginFunc_WdfCollectionGetItem;
FuncPtr2 OriginFunc_HalReadData;
UINT64 BaseAddr;
UINT64 gDeviceContext = NULL;
UINT64 gDevice = NULL;
UINT64 gDriver = NULL;
UINT32 gReadBufferOffset=NULL;

FuncPtr OriginFunc_memcpy_s;

/*
char* HexDump(unsigned char* read_buffer, int len) {
	int i = 0;
	for (i = 0; i < len; i++) {
		DbgPrintEx(DPFLTR_DEFAULT_ID, 0, "%02X ", *(read_buffer + i));
		if (i % 16 == 15) {
			DbgPrintEx(DPFLTR_DEFAULT_ID, 0, "\n");
		}
	}
	DbgPrintEx(DPFLTR_DEFAULT_ID, 0, "\n");
}*/

void HexDump(const void* data, size_t size) {
	char ascii[17];
	size_t i, j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i) {
		DbgPrintEx(DPFLTR_DEFAULT_ID, 0, "%02X ", ((unsigned char*)data)[i]);
		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
			ascii[i % 16] = ((unsigned char*)data)[i];
		}
		else {
			ascii[i % 16] = '.';
		}
		if ((i + 1) % 8 == 0 || i + 1 == size) {
			DbgPrintEx(DPFLTR_DEFAULT_ID, 0, " ");
			if ((i + 1) % 16 == 0) {
				DbgPrintEx(DPFLTR_DEFAULT_ID, 0, "|  %s \n", ascii);
			}
			else if (i + 1 == size) {
				ascii[(i + 1) % 16] = '\0';
				if ((i + 1) % 16 <= 8) {
					DbgPrintEx(DPFLTR_DEFAULT_ID, 0, " ");
				}
				for (j = (i + 1) % 16; j < 16; ++j) {
					DbgPrintEx(DPFLTR_DEFAULT_ID, 0, "   ");
				}
				DbgPrintEx(DPFLTR_DEFAULT_ID, 0, "|  %s \n", ascii);
			}
		}
	}
}

UINT64 GetKernelAddress(PCHAR name)
{
	NTSTATUS status = STATUS_SUCCESS;
	ULONG neededSize = 0;

	ZwQuerySystemInformation(
		SystemModuleInformation,
		&neededSize,
		0,
		&neededSize
	);

	PSYSTEM_MODULE_INFORMATION pModuleList;

	pModuleList = (PSYSTEM_MODULE_INFORMATION)ExAllocatePoolWithTag(NonPagedPool, neededSize, POOLTAG01);
	if (pModuleList == NULL)
	{
		return FALSE;
	}

	status = ZwQuerySystemInformation(SystemModuleInformation,
		pModuleList,
		neededSize,
		0
	);
	ULONG i = 0;

	UINT64 address = 0;

	for (i = 0; i < pModuleList->ulModuleCount; i++)
	{
		SYSTEM_MODULE mod = pModuleList->Modules[i];

		address = pModuleList->Modules[i].Base;

		if (strstr(&mod.ImageName, name) != NULL)
			break;
	}

	ExFreePoolWithTag(pModuleList, POOLTAG01);

	return address;
}


NTSTATUS DriverEntry(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PUNICODE_STRING RegistryPath
)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_DRIVER_CONFIG config;

	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "DriverEntry\n"));

	WDF_DRIVER_CONFIG_INIT(&config,
		AMTHIEvtDeviceAdd
	);

	status = WdfDriverCreate(DriverObject,
		RegistryPath,
		WDF_NO_OBJECT_ATTRIBUTES,
		&config,
		WDF_NO_HANDLE
	);
	return status;

}

UINT64 HookContextSpace(UINT64 a, UINT64 IoQueue) {
	WDFDEVICE handle = (*OriginFunc_GetDevice)(a, IoQueue);
	UINT64 ContextSpace = (*OriginFunc_ContextWorker)(a, handle, *(UINT64*)(BaseAddr + 0x33068));
	gDeviceContext = ContextSpace;
	gDriver = a;
	gDevice = handle;
	DbgPrintEx(DPFLTR_DEFAULT_ID, 0, "dev: %llx \n", a);
	DbgPrintEx(DPFLTR_DEFAULT_ID, 0, "handle: %llx \n", gDeviceContext);

	return handle;
}

UINT64 HookRequestSetInfo(UINT64 a1, UINT64 a2, UINT64 a3) {
	UINT64 x = 0xdeadbeefdeadbeef;
	UINT64* y = &x;
	UINT32 offset = 0x20;
	UINT32 offset2 = 0x68;
	UINT32 offset3 = 0x80;
	char* dst;
	UINT32 len;

	if (*(UINT64*)((UINT64)y + offset) == BaseAddr + 0x180fc ) {
		DbgPrintEx(DPFLTR_DEFAULT_ID, 0, "HalReadHooked1######################\n");
		len = *(UINT32*)((UINT64)y + offset2);
		dst = *(UINT64*)((UINT64)y + offset3);
		if (len == 0) {
			DbgPrintEx(DPFLTR_DEFAULT_ID, 0, "NODATA\n");
		}
		HexDump((unsigned char*)((UINT64)dst + gReadBufferOffset), len);

	}

	return OriginFunc_WdfRequestSetInfo(a1, a2, a3);
}

UINT64 HookRequestGetInfo(UINT64 a1, UINT64 a2) {
	UINT64 x = 0xdeadbeefdeadbeef;
	UINT64* y = &x;
	UINT32 offset = 0x30;

	if (*(UINT64*)((UINT64)y + offset) == BaseAddr + 0x180ac) {
		gReadBufferOffset = OriginFunc_WdfRequestGetInfo(a1, a2);
	}
	return OriginFunc_WdfRequestGetInfo(a1, a2);
}

UINT64 HookClientsEvt(UINT64 a1, UINT64 a2, UINT64 a3, UINT64 a4, UINT64 a5) {
	DbgPrintEx(DPFLTR_DEFAULT_ID, 0, "HalReadHooked2######################\n");
	HexDump(a4, a5);
	return OriginFunc_CientsEvt(a1, a2, a3, a4, a5);
}

UINT64 HookGetItem(UINT64 a1, UINT64 a2, UINT32 a3) {
	UINT64 x = 0xdeadbeefdeadbeef;
	UINT64* y = &x;
	UINT32 offset = 0x38;
	UINT32 offset2 = 0x98;
	UINT32 offset3 = 0x90;
	unsigned char* buf;
	UINT32 *header;
	UINT32 len;

	if (*(UINT64*)((UINT64)y + offset) == BaseAddr + 0xa69d && //from ClientsFindFileObjectByHostClientId
		*(UINT64*)((UINT64)y + offset2) == BaseAddr + 0x17cd8 && //from HalInterruptDpcHandleReadRequest
		*(UINT16*)((UINT64)y + offset3) == (UINT16)0x022a) { //from AMTHI to Host
		DbgPrintEx(DPFLTR_DEFAULT_ID, 0, "HalReadHooked3######################\n");
		header = (UINT32*)((UINT64)y + offset3);
		len = ((*header) >> 16) & 0x1ff; //get length field
		if (len == 0) {
			DbgPrintEx(DPFLTR_DEFAULT_ID, 0, "empty response\n");
			return OriginFunc_WdfCollectionGetItem(a1, a2, a3);
		}

		buf = ExAllocatePoolWithTag(NonPagedPool, len+1, "HOOK");
		if (buf <= 0) {
			DbgPrintEx(DPFLTR_DEFAULT_ID, 0, "allocation error\n");
			return OriginFunc_WdfCollectionGetItem(a1, a2, a3);
		}
		OriginFunc_HalReadData(gDeviceContext, header, buf, len);
		HexDump(buf, len);
		ExFreePoolWithTag(buf, "HOOK");	
		*(UINT64*)((UINT64)y + offset2) = BaseAddr + 0x17d1d;
		//_set_ret();
	}
	return OriginFunc_WdfCollectionGetItem(a1, a2, a3);
}


NTSTATUS
AMTHIEvtDeviceAdd(
	_In_ WDFDRIVER Driver,
	_Inout_ PWDFDEVICE_INIT DeviceInit
)
{
	NTSTATUS Status;
	WDFMEMORY memory;

	UINT64 address = GetKernelAddress("TeeDriverW8x64.sys");
	BaseAddr = address;
	unsigned char* pOrgData_Write = (unsigned char*)address + offset_Write;
	unsigned char* pOrgData_Read = (unsigned char*)address + offset_Read;
	unsigned char* pOrgData_ClientCreateAndConnectToInternalClient = (unsigned char*)address + offset_ClientCreateAndConnectToInternalClient;
	unsigned char* pOrgData_SendFlowControlRequestToDriver = (unsigned char*)address + offset_SendFlowControlRequestToDriver;
	unsigned char* pOrgData_ClientsDisconnect = (unsigned char*)address + offset_ClientsDisconnect;
	unsigned char* pOrgData_SendInternalIoControlRequestSynchronously = (unsigned char*)address + offset_SendInternalIoControlRequestSynchronously;
	unsigned char* pOrgData_SendInternalIoControlRequest = (unsigned char*)address + offset_SendInternalIoControlRequest;
	unsigned char* pOrgData_HalMeiReadRegister = (unsigned char*)address + offset_HalMeiReadRegister;

	struct Hal h;
	HECI_MESSAGE_HEADER header_send;
	HECI_MESSAGE_HEADER header_recv;
	HBM_FLOW_CONTROL flow_control;
	HBM_HOST_CLIENT_PROPERTIES_REQUEST properties_request;
	HBM_CLIENT_CONNECT_REQUEST connect_request;

	h.fp_Write = pOrgData_Write;
	h.fp_Read = pOrgData_Read;
	h.fp_ClientCreateAndConnectToInternalClient = pOrgData_ClientCreateAndConnectToInternalClient;
	h.fp_SendFlowControlRequestToDriver = pOrgData_SendFlowControlRequestToDriver;
	h.fp_ClientsDisconnect = pOrgData_ClientsDisconnect;
	h.fp_SendInternalIoControlRequestSynchronously = pOrgData_SendInternalIoControlRequestSynchronously;
	h.fp_SendInternalIoControlRequest = pOrgData_SendInternalIoControlRequest;
	h.fp_HalMeiReadRegister = pOrgData_HalMeiReadRegister;

	header_send.MEAddress = 0x0;
	header_send.HostAddress = 0x0;
	header_send.Length = 0x08;
	header_send.MessageComplete = TRUE;
	header_send.Reserved = 0x0;

	header_recv.MEAddress = 0x0;
	header_recv.HostAddress = 0x0;
	header_recv.Length = 0x1c;
	header_recv.MessageComplete = TRUE;
	header_recv.Reserved = 0x0;

	flow_control.Command.Command = 8;
	flow_control.Command.IsResponse = FALSE;
	flow_control.MEAddress = 0x2a;
	flow_control.HostAddress = 0x9;
	flow_control.Reserved[0] = 0x0;
	flow_control.Reserved[1] = 0x0;
	flow_control.Reserved[2] = 0x0;
	flow_control.Reserved[3] = 0x0;
	flow_control.Reserved[4] = 0x0;

	properties_request.Command.Command = 5;
	properties_request.Command.IsResponse = FALSE;
	properties_request.Address = 0x2a;
	properties_request.Reserved[0] = 0;
	properties_request.Reserved[1] = 0;

	connect_request.Command.Command = 0x06;
	connect_request.Command.IsResponse = FALSE;
	connect_request.MEAddress = 0x2a;
	connect_request.HostAddress = 0x09;
	connect_request.Reserved = 0x0;

	HECI_MESSAGE_HEADER* PHeader_send = &header_send;
	HECI_MESSAGE_HEADER* PHeader_recv = &header_recv;

	UINT64* WdfFunc_addr;

	//hooking wdf bind table
	WdfFunc_addr = address + 0x33980; // &imp_WdfObjectGetTypedContextWorker
	OriginFunc_ContextWorker = *WdfFunc_addr;
	WdfFunc_addr = address + 0x33818; // &imp_WdfioQueueGetDevice 
	OriginFunc_GetDevice = *WdfFunc_addr;
	*WdfFunc_addr = &HookContextSpace;

	//wait until we got handle
	while (gDeviceContext == NULL) {

	}
	//restore wdf bind state
	WdfFunc_addr = address + 0x33818;
	*WdfFunc_addr = OriginFunc_GetDevice;
	
	//HookWdfRequestSetInfo
	UINT64* WdfRequestSetInfo = address + 0x33bc8;
	OriginFunc_WdfRequestSetInfo = *WdfRequestSetInfo;
	*WdfRequestSetInfo = &HookRequestSetInfo;

	//HookWdfRequestGetInfo
	UINT64* WdfRequestGetInfo = address + 0x33bd0;
	OriginFunc_WdfRequestGetInfo = *WdfRequestGetInfo;
	*WdfRequestGetInfo = &HookRequestGetInfo;

	//HookClientsEvtReceivedClientMessage
	OriginFunc_CientsEvt = *(UINT64*)((UINT64)gDeviceContext + 720);
	*(UINT64*)((UINT64)gDeviceContext + 720) = &HookClientsEvt;

	//HookWdfCollectionGetItem
	UINT64* WdfGetItem = address + 0x333c0;
	OriginFunc_WdfCollectionGetItem = *WdfGetItem;
	*WdfGetItem = &HookGetItem;

	OriginFunc_HalReadData = address + 0x1aa10;

	INT64* st = gDeviceContext;
	/////////
	struct AMT_MSG amt;
	amt.groupID = 0x12;
	amt.command = 0x6;
	amt.isResponse = 0;
	amt.reserved = 0;
	amt.result = 0;

	struct CUSTOM_MSG cus;
	cus.uint1 = 0x0101;
	cus.uint2 = 0x04000050; ///////command find
	cus.uint3 = 0x28;
	cus.uint4 = 0x0;
	cus.uint5 = 0;
	cus.uint6 = 0;
	cus.uint7 = 0;
	cus.uint8 = 0;
	cus.uint9 = 0;
	cus.uinta = 0;
	cus.uintb = 0;
	cus.uintc = 0;
	cus.uintd = 0;

	char* GUID = "\x28\x00\xf8\x12\xb7\xb4\x2d\x4b\xac\xa8\x46\xe0\xff\x65\x81\x4c";
	//char* GUID = "\xdb\xa4\x33\x67\x76\x04\x7b\x4e\xb3\xaf\xbc\xfc\x29\xbe\xe7\xa7";
	UINT64 X = NULL;
	UINT64 Y = NULL;

	UINT16 clients_addr = 0x2a02;

	LARGE_INTEGER time;

	//flowcontrol

	for (int i = 0; i < 0x10; i++) {
		time.QuadPart = -10000000;
		KeDelayExecutionThread(KernelMode, TRUE, &time);
		//disconnect
		DbgPrintEx(DPFLTR_DEFAULT_ID, 0, "Sending:######################\n");
		DbgPrintEx(DPFLTR_DEFAULT_ID, 0, "07 2a 02 00\n");
		h.fp_SendInternalIoControlRequestSynchronously(gDeviceContext, 0x8000e144, &clients_addr, (UINT32)3, (UINT64)0, (BYTE)0, (UINT64)0, TRUE);
		
		//connect
		DbgPrintEx(DPFLTR_DEFAULT_ID, 0, "Sending:######################\n");
		DbgPrintEx(DPFLTR_DEFAULT_ID, 0, "06 2a 02 00\n");
		h.fp_SendInternalIoControlRequest(gDeviceContext, 0x8000e148, &clients_addr, (UINT32)3, &Y, (UINT32)5, (UINT64)0, (UINT64)0);

		//flowcontrol
		time.QuadPart = -1000000;
		KeDelayExecutionThread(KernelMode, TRUE, &time);
		PHeader_send->HostAddress = 0x02;
		PHeader_send->MEAddress = 0x2a;
		PHeader_send->Length = 0x34;
		UINT32 Z = 6;
		DbgPrintEx(DPFLTR_DEFAULT_ID, 0, "Sending:######################\n");
		HexDump((unsigned char*)&cus, 0x34);
		h.fp_Write(st, PHeader_send, &cus, 0x34);

		//h.fp_SendFlowControlRequestToDriver(st, 0x2, 0x2a);
		DbgPrintEx(DPFLTR_DEFAULT_ID, 0, "Sending:######################\n");
		DbgPrintEx(DPFLTR_DEFAULT_ID, 0, "08 2a 02 00 00 00 00 00\n");
		h.fp_SendInternalIoControlRequest(gDeviceContext, 0x8000e140, &clients_addr, (UINT32)3, (UINT64)0, (UINT32)0, (UINT64)0, (UINT64)0);
		cus.uint2 += 1;
	}

	//restore hooked WdfRequestSetInfo
	*WdfRequestSetInfo = OriginFunc_WdfRequestSetInfo;

	//restore hooked WdfRequestSetInfo
	*WdfRequestGetInfo = OriginFunc_WdfRequestGetInfo;

	//restore hooked ClientsEvtReceivedClientMessage
	*(UINT64*)((UINT64)gDeviceContext + 720) = OriginFunc_CientsEvt;

	//restore hooked WdfRequestSetInfo
	*WdfGetItem = OriginFunc_WdfCollectionGetItem;

	return 0;

	UNREFERENCED_PARAMETER(Driver);

	NTSTATUS status;
	WDFDEVICE hDevice;

	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "AMTHIEvtDeviceAdd\n"));

	status = WdfDeviceCreate(&DeviceInit,
		WDF_NO_OBJECT_ATTRIBUTES,
		&hDevice
	);
	return status;
}
