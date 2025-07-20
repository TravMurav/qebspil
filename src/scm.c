// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2025 Stephan Gerhold <stephan@gerhold.net>
 */

#include <efi.h>
#include <efilib.h>

#include "qebspil.h"

#include "external/EFIScm.h"
#include "external/scm_sip_interface.h"

static QCOM_SCM_PROTOCOL *scm;

// https://git.codelinaro.org/clo/la/abl/tianocore/edk2/-/blob/72c4d886ce585d584f2e3eeb87fd3761819824e7/QcomModulePkg/QcomModulePkg.dec#L102
EFI_GUID gQcomScmProtocolGuid = { 0x77ed108d, 0x8524, 0x4b8b, { 0x9d, 0x2e, 0x34, 0x98, 0x7a, 0xec, 0xb9, 0xc1 } };

EFI_STATUS scm_init(void)
{
	EFI_STATUS status;

	status = LibLocateProtocol(&gQcomScmProtocolGuid, (VOID**)&scm);
	if (EFI_ERROR(status)) {
		Print(u"Failed to find QCOM SCM protocol: %r\n", status);
		return status;
	}

	Print(u"Found QCOM SCM protocol version 0x%x\n", scm->Revision);
	return EFI_SUCCESS;
}

EFI_STATUS scm_pil_init(UINT8 pas_id, EFI_PHYSICAL_ADDRESS metadata)
{
	UINT64 Parameters[SCM_MAX_NUM_PARAMETERS] = {
		pas_id,
		metadata,
	};
	UINT64 Results[SCM_MAX_NUM_RESULTS];

	Print(u"Initializing PIL 0x%x\n", pas_id);
	return scm->ScmSipSysCall(scm, TZ_PIL_INIT_ID, TZ_PIL_INIT_ID_PARAM_ID, Parameters, Results);
}

EFI_STATUS scm_pil_start(UINT8 pas_id)
{
	UINT64 Parameters[SCM_MAX_NUM_PARAMETERS] = {
		pas_id,
	};
	UINT64 Results[SCM_MAX_NUM_RESULTS];

	Print(u"Starting PIL 0x%x\n", pas_id);
	return scm->ScmSipSysCall(scm, TZ_PIL_AUTH_RESET_ID, TZ_PIL_AUTH_RESET_ID_PARAM_ID, Parameters, Results);
}

EFI_STATUS scm_pil_stop(UINT8 pas_id)
{
	UINT64 Parameters[SCM_MAX_NUM_PARAMETERS] = {
		pas_id,
	};
	UINT64 Results[SCM_MAX_NUM_RESULTS];

	Print(u"Stopping PIL 0x%x\n", pas_id);
	return scm->ScmSipSysCall(scm, TZ_PIL_UNLOCK_XPU_ID, TZ_PIL_UNLOCK_XPU_ID_PARAM_ID, Parameters, Results);
}
