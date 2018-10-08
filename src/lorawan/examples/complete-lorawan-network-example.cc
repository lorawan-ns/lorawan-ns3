/*
 * This script simulates a complex scenario with multiple gateways and end
 * devices. The metric of interest for this script is the throughput of the
 * network.
 */

#include "ns3/end-device-lora-phy.h"
#include "ns3/gateway-lora-phy.h"
#include "ns3/end-device-lora-mac.h"
#include "ns3/gateway-lora-mac.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/lora-helper.h"
#include "ns3/node-container.h"
#include "ns3/mobility-helper.h"
#include "ns3/position-allocator.h"
#include "ns3/double.h"
#include "ns3/random-variable-stream.h"
#include "ns3/periodic-sender-helper.h"
#include "ns3/command-line.h"
#include <algorithm>
#include <ctime>
//--------------------------------------------------
#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/ipv4-address.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/okumura-hata-propagation-loss-model.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/mobility-module.h>
#include <ns3/simulator.h>
#include <iostream>
#include <iomanip>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ComplexLorawanNetworkExample");

// Network settings
int nDevices = 10;
int gatewayRings = 1;
//int nGateways = 3*gatewayRings*gatewayRings-3*gatewayRings+1;
int nGateways = 1;
double radius = 3000;
double gatewayRadius = 6000/((gatewayRings-1)*2+1);
double simulationTime = 600;
int appPeriodSeconds = 600;
std::vector<int> sfQuantity (6);
std::vector<double> distances(nDevices);
uint32_t randomSeed = 12345;
uint32_t nRuns = 1;
bool verbose = false;

int noMoreReceivers = 0;
int interfered = 0;
int received = 0;
int underSensitivity = 0;

// Output control
bool printEDs = true;
bool buildingsEnabled = true;

/**********************
 *  Global Callbacks  *
 **********************/

enum PacketOutcome {
  RECEIVED,
  INTERFERED,
  NO_MORE_RECEIVERS,
  UNDER_SENSITIVITY,
  UNSET
};

struct PacketStatus {
  Ptr<Packet const> packet;
  uint32_t senderId;
  int outcomeNumber;
  int recebido;
  std::vector<enum PacketOutcome> outcomes;
};

std::map<Ptr<Packet const>, PacketStatus> packetTracker;

void
CheckReceptionByAllGWsComplete (std::map<Ptr<Packet const>, PacketStatus>::iterator it)
{
//std::cout << (*it).second.outcomeNumber << "==" << nGateways <<"\n";
  // Check whether this packet is received by all gateways
  if ((*it).second.outcomeNumber == nGateways)
    {
      // Update the statistics
      PacketStatus status = (*it).second;
      for (int j = 0; j < nGateways; j++)
        {
		  //std::cout << status.outcomes.at (j) << "\n";
          switch (status.outcomes.at (j))
            {
            case RECEIVED:
              {
				if(j!=0){
					if ( status.outcomes.at (j) == status.outcomes.at (j-1))
						break;
				}
                received += 1;
                break;
              }
            case UNDER_SENSITIVITY:
              {
				if(j!=0){
					if ( status.outcomes.at (j) == status.outcomes.at (j-1))
						break;
				}
                underSensitivity += 1;
                break;
              }
            case NO_MORE_RECEIVERS:
              {
				if(j!=0){
					if ( status.outcomes.at (j) == status.outcomes.at (j-1))
						break;
				}
                noMoreReceivers += 1;
                break;
              }
            case INTERFERED:
              {
				if(j!=0){
					if ( status.outcomes.at (j) == status.outcomes.at (j-1))
						break;
				}
                interfered += 1;
                break;
              }
            case UNSET:
              {
                break;
              }
            }
        }
      // Remove the packet from the tracker
      packetTracker.erase (it);
    } else {
      // Update the statistics
	}
}

void
TransmissionCallback (Ptr<Packet const> packet, uint32_t systemId)
{
 if(verbose){
  NS_LOG_INFO ("Transmitted a packet from device " << systemId);
 }
  // Create a packetStatus
  PacketStatus status;
  status.packet = packet;
  status.senderId = systemId;
  status.outcomeNumber = 0;
  status.recebido = 0;
  status.outcomes = std::vector<enum PacketOutcome> (nGateways, UNSET);

  packetTracker.insert (std::pair<Ptr<Packet const>, PacketStatus> (packet, status));
}

void
PacketReceptionCallback (Ptr<Packet const> packet, uint32_t systemId)
{
 if(verbose){
  NS_LOG_INFO ("A packet was successfully received at gateway " << systemId );
 }
  std::map<Ptr<Packet const>, PacketStatus>::iterator it = packetTracker.find (packet);
  (*it).second.outcomes.at (systemId - nDevices) = RECEIVED;
  (*it).second.outcomeNumber += 1;
  (*it).second.recebido = 1;
  //std::cout << (*it).second.recebido << "\n"; //= true;
  std::cout << (*it).second.outcomeNumber << "\n";
  CheckReceptionByAllGWsComplete (it);
}

void
InterferenceCallback (Ptr<Packet const> packet, uint32_t systemId)
{
 if(verbose){
  NS_LOG_INFO ("A packet was lost because of interference at gateway " << systemId);
 }
  std::map<Ptr<Packet const>, PacketStatus>::iterator it = packetTracker.find (packet);
  (*it).second.outcomes.at (systemId - nDevices) = INTERFERED;
  (*it).second.outcomeNumber += 1;
  (*it).second.recebido = 0;
  std::cout << (*it).second.recebido << "\n"; //= true;

  CheckReceptionByAllGWsComplete (it);
}

void
NoMoreReceiversCallback (Ptr<Packet const> packet, uint32_t systemId)
{
 if(verbose){
   NS_LOG_INFO ("A packet was lost because there were no more receivers at gateway " << systemId);
 }
  std::map<Ptr<Packet const>, PacketStatus>::iterator it = packetTracker.find (packet);
  (*it).second.outcomes.at (systemId - nDevices) = NO_MORE_RECEIVERS;
  (*it).second.outcomeNumber += 1;
  (*it).second.recebido = 2;
  std::cout << (*it).second.recebido << "\n"; //= true;

  CheckReceptionByAllGWsComplete (it);
}

void
UnderSensitivityCallback (Ptr<Packet const> packet, uint32_t systemId)
{
 if(verbose){
   NS_LOG_INFO ("A packet arrived at the gateway under sensitivity at gateway " << systemId);
 }
  std::map<Ptr<Packet const>, PacketStatus>::iterator it = packetTracker.find (packet);
  (*it).second.outcomes.at (systemId - nDevices) = UNDER_SENSITIVITY;
  (*it).second.outcomeNumber += 1;
  (*it).second.recebido = 3;
  std::cout << (*it).second.recebido << "\n"; //= true;

  CheckReceptionByAllGWsComplete (it);
}

time_t oldtime = std::time (0);

// Periodically print simulation time
void PrintSimulationTime (void)
{
 if(verbose){
  NS_LOG_INFO ("Time: " << Simulator::Now().GetHours());
 }
  std::cout << "Simulated time: " << Simulator::Now ().GetHours () << " hours" << std::endl;
  std::cout << "Real time from last call: " << std::time (0) - oldtime << " seconds" << std::endl;
  oldtime = std::time (0);
  Simulator::Schedule (Minutes (30), &PrintSimulationTime);
}

void
 PrintarCoisas ( NodeContainer endDevices, NodeContainer gateways)
{
  //std::map<Ptr<Packet const>, PacketStatus>::iterator it = packetTracker.find (packet);
  for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j)
    {
  for (NodeContainer::Iterator z = gateways.Begin (); z != gateways.End (); ++z)
    {
      Ptr<Node> object_gw = *z;
      Ptr<MobilityModel> position_gw = object_gw->GetObject<MobilityModel> ();
      //Vector pos_gw = position_gw->GetPosition ();
   
      Ptr<Node> object = *j;
      Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
      NS_ASSERT (position != 0);
      Ptr<NetDevice> netDevice = object->GetDevice (0);
      Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      NS_ASSERT (loraNetDevice != 0);
      Ptr<EndDeviceLoraMac> mac = loraNetDevice->GetMac ()->GetObject<EndDeviceLoraMac> ();
	  std::cout << "Distance: "<< position->GetDistanceFrom (position_gw) << " \n" ;//<<(*it).second.recebido << "\n";
    }}
}

void
PrintEndDevices ( NodeContainer endDevices, NodeContainer gateways, std::string filename)
{
  const char * c = filename.c_str ();
  std::ofstream spreadingFactorFile;
  spreadingFactorFile.open (c);
  // Also print the gateways
//  for (NodeContainer::Iterator z = gateways.Begin (); z != gateways.End (); ++z)
//    {
//      Ptr<Node> object_gw = *z;
//      Ptr<MobilityModel> position_gw = object_gw->GetObject<MobilityModel> ();
//      Vector pos_gw = position_gw->GetPosition ();
//      std::cout << position_gw << " POS GW\n";
//      spreadingFactorFile << pos_gw.x << " " << pos_gw.y << " GW" << std::endl;
//    }
  for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j)
    {
  for (NodeContainer::Iterator z = gateways.Begin (); z != gateways.End (); ++z)
    {
      Ptr<Node> object_gw = *z;
      Ptr<MobilityModel> position_gw = object_gw->GetObject<MobilityModel> ();
      Vector pos_gw = position_gw->GetPosition ();
      //std::cout << position_gw << " POS GW\n";
      spreadingFactorFile << pos_gw.x << " " << pos_gw.y << " GW" << std::endl;
   
      Ptr<Node> object = *j;
      Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
      NS_ASSERT (position != 0);
      Ptr<NetDevice> netDevice = object->GetDevice (0);
      Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      NS_ASSERT (loraNetDevice != 0);
      Ptr<EndDeviceLoraMac> mac = loraNetDevice->GetMac ()->GetObject<EndDeviceLoraMac> ();
      int sf = int(mac->GetDataRate ());
      Vector pos = position->GetPosition ();
//	  std::cout << "Distance: "<< position->GetDistanceFrom (position_gw) << " ";
//      std::cout << 12-sf << std::endl;
      spreadingFactorFile << pos.x << " " << pos.y << " " << 12-sf << std::endl;
    }}
  spreadingFactorFile.close ();
}

int main (int argc, char *argv[])
{

  CommandLine cmd;
  cmd.AddValue ("nDevices", "Number of end devices to include in the simulation", nDevices);
  cmd.AddValue ("gatewayRings", "Number of gateway rings to include", gatewayRings);
  cmd.AddValue ("radius", "The radius of the area to simulate", radius);
  cmd.AddValue ("gatewayRadius", "The distance between two gateways", gatewayRadius);
  cmd.AddValue ("simulationTime", "The time for which to simulate", simulationTime);
  cmd.AddValue ("appPeriod", "The period in seconds to be used by periodically transmitting applications", appPeriodSeconds);
  cmd.AddValue ("printEDs", "Whether or not to print a file containing the ED's positions", printEDs);
  cmd.AddValue ("randomSeed", "Random seed used in experiments[Default:12345]", randomSeed);
  cmd.AddValue ("nRuns", "Number of simulation runs[Default:100]", nRuns);
  cmd.AddValue ("verbose", "Show verbose[Default:false]", verbose);

  cmd.Parse (argc, argv);

  if(verbose){
  // Set up logging
    LogComponentEnable ("ComplexLorawanNetworkExample", LOG_LEVEL_ALL);
    LogComponentEnable("LoraChannel", LOG_LEVEL_INFO);
//   LogComponentEnable("LoraPhy", LOG_LEVEL_ALL);
//    LogComponentEnable("EndDeviceLoraPhy", LOG_LEVEL_ALL);
    LogComponentEnable("GatewayLoraPhy", LOG_LEVEL_ALL);
 //   LogComponentEnable("LoraInterferenceHelper", LOG_LEVEL_ALL);
 //   LogComponentEnable("LoraMac", LOG_LEVEL_ALL);
 //   LogComponentEnable("EndDeviceLoraMac", LOG_LEVEL_ALL);
 //   LogComponentEnable("GatewayLoraMac", LOG_LEVEL_ALL);
 //   LogComponentEnable("LogicalLoraChannelHelper", LOG_LEVEL_ALL);
 //   LogComponentEnable("LogicalLoraChannel", LOG_LEVEL_ALL);
 //   LogComponentEnable("LoraHelper", LOG_LEVEL_ALL);
 //   LogComponentEnable("LoraPhyHelper", LOG_LEVEL_ALL);
 //   LogComponentEnable("LoraMacHelper", LOG_LEVEL_ALL);
 //   LogComponentEnable("PeriodicSenderHelper", LOG_LEVEL_ALL);
 //   LogComponentEnable("PeriodicSender", LOG_LEVEL_ALL);
 //   LogComponentEnable("LoraMacHeader", LOG_LEVEL_ALL);
 //   LogComponentEnable("LoraFrameHeader", LOG_LEVEL_ALL);
 //   LogComponentEnable("LoraNetDevice", LOG_LEVEL_ALL);
  }
  /***********
  *  Setup  *
  ***********/

  // Compute the number of gateways
  //nGateways = 3*gatewayRings*gatewayRings-3*gatewayRings+1;
  // Create the time value from the period
  Time appPeriod = Seconds (appPeriodSeconds);
  for (uint32_t i = 0; i < nRuns; i++) {
    uint32_t seed = randomSeed + i;
    SeedManager::SetSeed (seed);

  // Mobility
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator",
                                 "rho", DoubleValue (radius),
                                 "X", DoubleValue (0.0),
                                 "Y", DoubleValue (0.0));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  /************************
  *  Create the channel  *
  ************************/

  // Create the lora channel object
//  Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel> ();
//  loss->SetPathLossExponent (3.76);
//  loss->SetReference (1, 8.1);

  Ptr<OkumuraHataPropagationLossModel> loss = CreateObject<OkumuraHataPropagationLossModel> ();

  Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel> ();

  Ptr<LoraChannel> channel = CreateObject<LoraChannel> (loss, delay);

  /************************
  *  Create the helpers  *
  ************************/

  // Create the LoraPhyHelper
  LoraPhyHelper phyHelper = LoraPhyHelper ();
  phyHelper.SetChannel (channel);

  // Create the LoraMacHelper
  LoraMacHelper macHelper = LoraMacHelper ();

  // Create the LoraHelper
  LoraHelper helper = LoraHelper ();

  /************************
  *  Create End Devices  *
  ************************/

  // Create a set of nodes
  NodeContainer endDevices;
  endDevices.Create (nDevices);

  // Assign a mobility model to each node
  mobility.Install (endDevices);

  // Make it so that nodes are at a certain height > 0
  for (NodeContainer::Iterator j = endDevices.Begin ();
       j != endDevices.End (); ++j)
    {
      Ptr<MobilityModel> mobility = (*j)->GetObject<MobilityModel> ();
      Vector position = mobility->GetPosition ();
      position.z = 1.2;
      mobility->SetPosition (position);
    }

  // Create the LoraNetDevices of the end devices
  phyHelper.SetDeviceType (LoraPhyHelper::ED);
  macHelper.SetDeviceType (LoraMacHelper::ED);
  helper.Install (phyHelper, macHelper, endDevices);

  // Now end devices are connected to the channel

  // Connect trace sources
  for (NodeContainer::Iterator j = endDevices.Begin ();
       j != endDevices.End (); ++j)
    {
      Ptr<Node> node = *j;
      Ptr<LoraNetDevice> loraNetDevice = node->GetDevice (0)->GetObject<LoraNetDevice> ();
      Ptr<LoraPhy> phy = loraNetDevice->GetPhy ();
      phy->TraceConnectWithoutContext ("StartSending",
                                       MakeCallback (&TransmissionCallback));
    }

  /*********************
  *  Create Gateways  *
  *********************/

  // Create the gateway nodes (allocate them uniformely on the disc)
  NodeContainer gateways;
  gateways.Create (nGateways);

  Ptr<ListPositionAllocator> allocator = CreateObject<ListPositionAllocator> ();
  allocator->Add (Vector (0.0, 0.0, 0.0));
  mobility.SetPositionAllocator (allocator);
  mobility.Install (gateways);

  // Make it so that nodes are at a certain height > 0
  for (NodeContainer::Iterator j = gateways.Begin ();
       j != gateways.End (); ++j)
    {
      Ptr<MobilityModel> mobility = (*j)->GetObject<MobilityModel> ();
      Vector position = mobility->GetPosition ();
      position.z = 15;
      mobility->SetPosition (position);
    }

  // Create a netdevice for each gateway
  phyHelper.SetDeviceType (LoraPhyHelper::GW);
  macHelper.SetDeviceType (LoraMacHelper::GW);
  helper.Install (phyHelper, macHelper, gateways);

  /************************
  *  Configure Gateways  *
  ************************/

  // Install reception paths on gateways
  for (NodeContainer::Iterator j = gateways.Begin (); j != gateways.End (); j++)
    {

      Ptr<Node> object = *j;
      // Get the device
      Ptr<NetDevice> netDevice = object->GetDevice (0);
      Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
      NS_ASSERT (loraNetDevice != 0);
      Ptr<GatewayLoraPhy> gwPhy = loraNetDevice->GetPhy ()->GetObject<GatewayLoraPhy> ();

      // Set up height of the gateway
      Ptr<MobilityModel> gwMob = (*j)->GetObject<MobilityModel> ();
      Vector position = gwMob->GetPosition ();
      position.z = 15;
      gwMob->SetPosition (position);

      // Global callbacks (every gateway)
      gwPhy->TraceConnectWithoutContext ("ReceivedPacket",
                                         MakeCallback (&PacketReceptionCallback));
      gwPhy->TraceConnectWithoutContext ("LostPacketBecauseInterference",
                                         MakeCallback (&InterferenceCallback));
      gwPhy->TraceConnectWithoutContext ("LostPacketBecauseNoMoreReceivers",
                                         MakeCallback (&NoMoreReceiversCallback));
      gwPhy->TraceConnectWithoutContext ("LostPacketBecauseUnderSensitivity",
                                         MakeCallback (&UnderSensitivityCallback));
      //gwPhy->TraceConnectWithoutContext ("Teste", MakeCallback (&PrintarCoisas));
    }

  /**********************************************
  *  Set up the end device's spreading factor  *
  **********************************************/

 //sfQuantity = macHelper.SetSpreadingFactorsUp (endDevices, gateways, channel);
 macHelper.SetSpreadingFactorsUp (endDevices, gateways, channel);

  NS_LOG_DEBUG ("Completed configuration");

  /*********************************************
  *  Install applications on the end devices  *
  *********************************************/

  Time appStopTime = Seconds (simulationTime);
  PeriodicSenderHelper appHelper = PeriodicSenderHelper ();
  appHelper.SetPeriod (Seconds (appPeriodSeconds));
  ApplicationContainer appContainer = appHelper.Install (endDevices);

  appContainer.Start (Seconds (0));
  appContainer.Stop (appStopTime);

  /**********************
   * Print output files *
   *********************/
  if (printEDs)
    {
      PrintEndDevices (endDevices, gateways,
                       "src/lorawan/examples/endDevices.dat");
  PrintarCoisas ( endDevices, gateways);
    }

  /****************
  *  Simulation  *
  ****************/

  Simulator::Stop (appStopTime + Hours (2));

  // PrintSimulationTime ();

  Simulator::Run ();

  Simulator::Destroy ();

  /*************
  *  Results  *
  *************/
  double receivedProb = double(received)/nDevices;
  double interferedProb = double(interfered)/nDevices;
  double noMoreReceiversProb = double(noMoreReceivers)/nDevices;
  double underSensitivityProb = double(underSensitivity)/nDevices;

  double receivedProbGivenAboveSensitivity = double(received)/(nDevices - underSensitivity);
  double interferedProbGivenAboveSensitivity = double(interfered)/(nDevices - underSensitivity);
  double noMoreReceiversProbGivenAboveSensitivity = double(noMoreReceivers)/(nDevices - underSensitivity);
  //std::cout <<"\n"<< nDevices << " :nDevices\n" << double(nDevices)/simulationTime << " :double(nDevices)/simulationTime\n" << receivedProb << " :double(received)/nDevices\n" << interferedProb << " :double(interfered)/nDevices\n" << noMoreReceiversProb << " :double(noMoreReceivers)/nDevices\n" << underSensitivityProb << " :double(underSensitivity)/nDevices\n" << receivedProbGivenAboveSensitivity << " :double(received)/(nDevices - underSensitivity)\n" << interferedProbGivenAboveSensitivity << " :double(interfered)/(nDevices - underSensitivity)\n" << noMoreReceiversProbGivenAboveSensitivity << " :double(noMoreReceivers)/(nDevices - underSensitivity)\n"<< std::endl;
  std::cout << nDevices << " " << double(nDevices)/simulationTime << " " << receivedProb << " " << interferedProb << " " << noMoreReceiversProb << " " << underSensitivityProb << " " << receivedProbGivenAboveSensitivity << " " << interferedProbGivenAboveSensitivity << " " << noMoreReceiversProbGivenAboveSensitivity<< std::endl;
  received=0;
  interfered=0;
  noMoreReceivers=0;
  underSensitivity=0;
}
  return 0;
}
