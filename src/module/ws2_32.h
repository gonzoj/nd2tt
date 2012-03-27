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

#ifndef WS2_32_H_
#define WS2_32_H_

#include "types.h"

typedef DWORD SOCKET;

#define FUNC(r, a, f, p) extern r (*f)p a; extern char *str_##f;

FUNC(int, __attribute__((stdcall)), WS_send, (SOCKET s, const char *buf, int len, int flags))

#undef FUNC

#define _WS2_32_FUNC_START WS_send
#define _WS2_32_FUNC_END WS_send

#define _WS2_32_STR_START str_WS_send
#define _WS2_32_STR_END str_WS_send

#endif /* WS2_32_H_ */
