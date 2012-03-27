/*
 * Copyright (C) 2011 gonzoj
 *
 * Please check the CREDITS file for further information.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NTDLL_H_
#define NTDLL_H_

typedef struct _RTL_CRITICAL_SECTION {
	void *DebugInfo;
	long LockCount;
	long RecursionCount;
	void *OwningThread;
	void *LockSemaphore;
	unsigned long *SpinCount;
} RTL_CRITICAL_SECTION;

#define FUNC(r, a, f, p) extern r (*f)p a; extern char *str_##f;

FUNC(void *, __attribute__((stdcall)), RtlEnterCriticalSection, (RTL_CRITICAL_SECTION *cs))
FUNC(void *, __attribute__((stdcall)), RtlLeaveCriticalSection, (RTL_CRITICAL_SECTION *cs))
FUNC(void *, __attribute__((stdcall)), RtlInitializeCriticalSection, (RTL_CRITICAL_SECTION *cs))
FUNC(void *, __attribute__((stdcall)), RtlDeleteCriticalSection, (RTL_CRITICAL_SECTION *cs))

#undef FUNC

#define _NTDLL_FUNC_START RtlEnterCriticalSection
#define _NTDLL_FUNC_END RtlDeleteCriticalSection

#define _NTDLL_STR_START str_RtlEnterCriticalSection
#define _NTDLL_STR_END str_RtlDeleteCriticalSection

#endif /* NTDLL_H_ */
