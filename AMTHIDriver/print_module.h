#pragma once
#pragma once
#include<string.h>
//#include"driver.h"

BOOLEAN GetKernelInformation(PSYSTEM_MODULE_INFORMATION* pModuleList)
{
	NTSTATUS status = STATUS_SUCCESS;
	ULONG neededSize = 0;

	ZwQuerySystemInformation(
		SystemModuleInformation,
		&neededSize,
		0,
		&neededSize
	);

	*pModuleList = (PSYSTEM_MODULE_INFORMATION)ExAllocatePool(NonPagedPool, neededSize);
	if (*pModuleList == NULL)
	{
		return FALSE;
	}

	status = ZwQuerySystemInformation(SystemModuleInformation,
		*pModuleList,
		neededSize,
		0
	);

	return NT_SUCCESS(status);
}

int PrintMod()
{
	PSYSTEM_MODULE_INFORMATION pModuleList = NULL;

	if (!GetKernelInformation(&pModuleList))
		goto CLEANUP;

	for (ULONG i = 0; i < pModuleList->ulModuleCount; i++)
	{
		PSYSTEM_MODULE mod = &pModuleList->Modules[i];
		//printf("%s @ %p\n", mod->ImageName, mod->Base);
		//DbgPrintEx(DPFLTR_DEFAULT_ID, 0, "%s @ %p\n", mod->ImageName, mod->Base);
		if (strstr(mod->ImageName, "TeeDriverW8x64") != NULL) {
			DbgPrintEx(DPFLTR_DEFAULT_ID, 0, "%p\n", mod->Base);
		}
	}


CLEANUP:
	return 0;
}
