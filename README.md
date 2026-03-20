[English](README.md)   [中文](README_zh-cn.md)

# mt7915-killer
MT7915-Killer is a high-performance driver branch for MediaTek MT7915 (Wi-Fi 6). Based on [openwrt/mt76](https://github.com/openwrt/mt76) (Dec 2022) with patches up to March 2026 (excl. WED), it is specifically tuned to extract maximum performance from weak CPUs like MT7621 and eliminate long-term aging/stuttering issues.

## Key Driver Optimizations (Linux 5.4.268)
- Memory Optimization: Downgraded DMA allocation to Order-0 (4KB) to prevent latency and allocation failures caused by memory fragmentation on MIPS architectures.
- WIFI5 Tuning: Optimized MSDU aggregation (Max 3 packets) for WIFI5 NICs (e.g., Killer-1535).
- NAPI Throttle: Implemented ```mcu_poll_cnt``` intercept in ```mt7915_poll_tx``` to replace hardware interrupt storms with soft-polling, fixing spinlock deadlocks.
- Descriptor Slimming: Reduced RX queues (8->5) and downsized RX/TX_RING, MCU, and WTBL descriptors.
- Thread Affinity: Hard-coded mt76-tx workqueue to CPU2 to avoid IPI drift.

## Recommended Architecture (CPU Affinity)
- CPU2: Bind Hard IRQs (mt7915e, mt7915e-hif) and mt76-tx workqueues.
- CPU3: Offload HRTIMER tasks from CPU2 (sharing L1 Cache via VPE).
- CPU0/1: Bind NAPI POLL workqueues and user-space apps.

## Stress Test Summary
- Tested on MT7621 @880MHz with Killer-1535 NIC (PC runs on ```iperf3 -R -w 1M -P 1```)
- Stability: Continuous 12h+ uptime at 250-300Mbps.
- Resilience: Instant recovery from memory reclaim or Bad page state without hardware watchdog triggers.
- Metrics: Zero Dirty memory, optimized NET_RX/HRTIMER distribution.
