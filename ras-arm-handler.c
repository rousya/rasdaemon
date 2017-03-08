/*
 * Copyright (c) 2016, The Linux Foundation. All rights reserved.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "libtrace/kbuffer.h"
#include "ras-arm-handler.h"
#include "ras-record.h"
#include "ras-logger.h"
#include "ras-report.h"

int ras_arm_event_handler(struct trace_seq *s,
			 struct pevent_record *record,
			 struct event_format *event, void *context)
{
	unsigned long long val;
	struct ras_events *ras = context;
	time_t now;
	struct tm *tm;
	struct ras_arm_event ev;

	/*
	 * Newer kernels (3.10-rc1 or upper) provide an uptime clock.
	 * On previous kernels, the way to properly generate an event would
	 * be to inject a fake one, measure its timestamp and diff it against
	 * gettimeofday. We won't do it here. Instead, let's use uptime,
	 * falling-back to the event report's time, if "uptime" clock is
	 * not available (legacy kernels).
	 */

	if (ras->use_uptime)
		now = record->ts/user_hz + ras->uptime_diff;
	else
		now = time(NULL);

	tm = localtime(&now);
	if (tm)
		strftime(ev.timestamp, sizeof(ev.timestamp),
			 "%Y-%m-%d %H:%M:%S %z", tm);
	trace_seq_printf(s, "%s\n", ev.timestamp);

	if (pevent_get_field_val(s, event, "affinity", record, &val, 1) < 0)
		return -1;
	ev.affinity = val;
	trace_seq_printf(s, "affinity: %d\n", ev.affinity);

	if (pevent_get_field_val(s, event, "mpidr", record, &val, 1) < 0)
		return -1;
	ev.mpidr = val;
	trace_seq_printf(s, "MPIDR: 0x%llx\n", (unsigned long long)ev.mpidr);

	if (pevent_get_field_val(s, event, "midr", record, &val, 1) < 0)
		return -1;
	ev.midr = val;
	trace_seq_printf(s, "MIDR: 0x%llx\n", (unsigned long long)ev.midr);

	if (pevent_get_field_val(s, event, "running_state", record, &val, 1) < 0)
		return -1;
	ev.running_state = val;
	trace_seq_printf(s, "running_state: %d\n", ev.running_state);

	if (pevent_get_field_val(s, event, "psci_state", record, &val, 1) < 0)
		return -1;
	ev.psci_state = val;
	trace_seq_printf(s, "psci_state: %d\n", ev.psci_state);

	/* Insert data into the SGBD */
#ifdef HAVE_SQLITE3
	ras_store_arm_record(ras, &ev);
#endif

#ifdef HAVE_ABRT_REPORT
	/* Report event to ABRT */
	ras_report_arm_record(ras, &ev);
#endif

	return 0;
}
