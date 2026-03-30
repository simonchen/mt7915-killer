[English](README.md)   [中文](README_zh-cn.md)

# mt7915-lite
Unlike generic drivers, **mt7915-lite** is an ultra-stability, high-performance driver branch for MediaTek MT7915 (Wi-Fi 6) Based on [openwrt/mt76](https://github.com/openwrt/mt76) (Dec 2022) with patches up to March 2026 (excl. WED). This project moves beyond simple patching to implement deep driver-level logic optimization and kernel-level task offloading.

At the driver level, high-frequency, redundant polling operations for statistical data were causing hardware sampling distortions on the MT7915; furthermore, the WTBL (Wireless Table) mechanism would subsequently amplify this issue, leading to reduced speeds for specific MAC addresses. Conversely, the true performance bottleneck in a Wi-Fi 6 environment—particularly on platforms utilizing traditional CPUs like the MT7621—is not the raw data throughput itself, but rather the "interrupt and sampling overhead." Through streamlined optimizations applied to both the driver level and the kernel architecture, we have successfully achieved industrial-grade system stability.

## Key Driver Optimizations 
  - **Stats_2 Lightweight Sampling**: The most important fix - Drastically reduced Wi-Fi statistics polling frequency, eliminating the "aging" effect where throughput drops after 12+ hours.
  - Disable AMSDU - Offloading logic back to hardware
    Disabled software-level A-MSDU aggregation to reduce CPU overhead, combined with periodic asynchronous resetting of WTBL hardware counters.
  - Mitigating firmware-side heuristic penalties
    (ADM_COUNT_CLEAR) to eliminate firmware-side "rate-limiting bias," ensuring long-term throughput stability under 340Mbps sustained stress.
  - MSDU density (Being hardware now): changed from 4μs to 5us, forcing the hardware to aggregate more packets per interrupt, achieving a 99.92% MSDU aggregation rate.
  - Heartbeat Scaling (HZ=1): Synchronized the driver heartbeat to 1 second, freeing CPU3 from high-frequency timer storms.
  - Memory Optimization: Downgraded DMA allocation to Order-0 (4KB) to prevent latency and allocation failures caused by memory fragmentation on    MIPS architectures.
  - WIFI5 Tuning: Optimized MSDU aggregation (Max 3 packets) for WIFI5 NICs (e.g., Killer-1535).
  - NAPI Throttle (Presudo Hard-IRQ delay): Implemented ```mcu_poll_cnt``` intercept in ```mt7915_poll_tx``` to dealy Hard-IRQ.
  - Real Hard-IRQ delay: DebugFS tweaking supported.
  - Descriptor Slimming: Reduced RX queues (8->5) and downsized/optimized the size of RX/TX_RING, MCU, and WTBL descriptors.
  - Thread Affinity: Hard-coded mt76-tx workqueue to CPU2 to avoid IPI drift.

## Recommended Architecture (CPU Affinity)
  - CPU0/1/3: Bind NAPI POLL workqueues and user-space apps.
  - CPU2: Bind Hard IRQs (mt7915e, mt7915e-hif) and mt76-tx workqueues (enforced by the driver).
  - All Ifaces should set rx-0/rps_cpus = b. (CPU0/1/3 same as NAPI-POLL binds)

## Stress Test Summary
  - Tested on MT7621 @1000MHz(Overclocking) with Killer-1535 NIC (PC runs on ```iperf3 -R -w 1M -P 1```)
  - Uptime Milestone: Successfully completed a 26.5-hour continuous stress test without a single drop.
  - Throughput Stability: Maintained a consistent 320-340Mbps "straight-line" performance.
  
**Note:**
  During stress testing, all `napi-workq` processes—as well as the core MT7915 driver worker process, `mt76-tx`—operated at the kernel's default priority.
