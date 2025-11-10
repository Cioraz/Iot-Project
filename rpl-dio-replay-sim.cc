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

NS_LOG_COMPONENT_DEFINE("RplDioReplaySim");

// -----------------------------
// ReplayAttackerApp
// -----------------------------
class ReplayAttackerApp : public Application {
public:
    ReplayAttackerApp() {}
    virtual ~ReplayAttackerApp() { Simulator::Cancel(m_event); }

    void Setup(Ptr<Node> node, double intervalSeconds, bool repeat) {
        m_node = node;
        m_interval = Seconds(intervalSeconds); // ns3::Time
        m_repeat = repeat;
    }

    void StartApplication() override {
        Replay();
    }

private:
    void Replay() {
        NS_LOG_INFO("Replay attacker sending fake DIO at " << Simulator::Now().GetSeconds() << "s");
        // In real simulation: we would send fake DIO here

        if (m_repeat) {
            m_event = Simulator::Schedule(m_interval, &ReplayAttackerApp::Replay, this);
        }
    }

    Ptr<Node> m_node;
    ns3::Time m_interval;
    bool m_repeat;
    EventId m_event;
};

// -----------------------------
// DioReceiverApp
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
        if (m_mitigation) {
            // Drop replayed sequence numbers
            if (m_seenSeq.find(seq) != m_seenSeq.end()) {
                NS_LOG_INFO("Node " << m_node->GetId() << " DROPPED replayed DIO seq=" << seq << " at " << now);
                return;
            }
        }
        m_seenSeq[seq] = true;
        NS_LOG_INFO("Node " << m_node->GetId() << " accepted DIO seq=" << seq << " at " << now);
    }

private:
    Ptr<Node> m_node;
    bool m_mitigation;
    std::map<uint32_t, bool> m_seenSeq;
};

// -----------------------------
// Main
// -----------------------------
int main(int argc, char *argv[]) {
    double simTime = 10.0;
    bool enableMitigation = false;

    CommandLine cmd;
    cmd.AddValue("mitigation", "Enable DIO replay mitigation", enableMitigation);
    cmd.AddValue("simTime", "Simulation time in seconds", simTime);
    cmd.Parse(argc, argv);

    // Create nodes
    NodeContainer nodes;
    nodes.Create(3);

    // Install internet stack
    InternetStackHelper internet;
    internet.Install(nodes);

    // Create applications
    Ptr<ReplayAttackerApp> attacker = CreateObject<ReplayAttackerApp>();
    attacker->Setup(nodes.Get(0), 1.0, true);
    nodes.Get(0)->AddApplication(attacker);
    attacker->SetStartTime(Seconds(0.1));

    Ptr<DioReceiverApp> receiver1 = CreateObject<DioReceiverApp>();
    receiver1->Setup(nodes.Get(1), enableMitigation);
    nodes.Get(1)->AddApplication(receiver1);
    receiver1->SetStartTime(Seconds(0.1));

    Ptr<DioReceiverApp> receiver2 = CreateObject<DioReceiverApp>();
    receiver2->Setup(nodes.Get(2), enableMitigation);
    nodes.Get(2)->AddApplication(receiver2);
    receiver2->SetStartTime(Seconds(0.1));

    // Simulate DIOs being sent by attacker every 1s
    Simulator::Schedule(Seconds(1.0), &DioReceiverApp::ReceiveFakeDio, receiver1, 1);
    Simulator::Schedule(Seconds(2.0), &DioReceiverApp::ReceiveFakeDio, receiver1, 1); // replay
    Simulator::Schedule(Seconds(1.5), &DioReceiverApp::ReceiveFakeDio, receiver2, 1);
    Simulator::Schedule(Seconds(2.5), &DioReceiverApp::ReceiveFakeDio, receiver2, 1); // replay

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
