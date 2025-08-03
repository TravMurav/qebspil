// SPDX-License-Identifier: BSD-2-Clause-Patent
/*
 * Adapted from edk2 source code (BaseLib, BaseMemoryLib, BaseFdtLib):
 *
 * Copyright (c) 2006 - 2019, 2023, Intel Corporation. All rights reserved.
 * Copyright (c) 2023 Pedro Falcato All rights reserved.
 */

#include <efi.h>
#include <efilib.h>

#include <string.h>

static
CONST VOID *
EFIAPI
InternalMemScanMem8 (
  IN      CONST VOID  *Buffer,
  IN      UINTN       Length,
  IN      UINT8       Value
  )
{
  CONST UINT8  *Pointer;

  Pointer = (CONST UINT8 *)Buffer;
  do {
    if (*Pointer == Value) {
      return Pointer;
    }

    ++Pointer;
  } while (--Length != 0);

  return NULL;
}

static
UINTN
EFIAPI
AsciiStrnLenS (
  IN CONST CHAR8  *String,
  IN UINTN        MaxSize
  )
{
  UINTN  Length;

  if ((String == NULL) || (MaxSize == 0)) {
    return 0;
  }

  Length = 0;
  while (String[Length] != 0) {
    if (Length >= MaxSize - 1) {
      return MaxSize;
    }

    Length++;
  }

  return Length;
}

static
char *
fdt_strrchr (
  const char  *Str,
  int         Char
  )
{
  char  *S, *last;

  S    = (char *)Str;
  last = NULL;

  for ( ; ; S++) {
    if (*S == Char) {
      last = S;
    }

    if (*S == '\0') {
      return last;
    }
  }
}

void *memmove(void *dest, const void *src, size_t n)
{
	CopyMem(dest, (VOID*)src, n);
	return dest;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
	return CompareMem(s1, s2, n);
}

void *memchr(const void *s, int c, size_t n)
{
	return (void*)InternalMemScanMem8(s, n, c);
}

int strcmp(const char *s1, const char *s2)
{
	return AsciiStrCmp((const CHAR8*)s1, (const CHAR8*)s2);
}

int strncmp(const char *s1, const char *s2, size_t n)
{
	return AsciiStrnCmp((const CHAR8*)s1, (const CHAR8*)s2, n);
}

char *strrchr(const char *s, int c)
{
	return fdt_strrchr(s, c);
}

size_t strlen(const char *s)
{
	return AsciiStrLen((const CHAR8*)s);
}

size_t strnlen(const char *string, size_t maxlen)
{
	return AsciiStrnLenS((const CHAR8*)string, maxlen);
}
