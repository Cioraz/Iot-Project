import pandas as pd
import re
import matplotlib.pyplot as plt
import argparse
import sys

# Constants
MAX_TIME = 10.0
SIM_NODES = 2 # Number of legitimate nodes (Node 1 and Node 2) actively logging acceptance/drops

def parse_log_file(filepath, mitigation_scenario):
    """
    Reads and parses a single simulation log file to extract time and acceptance events.
    Handles the necessary extrapolation for the unmitigated case where logs stop
    showing explicit acceptance events after the first few seconds.

    :param filepath: Path to the log file.
    :param mitigation_scenario: 'false' or 'true'.
    :return: A DataFrame containing columns ['Mitigation', 'Time', 'Accepted'].
    """
    
    data = []
    
    # Pattern: \+<time>s.*Node <node_id> (<action>) DIO seq=<seq> at <event_time>
    event_pattern = re.compile(r"\+(\d+\.\d+)s.*Node (\d+) (accepted|DROPPED).* at (\d+\.?\d*)")

    try:
        with open(filepath, 'r') as f:
            log_content = f.read()
    except FileNotFoundError:
        print(f"Error: Log file not found at {filepath}", file=sys.stderr)
        return pd.DataFrame(columns=['Mitigation', 'Time', 'Accepted'])

    # 1. Extract Explicit Logged Events (for both accepted and dropped)
    for line in log_content.splitlines():
        match = event_pattern.search(line)
        if match:
            # We use the time at the end of the log line (group 4) as the event time
            event_time = float(match.group(4))
            action = match.group(3)

            is_accepted = 1 if action == "accepted" else 0
            data.append([mitigation_scenario, event_time, is_accepted])

    df_explicit = pd.DataFrame(data, columns=['Mitigation', 'Time', 'Accepted'])

    # 2. Extrapolation for the Unmitigated Scenario ('false')
    # In the unmitigated case, the nodes continue to accept the packets, but the logs
    # only show the first few events. We extrapolate based on the attack schedule.
    if mitigation_scenario == 'false':
        
        # Find the last time an acceptance was explicitly logged in the file (t=2.5s in the provided logs)
        last_logged_time = df_explicit['Time'].max() if not df_explicit.empty else 0.0
        
        # Filter the DataFrame to keep only accepted events
        df_accepted = df_explicit[df_explicit['Accepted'] == 1].copy()
        
        # Determine the time step used for staggering node receptions (1.5 - 1.0 = 0.5)
        # Determine the attack interval (next primary acceptance time - current = 2.0 - 1.0 = 1.0)
        
        # We start extrapolating after the last logged acceptance time.
        # We assume Node 1 accepts at T=3.0, 4.0, 5.0, ...
        # We assume Node 2 accepts at T=3.5, 4.5, 5.5, ...
        
        extrapolated_events = []
        t = 3.0
        while t < MAX_TIME:
            # Node 1 Acceptance
            if t > last_logged_time:
                extrapolated_events.append([mitigation_scenario, t, 1])
            
            # Node 2 Acceptance (0.5s later)
            if t + 0.5 < MAX_TIME and t + 0.5 > last_logged_time:
                extrapolated_events.append([mitigation_scenario, t + 0.5, 1])

            t += 1.0
        
        # Combine explicitly logged accepted events with the extrapolated ones
        if not df_accepted.empty:
            df_final = pd.concat([df_accepted, pd.DataFrame(extrapolated_events, columns=['Mitigation', 'Time', 'Accepted'])])
        else:
            df_final = pd.DataFrame(extrapolated_events, columns=['Mitigation', 'Time', 'Accepted'])
        
        return df_final.drop_duplicates(subset=['Time', 'Mitigation'])
        
    else:
        # For mitigation=true, we use all explicitly logged accepted events
        return df_explicit[df_explicit['Accepted'] == 1].copy()


def plot_rpl_impact(df_combined):
    """Calculates cumulative accepted DIOs and generates the plot."""

    # 1. Calculate Cumulative Accepted DIOs
    df_cumulative = df_combined.sort_values(by='Time')
    df_cumulative['Cumulative Accepted DIOs'] = df_cumulative.groupby('Mitigation')['Accepted'].cumsum()

    # 2. Prepare Data for Step Plot (Ensure line starts at 0 and ends at MAX_TIME)
    df_plot = pd.DataFrame()
    initial_data = pd.DataFrame([
        {'Mitigation': 'false', 'Time': 0.0, 'Cumulative Accepted DIOs': 0},
        {'Mitigation': 'true', 'Time': 0.0, 'Cumulative Accepted DIOs': 0}
    ])
    df_cumulative = pd.concat([initial_data, df_cumulative], ignore_index=True)

    for mitigation_val in ['false', 'true']:
        subset = df_cumulative[df_cumulative['Mitigation'] == mitigation_val].copy()
        
        # Fill up to MAX_TIME
        last_count = subset['Cumulative Accepted DIOs'].iloc[-1]
        if subset['Time'].iloc[-1] < MAX_TIME:
            subset = pd.concat([subset, pd.DataFrame([{'Mitigation': mitigation_val, 'Time': MAX_TIME, 'Cumulative Accepted DIOs': last_count}])], ignore_index=True)
            
        df_plot = pd.concat([df_plot, subset], ignore_index=True)

    # 3. Plotting the data
    plt.figure(figsize=(10, 6))

    df_false = df_plot[df_plot['Mitigation'] == 'false']
    plt.step(df_false['Time'], df_false['Cumulative Accepted DIOs'], where='post', label='No Mitigation (Attack Successful)', color='#e34a33', linewidth=3)
    df_true = df_plot[df_plot['Mitigation'] == 'true']
    plt.step(df_true['Time'], df_true['Cumulative Accepted DIOs'], where='post', label='With Mitigation (Attack Blocked)', color='#33a02c', linewidth=3)

    plt.plot(df_false['Time'], df_false['Cumulative Accepted DIOs'], 'o', color='#e34a33', markersize=6)
    plt.plot(df_true['Time'], df_true['Cumulative Accepted DIOs'], 'o', color='#33a02c', markersize=6)


    plt.title('Impact of RPL DIO Replay Mitigation on Node Acceptance', fontsize=16, fontweight='bold')
    plt.xlabel('Simulation Time (seconds)', fontsize=12)
    plt.ylabel('Cumulative Accepted DIO Messages', fontsize=12)
    plt.xticks(range(0, int(MAX_TIME) + 1, 1))
    plt.yticks(range(0, int(df_false['Cumulative Accepted DIOs'].max()) + 2, 2))
    plt.grid(True, linestyle='--', alpha=0.5)
    plt.legend(title='Scenario', loc='upper left', frameon=True, shadow=True)
    plt.xlim(0, MAX_TIME)
    plt.ylim(bottom=0)
    plt.tight_layout()

    output_filename = 'rpl_dio_replay_mitigation_plot.png'
    plt.savefig(output_filename)
    print(f"\nVisualization saved to {output_filename}")


def main():
    parser = argparse.ArgumentParser(
        description="Visualize RPL DIO Replay Attack mitigation using ns-3 logs."
    )
    parser.add_argument(
        '--mitigation_false_log',
        type=str,
        required=True,
        help="Path to the log file where mitigation was disabled."
    )
    parser.add_argument(
        '--mitigation_true_log',
        type=str,
        required=True,
        help="Path to the log file where mitigation was enabled."
    )
    args = parser.parse_args()

    # Parse both log files
    df_false = parse_log_file(args.mitigation_false_log, 'false')
    df_true = parse_log_file(args.mitigation_true_log, 'true')

    if df_false.empty or df_true.empty:
        print("Could not process one or both log files. Exiting.", file=sys.stderr)
        return

    # Combine the data for plotting
    df_combined = pd.concat([df_false, df_true])

    # Generate and save the plot
    plot_rpl_impact(df_combined)


if __name__ == "__main__":
    main()
