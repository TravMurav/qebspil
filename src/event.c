// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2025 Stephan Gerhold <stephan@gerhold.net>
 */

#include <efi.h>
#include <efilib.h>

#include "qebspil.h"

// https://github.com/tianocore/edk2/blob/8216419a02173421ce7070268fdd11a7caadfa4b/MdeModulePkg/Core/Dxe/Event/Event.h#L37-L50
#define EVENT_SIGNATURE  EFI_SIGNATURE_32('e','v','n','t')
typedef struct {
	UINTN Signature;
	UINT32 Type;
	UINT32 SignalCount;
	LIST_ENTRY SignalLink;
	EFI_TPL NotifyTpl;
	/* ... */
} IEVENT;

static void EFIAPI event_notify(EFI_EVENT event, VOID *ctx)
{
	event_callback_fn fn = ctx;
	EFI_STATUS status;

	(void)event;
	status = fn();
	if (EFI_ERROR(status))
		Print(u"qebspil: Event callback failed: %r\n", status);
}

EFI_STATUS event_register_group_callback(const EFI_GUID *group, EFI_EVENT *event, event_callback_fn fn)
{
	return BS->CreateEventEx(EVT_NOTIFY_SIGNAL, EFI_TPL_CALLBACK, event_notify, fn, group, event);
}

EFI_STATUS event_register_late_ebs_callback(EFI_EVENT *event, event_callback_fn fn)
{
	EFI_STATUS status;
	IEVENT *ievent;

	status = BS->CreateEvent(EFI_EVENT_SIGNAL_EXIT_BOOT_SERVICES, EFI_TPL_CALLBACK, event_notify, fn, event);
	if (EFI_ERROR(status))
		return status;

	/*
	 * HACK: We want to run as late as possible during ExitBootServices(),
	 * i.e. on EFI_TPL_CALLBACK-1. Unfortunately, EDK2 doesn't let EFI
	 * applications register callbacks on intermediate TPLs. Use a crude
	 * hack for now by replacing the TPL value in the internal EDK2 data
	 * structure. This might not work for other EFI implementations.
	 */
	ievent = *event;
	if (ievent->Signature == EVENT_SIGNATURE &&
	    ievent->Type == EFI_EVENT_SIGNAL_EXIT_BOOT_SERVICES &&
	    ievent->NotifyTpl == EFI_TPL_CALLBACK)
		ievent->NotifyTpl = EFI_TPL_CALLBACK - 1;
	else
		Print(u"qebspil: Unexpected IEvent structure (not edk2)? Signature: 0x%x, Type: 0x%x, TPL: %d. This will probably cause problems later.\n",
		      ievent->Signature, ievent->Type, ievent->NotifyTpl);

	return EFI_SUCCESS;
}
