// SPDX-License-Identifier: ISC
/*
 * Copyright (C) 2016 Felix Fietkau <nbd@nbd.name>
 */
#include "mt76.h"

static int
mt76_reg_set(void *data, u64 val)
{
	struct mt76_dev *dev = data;

	__mt76_wr(dev, dev->debugfs_reg, val);
	return 0;
}

static int
mt76_reg_get(void *data, u64 *val)
{
	struct mt76_dev *dev = data;

	*val = __mt76_rr(dev, dev->debugfs_reg);
	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(fops_regval, mt76_reg_get, mt76_reg_set,
			 "0x%08llx\n");

#if 0
static int
mt76_napi_threaded_set(void *data, u64 val)
{
	struct mt76_dev *dev = data;

	if (!mt76_is_mmio(dev))
		return -EOPNOTSUPP;

	if (dev->napi_dev.threaded != val)
		return dev_set_threaded(&dev->napi_dev, val);

	return 0;
}

static int
mt76_napi_threaded_get(void *data, u64 *val)
{
	struct mt76_dev *dev = data;

	*val = dev->napi_dev.threaded;
	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(fops_napi_threaded, mt76_napi_threaded_get,
			 mt76_napi_threaded_set, "%llu\n");
#endif

int mt76_queues_read(struct seq_file *s, void *data)
{
	struct mt76_dev *dev = dev_get_drvdata(s->private);
	int i;

	seq_puts(s, "     queue | hw-queued |      head |      tail |\n");
	for (i = 0; i < ARRAY_SIZE(dev->phy.q_tx); i++) {
		struct mt76_queue *q = dev->phy.q_tx[i];

		if (!q)
			continue;

		seq_printf(s, " %9d | %9d | %9d | %9d |\n",
			   i, q->queued, q->head, q->tail);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(mt76_queues_read);

static int mt76_rx_queues_read(struct seq_file *s, void *data)
{
	struct mt76_dev *dev = dev_get_drvdata(s->private);
	int i, queued;

	seq_puts(s, "     queue | hw-queued |      head |      tail |\n");
	mt76_for_each_q_rx(dev, i) {
		struct mt76_queue *q = &dev->q_rx[i];

		queued = mt76_is_usb(dev) ? q->ndesc - q->queued : q->queued;
		seq_printf(s, " %9d | %9d | %9d | %9d |\n",
			   i, queued, q->head, q->tail);
	}

	return 0;
}

static int mt76_rx_nframes_read(struct seq_file *s, void *data)
{
        struct mt76_dev *dev = dev_get_drvdata(s->private);

	seq_printf(s, "nframes: %d\n", dev->aggr_nframes);

	return 0;
}

static int
mt76_rx_nframes_limit_set(void *data, u64 val)
{
        struct mt76_dev *dev = data;

        dev->max_aggr_nframes = (u32)val;

        return 0;
}

static int
mt76_rx_nframes_limit_get(void *data, u64 *val)
{
        struct mt76_dev *dev = data;

        *val = (u64)dev->max_aggr_nframes;

        return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(fops_rx_nframes_limit, mt76_rx_nframes_limit_get, mt76_rx_nframes_limit_set,
                         "%lld\n");

static int
mt76_rx_poll_retry_cnt_read(struct seq_file *s, void *data)
{
	//struct mt76_dev *dev = dev_get_drvdata(s->private);

        seq_printf(s, "rx_poll_retry_cnt: %d\n", atomic_read(&rx_poll_retry_cnt));

        return 0;
}

static int
mt76_rx_poll_timeo_set(void *data, u64 val)
{
        //struct mt76_dev *dev = data;

        rx_poll_timeo = (int)val;

        return 0;
}

static int
mt76_rx_poll_timeo_get(void *data, u64 *val)
{
        //struct mt76_dev *dev = data;

        *val = (u64)rx_poll_timeo;

        return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(fops_rx_poll_timeo, mt76_rx_poll_timeo_get, mt76_rx_poll_timeo_set,
                         "%lld\n");


void mt76_seq_puts_array(struct seq_file *file, const char *str,
			 s8 *val, int len)
{
	int i;

	seq_printf(file, "%10s:", str);
	for (i = 0; i < len; i++)
		seq_printf(file, " %2d", val[i]);
	seq_puts(file, "\n");
}
EXPORT_SYMBOL_GPL(mt76_seq_puts_array);

struct dentry *
mt76_register_debugfs_fops(struct mt76_phy *phy,
			   const struct file_operations *ops)
{
	const struct file_operations *fops = ops ? ops : &fops_regval;
	struct mt76_dev *dev = phy->dev;
	struct dentry *dir;

	dir = debugfs_create_dir("mt76", phy->hw->wiphy->debugfsdir);
	if (!dir)
		return NULL;

	debugfs_create_u8("led_pin", 0600, dir, &phy->leds.pin);
	debugfs_create_bool("led_active_low", 0600, dir, &phy->leds.al);
	debugfs_create_u32("regidx", 0600, dir, &dev->debugfs_reg);
	debugfs_create_file_unsafe("regval", 0600, dir, dev, fops);
	debugfs_create_blob("eeprom", 0400, dir, &dev->eeprom);
	if (dev->otp.data)
		debugfs_create_blob("otp", 0400, dir, &dev->otp);
	debugfs_create_devm_seqfile(dev->dev, "rx-queues", dir,
				    mt76_rx_queues_read);
	debugfs_create_devm_seqfile(dev->dev, "rx-aggr-nframes", dir, mt76_rx_nframes_read);
	debugfs_create_file_unsafe("rx-aggr-nframes-limit", 0600, dir, dev, &fops_rx_nframes_limit);

	debugfs_create_devm_seqfile(dev->dev, "rx-poll-retry-cnt", dir, mt76_rx_poll_retry_cnt_read);
	debugfs_create_file_unsafe("rx-poll-timeo", 0600, dir, dev, &fops_rx_poll_timeo);

	debugfs_create_bool("rx-poll-retry-active", 0600, dir, &rx_poll_retry_enable);

	return dir;
}
EXPORT_SYMBOL_GPL(mt76_register_debugfs_fops);
