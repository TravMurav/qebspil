// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2025 Stephan Gerhold <stephan@gerhold.net>
 */

#include <efi.h>
#include <efilib.h>

#include <elf.h>

#include "external/CacheMaintenanceLib.h"
#include "qebspil.h"

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

static EFI_STATUS fw_parse_mbn_elf(VOID *buf, UINTN len, VOID *metadata)
{
	Elf32_Ehdr *ehdr = buf;
	UINTN metadata_pos = 0;
	UINTN min_addr = ~0UL;
	UINTN max_addr = 0UL;
	VOID *phdr_buf;

	if (len < sizeof(*ehdr))
		return EFI_END_OF_FILE;

	if (CompareMem(ehdr->e_ident, ELFMAG, SELFMAG))
		return EFI_VOLUME_CORRUPTED;

	if (ehdr->e_ident[EI_CLASS] != ELFCLASS32)
		return EFI_UNSUPPORTED;

	if (ehdr->e_type != ET_EXEC)
		return EFI_UNSUPPORTED;

	Print(u"%d phdrs, size %d offset 0x%x\n", ehdr->e_phnum, ehdr->e_phentsize, ehdr->e_phoff);

	if (len < (ehdr->e_phoff + (ehdr->e_phentsize * ehdr->e_phnum)))
		return EFI_END_OF_FILE;

	phdr_buf = buf + ehdr->e_phoff;
	for (UINTN i = 0; i < ehdr->e_phnum; i++, phdr_buf += ehdr->e_phentsize) {
		Elf32_Phdr *phdr = phdr_buf;

		Print(u"phdr %d: type=%d, offset=0x%x, vaddr=0x%x, paddr=0x%x, filesz=0x%x, memsz=0x%x, flags=0x%x, align=0x%x\n",
		      i, phdr->p_type, phdr->p_offset, phdr->p_vaddr, phdr->p_paddr, phdr->p_filesz, phdr->p_memsz, phdr->p_filesz, phdr->p_align);

		if (phdr->p_filesz > 0) {
			if (len < (phdr->p_offset + phdr->p_filesz))
				return EFI_END_OF_FILE;

			switch (phdr->p_type) {
			case PT_LOAD:
				CopyMem((VOID*)(UINTN)phdr->p_paddr, buf + phdr->p_offset, phdr->p_filesz);
				break;
			case PT_NULL:
				if ((metadata_pos + phdr->p_filesz) > MAX_METADATA_SIZE)
					return EFI_OUT_OF_RESOURCES;

				CopyMem(metadata + metadata_pos, buf + phdr->p_offset, phdr->p_filesz);
				metadata_pos += phdr->p_filesz;
				break;
			}
		}

		if (phdr->p_type == PT_LOAD) {
			UINTN end = phdr->p_paddr + phdr->p_memsz;

			if (phdr->p_memsz > phdr->p_filesz)
				ZeroMem((VOID*)(UINTN)phdr->p_paddr + phdr->p_filesz, phdr->p_memsz - phdr->p_filesz);

			if (phdr->p_paddr < min_addr)
				min_addr = phdr->p_paddr;

			if (end > max_addr)
				max_addr = end;
		}
	}

	if (min_addr < ~0UL && max_addr > 0)
		WriteBackInvalidateDataCacheRange((VOID*)min_addr, max_addr - min_addr);
	if (metadata_pos > 0)
		WriteBackInvalidateDataCacheRange(metadata, metadata_pos);

	return EFI_SUCCESS;
}

EFI_STATUS fw_load(EFI_FILE_HANDLE root, CHAR16 *path, EFI_PHYSICAL_ADDRESS *metadata)
{
	EFI_STATUS status;
	VOID *buf = NULL;
	UINTN len;

	status = fw_read_file(root, path, &buf, &len);
	if (EFI_ERROR(status))
		goto out;

	status = BS->AllocatePages(AllocateAnyPages, PoolAllocationType, METADATA_PAGES, metadata);
	if (EFI_ERROR(status))
		goto out;

	status = fw_parse_mbn_elf(buf, len, (VOID*)(UINTN)*metadata);
out:
	if (buf)
		FreePool(buf);
	return status;
}
