#include "canutil.h"
#include "canutil_pic32.h"
#include "signals.h"
#include "log.h"

CAN can1Actual(CAN::CAN1);
CAN can2Actual(CAN::CAN2);
CAN* can1 = &can1Actual;
CAN* can2 = &can2Actual;

/* Private: Initializes message filters on the CAN controller.
 *
 * bus - The CanBus instance to configure the filters for.
 * filters - An array of filters to initialize.
 * filterCount - The length of the filters array.
 */
void configureFilters(CanBus* bus, CanFilter* filters, int filterCount) {
    if(filterCount > 0) {
        debugNoNewline("Configuring %d filters...", filterCount);
        CAN_CONTROLLER(bus)->configureFilterMask(CAN::FILTER_MASK0, 0xFFF,
                CAN::SID, CAN::FILTER_MASK_IDE_TYPE);
        for(int i = 0; i < filterCount; i++) {
            CAN_CONTROLLER(bus)->configureFilter(
                    (CAN::FILTER) filters[i].number, filters[i].value,
                    CAN::SID);
            CAN_CONTROLLER(bus)->linkFilterToChannel(
                    (CAN::FILTER) filters[i].number, CAN::FILTER_MASK0,
                    (CAN::CHANNEL) filters[i].channel);
            CAN_CONTROLLER(bus)->enableFilter((CAN::FILTER) filters[i].number,
                    true);
        }
        debug("Done.");
    } else {
        debug("No filters configured, turning off acceptance filter");
        CAN_CONTROLLER(bus)->configureFilterMask(CAN::FILTER_MASK0, 0, CAN::SID,
            CAN::FILTER_MASK_IDE_TYPE);
        CAN_CONTROLLER(bus)->configureFilter(
                CAN::FILTER0, 0, CAN::SID);
        CAN_CONTROLLER(bus)->linkFilterToChannel(
                CAN::FILTER0, CAN::FILTER_MASK0, CAN::CHANNEL1);
        CAN_CONTROLLER(bus)->enableFilter(CAN::FILTER0,
                true);
    }
}

void initializeCan(CanBus* bus) {
    initializeCanCommon(bus);
    // Switch the CAN module ON and switch it to Configuration mode. Wait till
    // the switch is complete
    CAN_CONTROLLER(bus)->enableModule(true);
    CAN_CONTROLLER(bus)->setOperatingMode(CAN::CONFIGURATION);
    while(CAN_CONTROLLER(bus)->getOperatingMode() != CAN::CONFIGURATION);

    // Configure the CAN Module Clock. The CAN::BIT_CONFIG data structure is
    // used for this purpose. The propagation, phase segment 1 and phase segment
    // 2 are configured to have 3TQ. The CANSetSpeed() function sets the baud.
    CAN::BIT_CONFIG canBitConfig;
    canBitConfig.phaseSeg2Tq            = CAN::BIT_3TQ;
    canBitConfig.phaseSeg1Tq            = CAN::BIT_3TQ;
    canBitConfig.propagationSegTq       = CAN::BIT_3TQ;
    canBitConfig.phaseSeg2TimeSelect    = CAN::TRUE;
    canBitConfig.sample3Time            = CAN::TRUE;
    canBitConfig.syncJumpWidth          = CAN::BIT_2TQ;
    CAN_CONTROLLER(bus)->setSpeed(&canBitConfig, SYS_FREQ, bus->speed);

    // Assign the buffer area to the CAN module. Note the size of each Channel
    // area. It is 2 (Channels) * 8 (Messages Buffers) 16 (bytes/per message
    // buffer) bytes. Each CAN module should have its own message area.
    CAN_CONTROLLER(bus)->assignMemoryBuffer(bus->buffer, BUS_MEMORY_BUFFER_SIZE);

    // Configure channel 0 for TX with 8 byte buffers and with "Remote Transmit
    // Request" disabled, meaning that other nodes can't request for us to
    // transmit data.
    CAN_CONTROLLER(bus)->configureChannelForTx(CAN::CHANNEL0, 8, CAN::TX_RTR_DISABLED,
            CAN::LOW_MEDIUM_PRIORITY);

    // Configure channel 1 for RX with 8 byte buffers - remember this is channel
    // 1 on the given bus, it doesn't mean CAN1 or CAN2 on the chipKIT board.
    CAN_CONTROLLER(bus)->configureChannelForRx(CAN::CHANNEL1, 8,
            CAN::RX_FULL_RECEIVE);

    int filterCount;
    CanFilter* filters = initializeFilters(bus->address, &filterCount);
    configureFilters(bus, filters, filterCount);

    // Enable interrupt and events. Enable the receive channel not empty event
    // (channel event) and the receive channel event (module event). The
    // interrrupt peripheral library is used to enable the CAN interrupt to the
    // CPU.
    CAN_CONTROLLER(bus)->enableChannelEvent(CAN::CHANNEL1, CAN::RX_CHANNEL_NOT_EMPTY,
            true);
    CAN_CONTROLLER(bus)->enableModuleEvent(CAN::RX_EVENT, true);

    CAN_CONTROLLER(bus)->setOperatingMode(CAN::NORMAL_OPERATION);
    while(CAN_CONTROLLER(bus)->getOperatingMode() != CAN::NORMAL_OPERATION);

    CAN_CONTROLLER(bus)->attachInterrupt(bus->interruptHandler);

    debug("Done.");
}
