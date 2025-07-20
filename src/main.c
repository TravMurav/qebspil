// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2025 Stephan Gerhold <stephan@gerhold.net>
 */

#include <efi.h>
#include <efilib.h>

#include "qebspil.h"

static EFI_STATUS pil_load(EFI_FILE_HANDLE root, CHAR16 *path, UINT8 pas_id)
{
	EFI_PHYSICAL_ADDRESS metadata;
	EFI_STATUS status;

	Print(u"Loading %s\n", path);

	status = fw_load(root, path, &metadata);
	if (EFI_ERROR(status))
		return status;

	return scm_pil_init(pas_id, metadata);
}

static EFI_STATUS efi_late_ebs(void)
{
	Print(u"Reached late exit boot services. (Re)Starting PILs!\n");

	return scm_pil_start(0x1);
}

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *system_table)
{
	EFI_LOADED_IMAGE_PROTOCOL *loaded_image;
	EFI_FILE_HANDLE root;
	EFI_STATUS status;
	EFI_EVENT event;

	InitializeLib(image, system_table);

	Print(u"Hello World!\n");

	status = BS->HandleProtocol(image, &LoadedImageProtocol, (VOID**)&loaded_image);
	if (EFI_ERROR(status))
		return status;

	status = scm_init();
	if (EFI_ERROR(status))
		return status;

	root = LibOpenRoot(loaded_image->DeviceHandle);
	if (!root) {
		Print(u"Failed to open root volume. Not started from file?\n");
		return EFI_NOT_FOUND;
	}

	status = pil_load(root, u"qcadsp8280.mbn", 0x1);
	if (EFI_ERROR(status))
		return status;

	root->Close(root);
	return event_register_late_ebs_callback(&event, efi_late_ebs);
}
