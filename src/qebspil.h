/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef QEBSPIL_H
#define QEBSPIL_H

#include <efi.h>

typedef EFI_STATUS (*event_callback_fn)(void);
EFI_STATUS event_register_group_callback(const EFI_GUID *group, EFI_EVENT *event, event_callback_fn fn);
EFI_STATUS event_register_late_ebs_callback(EFI_EVENT *event, event_callback_fn fn);

EFI_STATUS fw_load(EFI_FILE_HANDLE root, CHAR16 *path, EFI_PHYSICAL_ADDRESS *metadata);

EFI_STATUS scm_init(void);
EFI_STATUS scm_pil_init(UINT8 pas_id, EFI_PHYSICAL_ADDRESS metadata);
EFI_STATUS scm_pil_start(UINT8 pas_id);
EFI_STATUS scm_pil_stop(UINT8 pas_id);

#endif
