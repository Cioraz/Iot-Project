# 🛡️ Evaluation of Mitigation for DIO Replay Attacks in NS-3 RPL Networks

This project implements and evaluates a **lightweight mitigation strategy** against **DIO (Destination-Oriented Directed Acyclic Graph Information Object) Replay Attacks** within static RPL (Routing Protocol for Low-Power and Lossy Networks) networks using the NS-3 simulator.

---

## 1. Attack and Mitigation Overview

### 1.1 The Threat: DIO Replay

An adversary captures a legitimate DIO control message (containing a static sequence number, `seq=1` in this simulation) and repeatedly re-broadcasts it. This forces legitimate nodes to process outdated routing information, leading to network instability and excessive resource consumption.

### 1.2 Mitigation Strategy: Sequence-Number Filtering

[cite_start]The defense uses **Sequence-Number Filtering**, a low-overhead technique suitable for resource-constrained IoT devices[cite: 72].

* [cite_start]**Logic:** Each node tracks the last valid sequence number received from a neighbor[cite: 51]. [cite_start]Any subsequent packet from that neighbor carrying a sequence number that is **not strictly greater** (i.e., equal or less) is dropped as a replay[cite: 54].

---

## 2. Simulation Results and Log Analysis

The simulation ran for 10 seconds, comparing the baseline scenario (`--mitigation=false`) against the protected scenario (`--mitigation=true`).

### 2.1 Baseline Scenario (`baseline.txt`)

[cite_start]**Observation:** All replayed DIOs are accepted[cite: 99]. The nodes are continuously exposed to the attack traffic.

| Time (s) | Node | Log Output | Outcome |
| :---: | :---: | :--- | :--- |
| **1.0** | Node 1 | `...Node 1 accepted DIO seq=1 at 1` | Accepted |
| **1.5** | Node 2 | `...Node 2 accepted DIO seq=1 at 1.5` | Accepted |
| **2.0** | Node 1 | `...Node 1 accepted DIO seq=1 at 2` | **Replay Accepted** |
| **2.5** | Node 2 | `...Node 2 accepted DIO seq=1 at 2.5` | **Replay Accepted** |
| 3.1 - 9.1 | Attacker | `Replay attacker sending fake DIO...` | Nodes continue to accept. |

### 2.2 Protected Scenario (`mitigated.txt`)

[cite_start]**Observation:** Replayed DIOs are immediately dropped after the initial acceptance[cite: 100], neutralizing the attack.

| Time (s) | Node | Log Output | Outcome |
| :---: | :---: | :--- | :--- |
| **1.0** | Node 1 | `...Node 1 accepted DIO seq=1 at 1` | Accepted (Initial) |
| **1.5** | Node 2 | `...Node 2 accepted DIO seq=1 at 1.5` | Accepted (Initial) |
| **2.0** | Node 1 | `...Node 1 DROPPED replayed DIO seq=1 at 2` | **DROPPED!** |
| **2.5** | Node 2 | `...Node 2 DROPPED replayed DIO seq=1 at 2.5` | **DROPPED!** |
| 3.1 - 9.1 | Attacker | `Replay attacker sending fake DIO...` | Nodes continue to drop. |

---

## 3. Visualization: Impact on Node Acceptance

The graph below plots the **Cumulative Accepted DIO Messages** over time, clearly demonstrating the performance difference.

*(Note: You must run `rpl_dynamic_visualization.py` on your machine to generate this image, which should be placed in the same directory as this file.)*

![Impact of RPL DIO Replay Mitigation on Node Acceptance](rpl_dio_replay_mitigation_plot.png)

| Scenario | Final Accepted Count | Analysis |
| :--- | :--- | :--- |
| **No Mitigation** (Red Line) | **~16** | The count rises linearly. The attack is fully successful, wasting node resources. |
| **With Mitigation** (Green Line) | **2** | The count **plateaus immediately** after the initial packets are processed. The attack is neutralized. |

---

## 4. Visualization Script and Execution

The script below is used to parse the log files (`baseline.txt`, `mitigated.txt`) and generate the comparison plot.

### Script Execution

```bash
# Ensure you are in your Python virtual environment (venv)
python rpl_dynamic_visualization.py --mitigation_false_log baseline.txt --mitigation_true_log mitigated.txt
