#include "log.h"
#include "ethernetutil.h"

void initializeEthernet(uint8_t MACAddr[], uint8_t IPAddr[], Server& server)
{
	debug("initializing Ethernet...");
	Ethernet.begin(MACAddr, IPAddr);
	server.begin();
}
void processEthernetSendQueue(EthernetDevice* device)
{

}
