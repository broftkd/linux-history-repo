/*
 * OSS compatible sequencer driver
 *
 * open/close and reset interface
 *
 * Copyright (C) 1998-1999 Takashi Iwai <tiwai@suse.de>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include "seq_oss_device.h"
#include "seq_oss_synth.h"
#include "seq_oss_midi.h"
#include "seq_oss_writeq.h"
#include "seq_oss_readq.h"
#include "seq_oss_timer.h"
#include "seq_oss_event.h"
#include <linux/init.h>
#include <linux/moduleparam.h>

/*
 * common variables
 */
static int maxqlen = SNDRV_SEQ_OSS_MAX_QLEN;
module_param(maxqlen, int, 0444);
MODULE_PARM_DESC(maxqlen, "maximum queue length");

static int system_client = -1; /* ALSA sequencer client number */
static int system_port = -1;

static int num_clients;
static seq_oss_devinfo_t *client_table[SNDRV_SEQ_OSS_MAX_CLIENTS];


/*
 * prototypes
 */
static int receive_announce(snd_seq_event_t *ev, int direct, void *private, int atomic, int hop);
static int translate_mode(struct file *file);
static int create_port(seq_oss_devinfo_t *dp);
static int delete_port(seq_oss_devinfo_t *dp);
static int alloc_seq_queue(seq_oss_devinfo_t *dp);
static int delete_seq_queue(int queue);
static void free_devinfo(void *private);

#define call_ctl(type,rec) snd_seq_kernel_client_ctl(system_client, type, rec)


/*
 * create sequencer client for OSS sequencer
 */
int __init
snd_seq_oss_create_client(void)
{
	int rc;
	snd_seq_client_callback_t callback;
	snd_seq_client_info_t *info;
	snd_seq_port_info_t *port;
	snd_seq_port_callback_t port_callback;

	info = kmalloc(sizeof(*info), GFP_KERNEL);
	port = kmalloc(sizeof(*port), GFP_KERNEL);
	if (!info || !port) {
		rc = -ENOMEM;
		goto __error;
	}

	/* create ALSA client */
	memset(&callback, 0, sizeof(callback));

	callback.private_data = NULL;
	callback.allow_input = 1;
	callback.allow_output = 1;

	rc = snd_seq_create_kernel_client(NULL, SNDRV_SEQ_CLIENT_OSS, &callback);
	if (rc < 0)
		goto __error;

	system_client = rc;
	debug_printk(("new client = %d\n", rc));

	/* set client information */
	memset(info, 0, sizeof(*info));
	info->client = system_client;
	info->type = KERNEL_CLIENT;
	strcpy(info->name, "OSS sequencer");

	rc = call_ctl(SNDRV_SEQ_IOCTL_SET_CLIENT_INFO, info);

	/* look up midi devices */
	snd_seq_oss_midi_lookup_ports(system_client);

	/* create annoucement receiver port */
	memset(port, 0, sizeof(*port));
	strcpy(port->name, "Receiver");
	port->addr.client = system_client;
	port->capability = SNDRV_SEQ_PORT_CAP_WRITE; /* receive only */
	port->type = 0;

	memset(&port_callback, 0, sizeof(port_callback));
	/* don't set port_callback.owner here. otherwise the module counter
	 * is incremented and we can no longer release the module..
	 */
	port_callback.event_input = receive_announce;
	port->kernel = &port_callback;
	
	call_ctl(SNDRV_SEQ_IOCTL_CREATE_PORT, port);
	if ((system_port = port->addr.port) >= 0) {
		snd_seq_port_subscribe_t subs;

		memset(&subs, 0, sizeof(subs));
		subs.sender.client = SNDRV_SEQ_CLIENT_SYSTEM;
		subs.sender.port = SNDRV_SEQ_PORT_SYSTEM_ANNOUNCE;
		subs.dest.client = system_client;
		subs.dest.port = system_port;
		call_ctl(SNDRV_SEQ_IOCTL_SUBSCRIBE_PORT, &subs);
	}
	rc = 0;

 __error:
	kfree(port);
	kfree(info);
	return rc;
}


/*
 * receive annoucement from system port, and check the midi device
 */
static int
receive_announce(snd_seq_event_t *ev, int direct, void *private, int atomic, int hop)
{
	snd_seq_port_info_t pinfo;

	if (atomic)
		return 0; /* it must not happen */

	switch (ev->type) {
	case SNDRV_SEQ_EVENT_PORT_START:
	case SNDRV_SEQ_EVENT_PORT_CHANGE:
		if (ev->data.addr.client == system_client)
			break; /* ignore myself */
		memset(&pinfo, 0, sizeof(pinfo));
		pinfo.addr = ev->data.addr;
		if (call_ctl(SNDRV_SEQ_IOCTL_GET_PORT_INFO, &pinfo) >= 0)
			snd_seq_oss_midi_check_new_port(&pinfo);
		break;

	case SNDRV_SEQ_EVENT_PORT_EXIT:
		if (ev->data.addr.client == system_client)
			break; /* ignore myself */
		snd_seq_oss_midi_check_exit_port(ev->data.addr.client,
						ev->data.addr.port);
		break;
	}
	return 0;
}


/*
 * delete OSS sequencer client
 */
int
snd_seq_oss_delete_client(void)
{
	if (system_client >= 0)
		snd_seq_delete_kernel_client(system_client);

	snd_seq_oss_midi_clear_all();

	return 0;
}


/*
 * open sequencer device
 */
int
snd_seq_oss_open(struct file *file, int level)
{
	int i, rc;
	seq_oss_devinfo_t *dp;

	if ((dp = kzalloc(sizeof(*dp), GFP_KERNEL)) == NULL) {
		snd_printk(KERN_ERR "can't malloc device info\n");
		return -ENOMEM;
	}
	debug_printk(("oss_open: dp = %p\n", dp));

	for (i = 0; i < SNDRV_SEQ_OSS_MAX_CLIENTS; i++) {
		if (client_table[i] == NULL)
			break;
	}
	if (i >= SNDRV_SEQ_OSS_MAX_CLIENTS) {
		snd_printk(KERN_ERR "too many applications\n");
		kfree(dp);
		return -ENOMEM;
	}

	dp->index = i;
	dp->cseq = system_client;
	dp->port = -1;
	dp->queue = -1;
	dp->readq = NULL;
	dp->writeq = NULL;

	/* look up synth and midi devices */
	snd_seq_oss_synth_setup(dp);
	snd_seq_oss_midi_setup(dp);

	if (dp->synth_opened == 0 && dp->max_mididev == 0) {
		/* snd_printk(KERN_ERR "no device found\n"); */
		rc = -ENODEV;
		goto _error;
	}

	/* create port */
	debug_printk(("create new port\n"));
	if ((rc = create_port(dp)) < 0) {
		snd_printk(KERN_ERR "can't create port\n");
		goto _error;
	}

	/* allocate queue */
	debug_printk(("allocate queue\n"));
	if ((rc = alloc_seq_queue(dp)) < 0)
		goto _error;

	/* set address */
	dp->addr.client = dp->cseq;
	dp->addr.port = dp->port;
	/*dp->addr.queue = dp->queue;*/
	/*dp->addr.channel = 0;*/

	dp->seq_mode = level;

	/* set up file mode */
	dp->file_mode = translate_mode(file);

	/* initialize read queue */
	debug_printk(("initialize read queue\n"));
	if (is_read_mode(dp->file_mode)) {
		if ((dp->readq = snd_seq_oss_readq_new(dp, maxqlen)) == NULL) {
			rc = -ENOMEM;
			goto _error;
		}
	}

	/* initialize write queue */
	debug_printk(("initialize write queue\n"));
	if (is_write_mode(dp->file_mode)) {
		dp->writeq = snd_seq_oss_writeq_new(dp, maxqlen);
		if (dp->writeq == NULL) {
			rc = -ENOMEM;
			goto _error;
		}
	}

	/* initialize timer */
	debug_printk(("initialize timer\n"));
	if ((dp->timer = snd_seq_oss_timer_new(dp)) == NULL) {
		snd_printk(KERN_ERR "can't alloc timer\n");
		rc = -ENOMEM;
		goto _error;
	}
	debug_printk(("timer initialized\n"));

	/* set private data pointer */
	file->private_data = dp;

	/* set up for mode2 */
	if (level == SNDRV_SEQ_OSS_MODE_MUSIC)
		snd_seq_oss_synth_setup_midi(dp);
	else if (is_read_mode(dp->file_mode))
		snd_seq_oss_midi_open_all(dp, SNDRV_SEQ_OSS_FILE_READ);

	client_table[dp->index] = dp;
	num_clients++;

	debug_printk(("open done\n"));
	return 0;

 _error:
	snd_seq_oss_synth_cleanup(dp);
	snd_seq_oss_midi_cleanup(dp);
	i = dp->queue;
	delete_port(dp);
	delete_seq_queue(i);

	return rc;
}

/*
 * translate file flags to private mode
 */
static int
translate_mode(struct file *file)
{
	int file_mode = 0;
	if ((file->f_flags & O_ACCMODE) != O_RDONLY)
		file_mode |= SNDRV_SEQ_OSS_FILE_WRITE;
	if ((file->f_flags & O_ACCMODE) != O_WRONLY)
		file_mode |= SNDRV_SEQ_OSS_FILE_READ;
	if (file->f_flags & O_NONBLOCK)
		file_mode |= SNDRV_SEQ_OSS_FILE_NONBLOCK;
	return file_mode;
}


/*
 * create sequencer port
 */
static int
create_port(seq_oss_devinfo_t *dp)
{
	int rc;
	snd_seq_port_info_t port;
	snd_seq_port_callback_t callback;

	memset(&port, 0, sizeof(port));
	port.addr.client = dp->cseq;
	sprintf(port.name, "Sequencer-%d", dp->index);
	port.capability = SNDRV_SEQ_PORT_CAP_READ|SNDRV_SEQ_PORT_CAP_WRITE; /* no subscription */
	port.type = SNDRV_SEQ_PORT_TYPE_SPECIFIC;
	port.midi_channels = 128;
	port.synth_voices = 128;

	memset(&callback, 0, sizeof(callback));
	callback.owner = THIS_MODULE;
	callback.private_data = dp;
	callback.event_input = snd_seq_oss_event_input;
	callback.private_free = free_devinfo;
	port.kernel = &callback;

	rc = call_ctl(SNDRV_SEQ_IOCTL_CREATE_PORT, &port);
	if (rc < 0)
		return rc;

	dp->port = port.addr.port;
	debug_printk(("new port = %d\n", port.addr.port));

	return 0;
}

/*
 * delete ALSA port
 */
static int
delete_port(seq_oss_devinfo_t *dp)
{
	if (dp->port < 0)
		return 0;

	debug_printk(("delete_port %i\n", dp->port));
	return snd_seq_event_port_detach(dp->cseq, dp->port);
}

/*
 * allocate a queue
 */
static int
alloc_seq_queue(seq_oss_devinfo_t *dp)
{
	snd_seq_queue_info_t qinfo;
	int rc;

	memset(&qinfo, 0, sizeof(qinfo));
	qinfo.owner = system_client;
	qinfo.locked = 1;
	strcpy(qinfo.name, "OSS Sequencer Emulation");
	if ((rc = call_ctl(SNDRV_SEQ_IOCTL_CREATE_QUEUE, &qinfo)) < 0)
		return rc;
	dp->queue = qinfo.queue;
	return 0;
}

/*
 * release queue
 */
static int
delete_seq_queue(int queue)
{
	snd_seq_queue_info_t qinfo;
	int rc;

	if (queue < 0)
		return 0;
	memset(&qinfo, 0, sizeof(qinfo));
	qinfo.queue = queue;
	rc = call_ctl(SNDRV_SEQ_IOCTL_DELETE_QUEUE, &qinfo);
	if (rc < 0)
		printk(KERN_ERR "seq-oss: unable to delete queue %d (%d)\n", queue, rc);
	return rc;
}


/*
 * free device informations - private_free callback of port
 */
static void
free_devinfo(void *private)
{
	seq_oss_devinfo_t *dp = (seq_oss_devinfo_t *)private;

	if (dp->timer)
		snd_seq_oss_timer_delete(dp->timer);
		
	if (dp->writeq)
		snd_seq_oss_writeq_delete(dp->writeq);

	if (dp->readq)
		snd_seq_oss_readq_delete(dp->readq);
	
	kfree(dp);
}


/*
 * close sequencer device
 */
void
snd_seq_oss_release(seq_oss_devinfo_t *dp)
{
	int queue;

	client_table[dp->index] = NULL;
	num_clients--;

	debug_printk(("resetting..\n"));
	snd_seq_oss_reset(dp);

	debug_printk(("cleaning up..\n"));
	snd_seq_oss_synth_cleanup(dp);
	snd_seq_oss_midi_cleanup(dp);

	/* clear slot */
	debug_printk(("releasing resource..\n"));
	queue = dp->queue;
	if (dp->port >= 0)
		delete_port(dp);
	delete_seq_queue(queue);

	debug_printk(("release done\n"));
}


/*
 * Wait until the queue is empty (if we don't have nonblock)
 */
void
snd_seq_oss_drain_write(seq_oss_devinfo_t *dp)
{
	if (! dp->timer->running)
		return;
	if (is_write_mode(dp->file_mode) && !is_nonblock_mode(dp->file_mode) &&
	    dp->writeq) {
		debug_printk(("syncing..\n"));
		while (snd_seq_oss_writeq_sync(dp->writeq))
			;
	}
}


/*
 * reset sequencer devices
 */
void
snd_seq_oss_reset(seq_oss_devinfo_t *dp)
{
	int i;

	/* reset all synth devices */
	for (i = 0; i < dp->max_synthdev; i++)
		snd_seq_oss_synth_reset(dp, i);

	/* reset all midi devices */
	if (dp->seq_mode != SNDRV_SEQ_OSS_MODE_MUSIC) {
		for (i = 0; i < dp->max_mididev; i++)
			snd_seq_oss_midi_reset(dp, i);
	}

	/* remove queues */
	if (dp->readq)
		snd_seq_oss_readq_clear(dp->readq);
	if (dp->writeq)
		snd_seq_oss_writeq_clear(dp->writeq);

	/* reset timer */
	snd_seq_oss_timer_stop(dp->timer);
}


/*
 * misc. functions for proc interface
 */
char *
enabled_str(int bool)
{
	return bool ? "enabled" : "disabled";
}

static char *
filemode_str(int val)
{
	static char *str[] = {
		"none", "read", "write", "read/write",
	};
	return str[val & SNDRV_SEQ_OSS_FILE_ACMODE];
}


/*
 * proc interface
 */
void
snd_seq_oss_system_info_read(snd_info_buffer_t *buf)
{
	int i;
	seq_oss_devinfo_t *dp;

	snd_iprintf(buf, "ALSA client number %d\n", system_client);
	snd_iprintf(buf, "ALSA receiver port %d\n", system_port);

	snd_iprintf(buf, "\nNumber of applications: %d\n", num_clients);
	for (i = 0; i < num_clients; i++) {
		snd_iprintf(buf, "\nApplication %d: ", i);
		if ((dp = client_table[i]) == NULL) {
			snd_iprintf(buf, "*empty*\n");
			continue;
		}
		snd_iprintf(buf, "port %d : queue %d\n", dp->port, dp->queue);
		snd_iprintf(buf, "  sequencer mode = %s : file open mode = %s\n",
			    (dp->seq_mode ? "music" : "synth"),
			    filemode_str(dp->file_mode));
		if (dp->seq_mode)
			snd_iprintf(buf, "  timer tempo = %d, timebase = %d\n",
				    dp->timer->oss_tempo, dp->timer->oss_timebase);
		snd_iprintf(buf, "  max queue length %d\n", maxqlen);
		if (is_read_mode(dp->file_mode) && dp->readq)
			snd_seq_oss_readq_info_read(dp->readq, buf);
	}
}

