[English](README.md)   [中文](README_zh-cn.md)

# mt7915-killer
MT7915-Killer is a high-performance driver branch for MediaTek MT7915 (Wi-Fi 6). Based on [openwrt/mt76](https://github.com/openwrt/mt76) (Dec 2022) with patches up to March 2026 (excl. WED), it is specifically tuned to extract maximum performance from weak CPUs like MT7621 and eliminate long-term aging/stuttering issues.

## Perfermance (AP+Client mode)
- The base of average DL rate after continuously running ```iperf3 -R -w 1M -P1``` for 17h
<img width="472" height="277" alt="Perf-250M-after-17hr" src="iperf3-r-after-17hr.png" />

- Reovery after a heavy meory reclaim period
  Resilience Under Pressure (17h+ Runtime): Notice the recovery after a heavy memory reclaim period (V-dip). The driver seamlessly transitions from kernel-throttled state back to full speed (320Mbps), proving the reliability of the NAPI-intercept mechanism on legacy MIPS silicon.

  <img width="384" height="199" alt="image" src="https://github.com/user-attachments/assets/de280c8f-91b2-4a06-a203-6b91bb64a4ca" />
  <img width="384" height="190" alt="image" src="https://github.com/user-attachments/assets/fdbf2f3a-597b-4a0e-a2b4-1bb048389a05" />


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
- All Ifaces should set rx-0/rps_cpus = 0. (NAPI-POLL mode)

## Stress Test Summary
- Tested on MT7621 @1000MHz(Overclocking) with Killer-1535 NIC (PC runs on ```iperf3 -R -w 1M -P 1```)
- Stability: Continuous 12h+ uptime at 250-300Mbps.
- Resilience: Instant recovery from memory reclaim or Bad page state without hardware watchdog triggers.
- Metrics: Zero Dirty memory, optimized NET_RX/HRTIMER distribution.
- iperf3 final report:
  ```
  [ ID] Interval           Transfer     Bandwidth       Retr
  [  5]   0.00-72000.00 sec  0.00 %v絪   282 Mbits/sec  112921             sender
  [  5]   0.00-72000.00 sec  0.00 %v絪   282 Mbits/sec                  receiver
  ```
  
**Note:**
  During stress testing, all `napi-workq` processes—as well as the core MT7915 driver worker process, `mt76-tx`—operated at the kernel's default priority.
