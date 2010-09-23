/* MSPDebug - debugging tool for the eZ430
 * Copyright (C) 2009, 2010 Daniel Beer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <string.h>
#include "device.h"
#include "prog.h"
#include "output.h"

void prog_init(struct prog_data *prog, int flags)
{
	memset(prog, 0, sizeof(*prog));
	prog->flags = flags;
}

int prog_flush(struct prog_data *prog)
{
	if (!prog->len)
		return 0;

	if (!prog->have_erased && (prog->flags & PROG_WANT_ERASE)) {
		printc("Erasing...\n");
		if (device_default->erase(device_default,
					  DEVICE_ERASE_MAIN, 0) < 0)
			return -1;

		prog->have_erased = 1;
	}

	printc_dbg("Writing %3d bytes to %04x...\n", prog->len, prog->addr);
	if (device_default->writemem(device_default, prog->addr,
				     prog->buf, prog->len) < 0)
		return -1;

	prog->addr += prog->len;
	prog->len = 0;
	return 0;
}

int prog_feed(struct prog_data *prog, address_t addr,
	      const uint8_t *data, int len)
{
	/* Flush if this section is discontiguous */
	if (prog->len && prog->addr + prog->len != addr &&
	    prog_flush(prog) < 0)
		return -1;

	if (!prog->len)
		prog->addr = addr;

	/* Add the buffer in piece by piece, flushing when it gets
	 * full.
	 */
	while (len) {
		int count = sizeof(prog->buf) - prog->len;

		if (count > len)
			count = len;

		if (!count) {
			if (prog_flush(prog) < 0)
				return -1;
		} else {
			memcpy(prog->buf + prog->len, data, count);
			prog->len += count;
			data += count;
			len -= count;
		}
	}

	return 0;
}
