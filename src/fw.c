// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2025 Stephan Gerhold <stephan@gerhold.net>
 */

#include <stdalign.h>

#include <efi.h>
#include <efilib.h>

#include <elf.h>

#include "external/CacheMaintenanceLib.h"
#include "qebspil.h"

// https://github.com/tianocore/edk2/blob/438045682bee1d075489e584d290b91f1430f73e/FatPkg/EnhancedFatDxe/Fat.h#L113C1-L113C37
#define EFI_PATH_STRING_LENGTH	260

#define FW_PATH_PREFIX		u"firmware\\"
#define FW_PATH_MAX_LEN		(EFI_PATH_STRING_LENGTH - ARRAY_SIZE(FW_PATH_PREFIX))

#define UTF8_BIT		0x80

#define METADATA_PAGES		4
#define MAX_METADATA_SIZE	(METADATA_PAGES * EFI_PAGE_SIZE)

static EFI_STATUS fw_read_file(EFI_FILE_HANDLE root, CHAR16 *path, VOID **buf, UINTN *len)
{
	EFI_FILE_HANDLE file;
	EFI_FILE_INFO *info;
	EFI_STATUS status;
	UINTN size;

	status = root->Open(root, &file, path, EFI_FILE_MODE_READ, 0);
	if (EFI_ERROR(status))
		return status;

	info = LibFileInfo(file);
	if (!info) {
		status = EFI_VOLUME_CORRUPTED;
		goto out;
	}

	*len = info->FileSize;
	FreePool(info);

	*buf = AllocatePool(*len);
	if (!*buf) {
		status = EFI_OUT_OF_RESOURCES;
		goto out;
	}

	size = *len;
	status = file->Read(file, &size, *buf);
	if (EFI_ERROR(status))
		goto out;

	if (size != *len) {
		status = EFI_END_OF_FILE;
		goto out;
	}

out:
	file->Close(file);
	return status;
}

static BOOLEAN fw_convert_path(CHAR16 *path, const char *fw_name)
{
	for (UINTN i = 0; i < FW_PATH_MAX_LEN; ++i) {
		if (!fw_name[i]) {
			path[i] = 0;
			return TRUE;
		}

		if (fw_name[i] & UTF8_BIT)
			return FALSE; /* Only ASCII supported for now */

		if (fw_name[i] == '/')
			path[i] = '\\';
		else
			path[i] = fw_name[i];
	}

	/* Path too long */
	return FALSE;
}

EFI_STATUS fw_prepare(struct pil_fw *fw, EFI_FILE_HANDLE root, const char *fw_name)
{
	CHAR16 path[EFI_PATH_STRING_LENGTH] = FW_PATH_PREFIX;
	EFI_STATUS status;

	if (!*fw_name || *fw_name == '/' || *fw_name == '\\' || *fw_name == '.')
		return EFI_INVALID_PARAMETER;

	if (!fw_convert_path(&path[ARRAY_SIZE(FW_PATH_PREFIX) - 1], fw_name))
		return EFI_INVALID_PARAMETER;

	status = fw_read_file(root, path, &fw->elf_data, &fw->elf_size);
	if (EFI_ERROR(status))
		return status;

	status = BS->AllocatePages(AllocateAnyPages, PoolAllocationType, METADATA_PAGES, &fw->metadata);
	if (EFI_ERROR(status))
		return status;

	return EFI_SUCCESS;
}

/* Used to upgrade integers to 64-bit to avoid overflows with 32-bit numbers */
static inline UINT64 u64(UINT64 a)
{
	return a;
}

EFI_STATUS fw_check(const struct pil_fw *fw)
{
	const Elf32_Ehdr *ehdr = fw->elf_data;
	const Elf32_Phdr *phdr;

	if (!ehdr)
		return EFI_LOAD_ERROR;

	if (fw->elf_size < sizeof(*ehdr))
		return EFI_END_OF_FILE;

	if (CompareMem(ehdr->e_ident, ELFMAG, SELFMAG))
		return EFI_VOLUME_CORRUPTED;

	if (ehdr->e_ident[EI_CLASS] != ELFCLASS32)
		return EFI_UNSUPPORTED;

	if (ehdr->e_ident[EI_VERSION] != EV_CURRENT || ehdr->e_version != EV_CURRENT)
		return EFI_INCOMPATIBLE_VERSION;

	if (ehdr->e_ehsize != sizeof(*ehdr) || ehdr->e_phentsize != sizeof(*phdr))
		return EFI_INVALID_PARAMETER;

	if (ehdr->e_phoff % alignof(*phdr))
		return EFI_INVALID_PARAMETER;

	if (fw->elf_size < (ehdr->e_phoff + (u64(ehdr->e_phentsize) * ehdr->e_phnum)))
		return EFI_END_OF_FILE;

	phdr = fw->elf_data + ehdr->e_phoff;
	for (Elf32_Half i = 0; i < ehdr->e_phnum; i++, phdr++) {
		if (phdr->p_type == PT_LOAD && phdr->p_filesz > phdr->p_memsz)
			return EFI_INVALID_PARAMETER;
		if (phdr->p_paddr > (phdr->p_paddr + phdr->p_memsz))
			return EFI_SECURITY_VIOLATION; /* Address overflow */
		if (phdr->p_filesz > 0 &&
		    fw->elf_size < (u64(phdr->p_offset) + phdr->p_filesz))
			return EFI_END_OF_FILE;
	}

	return EFI_SUCCESS;
}

EFI_STATUS fw_load_metadata(struct pil_fw *fw)
{
	const Elf32_Ehdr *ehdr = fw->elf_data;
	const Elf32_Phdr *phdr = fw->elf_data + ehdr->e_phoff;
	VOID *metadata = (VOID*)fw->metadata;
	UINT64 pos = 0;

	for (Elf32_Half i = 0; i < ehdr->e_phnum; i++, phdr++) {
		if (phdr->p_type != PT_NULL || phdr->p_filesz == 0)
			continue;
		if ((pos + phdr->p_filesz) > MAX_METADATA_SIZE)
			return EFI_BUFFER_TOO_SMALL;

		CopyMem(metadata + pos, fw->elf_data + phdr->p_offset, phdr->p_filesz);
		pos += phdr->p_filesz;
	}

	if (pos == 0)
		return EFI_NOT_FOUND;

	WriteBackInvalidateDataCacheRange(metadata, MAX_METADATA_SIZE);
	return EFI_SUCCESS;
}

EFI_STATUS fw_load(const struct pil_fw *fw)
{
	const Elf32_Ehdr *ehdr = fw->elf_data;
	const Elf32_Phdr *phdr = fw->elf_data + ehdr->e_phoff;
	VOID *base = (VOID*)(UINTN)fw->mem_addr;
	Elf32_Addr min_addr = 0xffffffff;
	Elf32_Addr max_addr = 0x00000000;
	Elf32_Addr size;

	/* Find min and max address for relocation */
	for (Elf32_Half i = 0; i < ehdr->e_phnum; i++, phdr++) {
		if (phdr->p_type != PT_LOAD)
			continue;

		if (phdr->p_paddr < min_addr)
			min_addr = phdr->p_paddr;
		if ((phdr->p_paddr + phdr->p_memsz) > max_addr)
			max_addr = phdr->p_paddr + phdr->p_memsz;
	}

	if (min_addr > max_addr)
		return EFI_NOT_FOUND;

	size = max_addr - min_addr;
	if (size > fw->mem_size)
		return EFI_BUFFER_TOO_SMALL;

	/* Load segments */
	phdr = fw->elf_data + ehdr->e_phoff;
	for (Elf32_Half i = 0; i < ehdr->e_phnum; i++, phdr++) {
		VOID *dest;

		if (phdr->p_type != PT_LOAD)
			continue;

		dest = base + (phdr->p_paddr - min_addr);
		if (phdr->p_filesz > 0)
			CopyMem(dest, fw->elf_data + phdr->p_offset, phdr->p_filesz);
		if (phdr->p_memsz > phdr->p_filesz)
			ZeroMem(dest + phdr->p_filesz, phdr->p_memsz - phdr->p_filesz);
	}

	WriteBackInvalidateDataCacheRange(base, fw->mem_size);
	return EFI_SUCCESS;
}

void fw_free(struct pil_fw *fw)
{
	if (fw->elf_data)
		FreePool(fw->elf_data);
	if (fw->metadata)
		BS->FreePages(fw->metadata, METADATA_PAGES);
}
