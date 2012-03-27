
#  Copyright (C) 2011 gonzoj
#
#  Please check the CREDITS file for further information.
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.


.section .text

.globl draw_ingame_STUB

.globl join_game_STUB

.globl send_packet_STUB

.type draw_ingame_STUB, @function
draw_ingame_STUB:

	call draw_ingame
	pop %esi
	pop %ebx
	pop %ecx
	ret $0x4

.type join_game_STUB, @function
join_game_STUB:

	push %ebx
	push %edi

	push $0x0
	push $0x0
	lea 0x0(%edx), %ebx
	lea 0x0(%ecx), %edi

	call *D2MULTI_join_game_1

	call *D2MULTI_join_game_2

	pop %edi
	pop %ebx
	ret

.type send_packet_STUB, @function
send_packet_STUB:

	mov 0x04(%esp), %eax
	mov 0x08(%esp), %ecx
	mov 0x0c(%esp), %edx

	push %edx
	push %ecx
	push %eax

	call on_send_packet

	mov D2NET_send_packet, %eax
	jmp *%eax
