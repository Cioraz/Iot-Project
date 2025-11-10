Design and Evaluation of Mitigation Techniques for DIO Replay Attacks in Static RPL Networks

The objective of this project was to implement and evaluate a lightweight mitigation strategy against DIO (Destination-Oriented Directed Acyclic Graph Information Object) Replay Attacks within static RPL (Routing Protocol for Low-Power and Lossy Networks) using the NS-3 simulator.

1. Mitigation Strategy: Sequence-Number Filtering

The implemented defense relies on Sequence-Number Filtering, a lightweight mechanism suitable for resource-constrained IoT devices as it avoids cryptographic overhead.

Core Logic (Implemented in DioReceiverApp)

Each legitimate node checks the incoming DIO's sequence number (DIO.seq) against the last number it successfully processed from that attacker's source address.

Acceptance: If the received DIO.seq is strictly greater, the packet is considered new and is accepted.

Replay Detection: If the received DIO.seq is equal to or less than the last recorded number, the packet is flagged as a replay and dropped.

2. Simulation Results and Log Analysis

The simulation compared network behavior with the mitigation disabled (baseline.txt) versus the mitigation enabled (mitigated.txt). The attacker sends a replayed DIO with seq=1 every 1.0 second.

2.1 Baseline Scenario (--mitigation=false)

Result: All replayed DIOs are accepted. Nodes 1 and 2 process duplicate packets repeatedly, draining resources.

----- Baseline Log (baseline.txt) -----
 1 | +0.100s 0 RplDioReplaySim:Replay(): [INFO ] Replay attacker sending fake DIO at 0.1s
 2 | +1.000s -1 RplDioReplaySim:ReceiveFakeDio(): [INFO ] Node 1 accepted DIO seq=1 at 1
 3 | +1.100s 0 RplDioReplaySim:Replay(): [INFO ] Replay attacker sending fake DIO at 1.1s
 4 | +1.500s -1 RplDioReplaySim:ReceiveFakeDio(): [INFO ] Node 2 accepted DIO seq=1 at 1.5
 5 | +2.000s -1 RplDioReplaySim:ReceiveFakeDio(): [INFO ] Node 1 accepted DIO seq=1 at 2   <-- Replay accepted
 6 | +2.100s 0 RplDioReplaySim:Replay(): [INFO ] Replay attacker sending fake DIO at 2.1s
 7 | +2.500s -1 RplDioReplaySim:ReceiveFakeDio(): [INFO ] Node 2 accepted DIO seq=1 at 2.5  <-- Replay accepted
... continues accepting until 9.1s


2.2 Protected Scenario (--mitigation=true)

Result: The replayed packets are immediately dropped after the initial acceptance, neutralizing the attack.

----- Protected Log (mitigated.txt) -----
 1 | +0.100s 0 RplDioReplaySim:Replay(): [INFO ] Replay attacker sending fake DIO at 0.1s
 2 | +1.000s -1 RplDioReplaySim:ReceiveFakeDio(): [INFO ] Node 1 accepted DIO seq=1 at 1
 3 | +1.100s 0 RplDioReplaySim:Replay(): [INFO ] Replay attacker sending fake DIO at 1.1s
 4 | +1.500s -1 RplDioReplaySim:ReceiveFakeDio(): [INFO ] Node 2 accepted DIO seq=1 at 1.5
 5 | +2.000s -1 RplDioReplaySim:ReceiveFakeDio(): [INFO ] Node 1 DROPPED replayed DIO seq=1 at 2  <-- DROPPED!
 6 | +2.100s 0 RplDioReplaySim:Replay(): [INFO ] Replay attacker sending fake DIO at 2.1s
 7 | +2.500s -1 RplDioReplaySim:ReceiveFakeDio(): [INFO ] Node 2 DROPPED replayed DIO seq=1 at 2.5 <-- DROPPED!
... continues dropping until 9.1s


3. Visualization: Impact on Node Acceptance

The graph below plots the Cumulative Accepted DIO Messages over time, confirming the effectiveness of the mitigation strategy based on the data above.

Scenario

Final Accepted Count

Analysis

No Mitigation (Red Line)

~16

The count rises linearly, showing continuous susceptibility to the attack.

With Mitigation (Green Line)

2

The count plateaus immediately after the initial, necessary setup packets are processed. The attack is fully blocked.

4. Visualization Script

Use the following Python script to generate the rpl_dio_replay_mitigation_plot.png image from your log files:

python rpl_dynamic_visualization.py --mitigation_false_log baseline.txt --mitigation_true_log mitigated.txt


This report was generated from NS-3 logs and project documentation.
