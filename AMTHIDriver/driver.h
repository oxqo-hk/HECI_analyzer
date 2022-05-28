#include<wdm.h>
#include<wdf.h>
#include<ntddk.h>

#define INITGUID

#define offset_Write 0x1ba14
#define offset_Read 0x1aa10
#define offset_ClientCreateAndConnectToInternalClient 0x8560
#define offset_SendFlowControlRequestToDriver 0x27fe8
#define offset_ClientsDisconnect 0x8d10
#define offset_SendInternalIoControlRequestSynchronously 0x1e4b0
#define offset_SendInternalIoControlRequest 0x1df64
#define offset_HalMeiReadRegister 0x1a2dc

#define POOLTAG01 'ahIK'

typedef enum _SYSTEM_INFORMATION_CLASS
{
	SystemInformationClassMin = 0,
	SystemBasicInformation = 0,
	SystemProcessorInformation = 1,
	SystemPerformanceInformation = 2,
	SystemTimeOfDayInformation = 3,
	SystemPathInformation = 4,
	SystemNotImplemented1 = 4,
	SystemProcessInformation = 5,
	SystemProcessesAndThreadsInformation = 5,
	SystemCallCountInfoInformation = 6,
	SystemCallCounts = 6,
	SystemDeviceInformation = 7,
	SystemConfigurationInformation = 7,
	SystemProcessorPerformanceInformation = 8,
	SystemProcessorTimes = 8,
	SystemFlagsInformation = 9,
	SystemGlobalFlag = 9,
	SystemCallTimeInformation = 10,
	SystemNotImplemented2 = 10,
	SystemModuleInformation = 11,
	SystemLocksInformation = 12,
	SystemLockInformation = 12,
	SystemStackTraceInformation = 13,
	SystemNotImplemented3 = 13,
	SystemPagedPoolInformation = 14,
	SystemNotImplemented4 = 14,
	SystemNonPagedPoolInformation = 15,
	SystemNotImplemented5 = 15,
	SystemHandleInformation = 16,
	SystemObjectInformation = 17,
	SystemPageFileInformation = 18,
	SystemPagefileInformation = 18,
	SystemVdmInstemulInformation = 19,
	SystemInstructionEmulationCounts = 19,
	SystemVdmBopInformation = 20,
	SystemInvalidInfoClass1 = 20,
	SystemFileCacheInformation = 21,
	SystemCacheInformation = 21,
	SystemPoolTagInformation = 22,
	SystemInterruptInformation = 23,
	SystemProcessorStatistics = 23,
	SystemDpcBehaviourInformation = 24,
	SystemDpcInformation = 24,
	SystemFullMemoryInformation = 25,
	SystemNotImplemented6 = 25,
	SystemLoadImage = 26,
	SystemUnloadImage = 27,
	SystemTimeAdjustmentInformation = 28,
	SystemTimeAdjustment = 28,
	SystemSummaryMemoryInformation = 29,
	SystemNotImplemented7 = 29,
	SystemNextEventIdInformation = 30,
	SystemNotImplemented8 = 30,
	SystemEventIdsInformation = 31,
	SystemNotImplemented9 = 31,
	SystemCrashDumpInformation = 32,
	SystemExceptionInformation = 33,
	SystemCrashDumpStateInformation = 34,
	SystemKernelDebuggerInformation = 35,
	SystemContextSwitchInformation = 36,
	SystemRegistryQuotaInformation = 37,
	SystemLoadAndCallImage = 38,
	SystemPrioritySeparation = 39,
	SystemPlugPlayBusInformation = 40,
	SystemNotImplemented10 = 40,
	SystemDockInformation = 41,
	SystemNotImplemented11 = 41,
	/* SystemPowerInformation = 42, Conflicts with POWER_INFORMATION_LEVEL 1 */
	SystemInvalidInfoClass2 = 42,
	SystemProcessorSpeedInformation = 43,
	SystemInvalidInfoClass3 = 43,
	SystemCurrentTimeZoneInformation = 44,
	SystemTimeZoneInformation = 44,
	SystemLookasideInformation = 45,
	SystemSetTimeSlipEvent = 46,
	SystemCreateSession = 47,
	SystemDeleteSession = 48,
	SystemInvalidInfoClass4 = 49,
	SystemRangeStartInformation = 50,
	SystemVerifierInformation = 51,
	SystemAddVerifier = 52,
	SystemSessionProcessesInformation = 53,
	SystemInformationClassMax
} SYSTEM_INFORMATION_CLASS;

typedef struct _SYSTEM_BASIC_INFORMATION
{
	ULONG Reserved;
	ULONG TimerResolution;
	ULONG PageSize;
	ULONG NumberOfPhysicalPages;
	ULONG LowestPhysicalPageNumber;
	ULONG HighestPhysicalPageNumber;
	ULONG AllocationGranularity;
	ULONG_PTR MinimumUserModeAddress;
	ULONG_PTR MaximumUserModeAddress;
	KAFFINITY ActiveProcessorsAffinityMask;
	CHAR NumberOfProcessors;
} SYSTEM_BASIC_INFORMATION, * PSYSTEM_BASIC_INFORMATION;

typedef struct _SYSTEM_MODULE   // Information Class 11
{
	ULONG_PTR Reserved[2];
	PVOID Base;
	ULONG Size;
	ULONG Flags;
	USHORT Index;
	USHORT Unknown;
	USHORT LoadCount;
	USHORT ModuleNameOffset;
	CHAR ImageName[256];
} SYSTEM_MODULE, * PSYSTEM_MODULE;

typedef struct _SYSTEM_MODULE_INFORMATION   // Information Class 11
{
	ULONG_PTR ulModuleCount;
	SYSTEM_MODULE Modules[1];
} SYSTEM_MODULE_INFORMATION, * PSYSTEM_MODULE_INFORMATION;


typedef struct _HECI_MESSAGE_HEADER {
	UINT32 MEAddress : 8;
	UINT32 HostAddress : 8;
	UINT32 Length : 9;
	UINT32 Reserved : 6;
	UINT32 MessageComplete : 1;
} HECI_MESSAGE_HEADER;

typedef struct _HBM_COMMAND {
	UINT8 Command : 7;
	UINT8 IsResponse : 1;
} HBM_COMMAND;

typedef struct _HBM_FLOW_CONTROL {
	HBM_COMMAND Command;
	UINT8 MEAddress;
	UINT8 HostAddress;
	UINT8 Reserved[5];
}HBM_FLOW_CONTROL;

typedef struct _HBM_HOST_CLIENT_PROPERTIES_REQUEST {
	HBM_COMMAND Command;
	UINT8 Address;
	UINT8 Reserved[2];
} HBM_HOST_CLIENT_PROPERTIES_REQUEST;

typedef struct _HBM_CLIENT_CONNECT_REQUEST {
	HBM_COMMAND Command;
	UINT8 MEAddress;
	UINT8 HostAddress;
	UINT8 Reserved;
} HBM_CLIENT_CONNECT_REQUEST;

struct Hal {
	char(*fp_Write)(INT64, HECI_MESSAGE_HEADER*, PVOID, unsigned int);
	char(*fp_Read)(INT64, HECI_MESSAGE_HEADER*, PVOID, unsigned int);
	char(*fp_Buslayer)(INT64, HECI_MESSAGE_HEADER*);
	char(*fp_Buslayer_properties)(INT64, INT64);
	char(*fp_Buslayer_flow)(INT64, INT64, INT64);
	char(*fp_Buslayer_init)(INT64);
	char(*fp_Buslayer_enum)(INT64);
	char(*fp_Buslayer_Dis)(INT64, INT64, INT64);
	char(*fp_Buslayer_Noti)(INT64, INT64, INT64, INT64);
	char(*fp_Buslayer_ready)(INT64);
	char(*fp_Read_Request)(INT64);
	char(*fp_interrupt_Read)(INT64);
	char(*fp_Write_Request)(INT64);
	char(*fp_ClientCreateAndConnectToInternalClient)(INT64);
	char(*fp_SendFlowControlRequestToDriver)(UINT64, UINT64, UINT64);
	char(*fp_ClientsDisconnect)(UINT64, UINT64);
	char(*fp_SendInternalIoControlRequestSynchronously)(UINT64, UINT32, UINT64, UINT32, UINT64, UINT32, UINT64, BOOLEAN);
	char(*fp_SendInternalIoControlRequest)(UINT64, UINT32, UINT64, UINT32, UINT64, UINT32, UINT64, UINT64);
	char(*fp_HalMeiReadRegister)(UINT64, UINT32);
	//char(*fp_Host_State)(INT64);
	//char(*fp_Me_State)(INT64);
};

NTSTATUS ZwQuerySystemInformation(
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	IN OUT PVOID SystemInformation,
	IN ULONG SystemInformationLength,
	OUT PULONG ReturnLength OPTIONAL);

struct AMT_MSG {
	unsigned int groupID;
	unsigned int command;
	unsigned int isResponse;
	unsigned int reserved;
	unsigned int result;
};

struct CUSTOM_MSG {
	unsigned int uint1;
	unsigned int uint2;
	unsigned int uint3;
	unsigned int uint4;
	unsigned int uint5;
	unsigned int uint6;
	unsigned int uint7;
	unsigned int uint8;
	unsigned int uint9;
	unsigned int uinta;
	unsigned int uintb;
	unsigned int uintc;
	unsigned int uintd;
	unsigned int uinte;
	unsigned int uintf;

};