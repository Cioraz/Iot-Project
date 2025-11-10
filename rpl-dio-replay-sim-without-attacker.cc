#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv6-address.h"
#include "ns3/ipv6-routing-helper.h"
#include "ns3/ipv6-static-routing-helper.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/applications-module.h"
#include "ns3/udp-socket.h"
#include "ns3/udp-socket-factory.h"

#include <iostream>
#include <map>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("RplDioNoAttackSim");

// -----------------------------
// DioReceiverApp (Same as original, but mitigation is irrelevant without attack)
// -----------------------------
class DioReceiverApp : public Application {
public:
    DioReceiverApp() {}
    virtual ~DioReceiverApp() {}

    void Setup(Ptr<Node> node, bool mitigation) {
        m_node = node;
        m_mitigation = mitigation;
    }

    void ReceiveFakeDio(uint32_t seq) {
        double now = Simulator::Now().GetSeconds();

        // Mitigation check is included but will only drop if 'seq' is a duplicate.
        // In this 'No Attack' code, all scheduled 'seq' are unique, so nothing is dropped.
        if (m_mitigation) {
            // Drop replayed sequence numbers
            if (m_seenSeq.find(seq) != m_seenSeq.end()) {
                NS_LOG_INFO("Node " << m_node->GetId() << " DROPPED REPLAY DIO seq=" << seq << " at t=" << now);
                return;
            }
        }

        m_seenSeq[seq] = true;
        NS_LOG_INFO("Node " << m_node->GetId() << " accepted UNIQUE DIO seq=" << seq << " at t=" << now);
    }

private:
    Ptr<Node> m_node;
    bool m_mitigation;
    std::map<uint32_t, bool> m_seenSeq;
};

// -----------------------------
// Main (Attacker components removed)
// -----------------------------
int main(int argc, char *argv[]) {
    double simTime = 5.0; // Reduced time since fewer events are scheduled
    bool enableMitigation = false; // Mitigation flag remains, but is inactive in this scenario

    CommandLine cmd;
    cmd.AddValue("mitigation", "Enable DIO replay mitigation (inactive in this example)", enableMitigation);
    cmd.AddValue("simTime", "Simulation time in seconds", simTime);
    cmd.Parse(argc, argv);

    // Create nodes
    NodeContainer nodes;
    nodes.Create(3);

    // Install internet stack
    InternetStackHelper internet;
    internet.Install(nodes);

    // Create receiver applications
    // We can arbitrarily set mitigation to true or false; it won't drop unique messages.
    Ptr<DioReceiverApp> receiver1 = CreateObject<DioReceiverApp>();
    receiver1->Setup(nodes.Get(1), true); // Mitigation set to true for demo
    nodes.Get(1)->AddApplication(receiver1);
    receiver1->SetStartTime(Seconds(0.1));

    Ptr<DioReceiverApp> receiver2 = CreateObject<DioReceiverApp>();
    receiver2->Setup(nodes.Get(2), false); // Mitigation set to false for demo
    nodes.Get(2)->AddApplication(receiver2);
    receiver2->SetStartTime(Seconds(0.1));

    // --- Schedule ONLY UNIQUE DIOs (Simulating Normal Network Traffic) ---
    // The sequence number (seq) increases to reflect a new, legitimate control message.

    // Receiver 1 accepts its first DIO
    Simulator::Schedule(Seconds(1.0), &DioReceiverApp::ReceiveFakeDio, receiver1, 1);

    // Receiver 2 accepts its first DIO (same seq is fine, as it's a different receiver/DODAG structure)
    Simulator::Schedule(Seconds(1.5), &DioReceiverApp::ReceiveFakeDio, receiver2, 1);

    // Receiver 1 accepts a NEW, UPDATED DIO (new sequence number)
    Simulator::Schedule(Seconds(2.0), &DioReceiverApp::ReceiveFakeDio, receiver1, 2);

    // Receiver 2 accepts a NEW, UPDATED DIO (new sequence number)
    Simulator::Schedule(Seconds(2.5), &DioReceiverApp::ReceiveFakeDio, receiver2, 2);

    // Receiver 1 accepts yet another NEW DIO
    Simulator::Schedule(Seconds(3.0), &DioReceiverApp::ReceiveFakeDio, receiver1, 3);

    // Total unique accepted messages will be 5.
    // --------------------------------------------------------------------

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
