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

#include <stdlib.h>

#include "ntdll.h"

#include "../debug.h"

#define FUNC(r, a, f, p) typedef r a (*f##_t)p; f##_t f = (f##_t) NULL; char *str_##f = #f;

FUNC(void *, __attribute__((stdcall)), RtlEnterCriticalSection, (RTL_CRITICAL_SECTION *cs))
FUNC(void *, __attribute__((stdcall)), RtlLeaveCriticalSection, (RTL_CRITICAL_SECTION *cs))
FUNC(void *, __attribute__((stdcall)), RtlInitializeCriticalSection, (RTL_CRITICAL_SECTION *cs))
FUNC(void *, __attribute__((stdcall)), RtlDeleteCriticalSection, (RTL_CRITICAL_SECTION *cs))

#undef FUNC
