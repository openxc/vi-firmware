#include "can/canutil.h"
#include "canutil_pic32.h"
#include "signals.h"
#include "util/log.h"
#include "gpio.h"

#if defined(CROSSCHASM_C5_CT)
    #define CAN1_TRANSCEIVER_SWITCHED
    #define CAN1_TRANSCEIVER_ENABLE_POLARITY    0
    #define CAN1_TRANSCEIVER_ENABLE_PIN            38 // PORTD BIT10 (RD10)
#elif defined(CROSSCHASM_C5_CELLULAR)
    #define CAN1_TRANSCEIVER_SWITCHED
    #define CAN1_TRANSCEIVER_ENABLE_POLARITY    0
    #define CAN1_TRANSCEIVER_ENABLE_PIN            38 // PORTD BIT10 (RD10)
#endif

#define CAN_RX_CHANNEL 1
#define BUS_MEMORY_BUFFER_SIZE 2 * 8 * 16

namespace gpio = openxc::gpio;

using openxc::signals::getCanBuses;
using openxc::util::log::debug;
using openxc::gpio::GpioValue;
using openxc::gpio::GPIO_VALUE_LOW;
using openxc::gpio::GPIO_VALUE_HIGH;
using openxc::gpio::GPIO_DIRECTION_OUTPUT;

CAN can1Actual(CAN::CAN1);
CAN can2Actual(CAN::CAN2);
CAN* can1 = &can1Actual;
CAN* can2 = &can2Actual;

/* Private:  A message area for each bus, for 2 channels to store 8 16 byte
 * messages - required by the PIC32 CAN library. We could add this to the CanBus
 * struct, but the PIC32 has way more memory than some of our other supported
 * platforms so I don't want to burden them unnecessarily.
 */
uint8_t CAN_CONTROLLER_BUFFERS[2][BUS_MEMORY_BUFFER_SIZE];

static CAN::OP_MODE switchControllerMode(CanBus* bus, CAN::OP_MODE mode) {
    CAN::OP_MODE previousMode = CAN_CONTROLLER(bus)->getOperatingMode();
    if(previousMode != mode) {
        CAN_CONTROLLER(bus)->setOperatingMode(mode);
        while(CAN_CONTROLLER(bus)->getOperatingMode() != mode);
    }
    return previousMode;
}

bool openxc::can::resetAcceptanceFilterStatus(CanBus* bus, bool enabled) {
    CAN::OP_MODE previousMode = switchControllerMode(bus, CAN::CONFIGURATION);
    debug("Resetting AF filter on bus %d - will wipe all "
            "previous configuration", bus->address);
    if(enabled) {
        debug("Enabling primary AF filter masks for bus %d", bus->address);
        CAN_CONTROLLER(bus)->configureFilterMask(CAN::FILTER_MASK0, 0xFFF,
                CAN::SID, CAN::FILTER_MASK_IDE_TYPE);
        CAN_CONTROLLER(bus)->configureFilterMask(CAN::FILTER_MASK1, 0x1FFFFFFF,
                CAN::EID, CAN::FILTER_MASK_IDE_TYPE);
    } else {
        debug("Disabling primary AF filter mask for bus %d to allow "
                "all messages through", bus->address);
        CAN_CONTROLLER(bus)->configureFilterMask(CAN::FILTER_MASK0, 0,
                CAN::SID, CAN::FILTER_MASK_ANY_TYPE);
        CAN_CONTROLLER(bus)->configureFilter(CAN::FILTER0, 0, CAN::SID);
        CAN_CONTROLLER(bus)->linkFilterToChannel(
                CAN::FILTER0, CAN::FILTER_MASK0, CAN::CHANNEL1);

        CAN_CONTROLLER(bus)->configureFilter(CAN::FILTER1, 0, CAN::EID);
        CAN_CONTROLLER(bus)->linkFilterToChannel(
                CAN::FILTER1, CAN::FILTER_MASK0, CAN::CHANNEL1);

        CAN_CONTROLLER(bus)->enableFilter(CAN::FILTER0, true);
        CAN_CONTROLLER(bus)->enableFilter(CAN::FILTER1, true);
    }
    switchControllerMode(bus, previousMode);
    return true;
}

bool openxc::can::updateAcceptanceFilterTable(CanBus* buses, const int busCount) {
    // For the PIC32 we *could* only change the filters for one bus, but to
    // simplify things we'll reset everything like we have to with the LPC1768
    uint16_t totalFilterCount = 0;
    for(int i = 0; i < busCount; i++) {
        CanBus* bus = &buses[i];
        uint16_t busFilterCount = 0;
        CAN::OP_MODE previousMode = switchControllerMode(bus, CAN::CONFIGURATION);

        if(LIST_EMPTY(&bus->acceptanceFilters) || bus->bypassFilters) {
            debug("Bus %d has no filters configured or manually set to bypass, "
                    "turning off acceptance filter", bus->address);
            resetAcceptanceFilterStatus(bus, false);
        } else {
            // Must set the controller's AF filter status first and only once,
            // before configuring, because it wipes anything you've configured
            // when you set it.
            resetAcceptanceFilterStatus(bus, true);

            AcceptanceFilterListEntry* entry;
            LIST_FOREACH(entry, &bus->acceptanceFilters, entries) {
                if(busFilterCount >= MAX_ACCEPTANCE_FILTERS) {
                    break;
                }

                // Must disable before changing or else the filters do not work!
                CAN_CONTROLLER(bus)->enableFilter(CAN::FILTER(totalFilterCount), false);
                if(entry->format == CanMessageFormat::STANDARD) {
                    // Standard format message IDs match filter mask 0
                    debug("Added acceptance filter for STD 0x%x on bus %d to AF",
                            entry->filter, bus->address);
                    CAN_CONTROLLER(bus)->configureFilter(
                            CAN::FILTER(totalFilterCount), entry->filter, CAN::SID);
                    CAN_CONTROLLER(bus)->linkFilterToChannel(CAN::FILTER(totalFilterCount),
                            CAN::FILTER_MASK0, CAN::CHANNEL(CAN_RX_CHANNEL));
                } else {
                    // Extended format message IDs match filter mask 1
                    debug("Added acceptance filter for EXT 0x%x on bus %d to AF",
                            entry->filter, bus->address);
                    CAN_CONTROLLER(bus)->configureFilter(
                            CAN::FILTER(totalFilterCount), entry->filter, CAN::EID);
                    CAN_CONTROLLER(bus)->linkFilterToChannel(CAN::FILTER(totalFilterCount),
                            CAN::FILTER_MASK1, CAN::CHANNEL(CAN_RX_CHANNEL));
                }
                CAN_CONTROLLER(bus)->enableFilter(CAN::FILTER(totalFilterCount), true);

                ++totalFilterCount;
                ++busFilterCount;
            }

            // Disable the remaining unused filters. When AF is "off" we are
            // actually using filter 0, so we don't want to disable that.
            for(int disabledFilters = busFilterCount;
                    disabledFilters < MAX_ACCEPTANCE_FILTERS; ++disabledFilters) {
                CAN_CONTROLLER(bus)->enableFilter(CAN::FILTER(disabledFilters), false);
            }
        }

        switchControllerMode(bus, previousMode);
    }

    return true;
}

/* Public: Change the operational mode of the specified CAN module to
 * CAN_DISABLE. Also set state of any off-chip CAN line driver as needed for
 * platform.
 *
 * CAN module will still be capable of wake from sleep.
 * The OP_MODE of the CAN module itself is actually irrelevant
 * when going to sleep. The main reason for this is to provide a generic
 * function call to disable the off-chip transceiver(s), which saves power,
 * without disabling the CAN module itself.
 */
void openxc::can::deinitialize(CanBus* bus) {
    switchControllerMode(bus, CAN::DISABLE);

    // disable off-chip line driver
    #if defined(CAN1_TRANSCEIVER_SWITCHED)
    GpioValue value = CAN1_TRANSCEIVER_ENABLE_POLARITY ? GPIO_VALUE_LOW : GPIO_VALUE_HIGH;
    gpio::setDirection(0, CAN1_TRANSCEIVER_ENABLE_PIN, GPIO_DIRECTION_OUTPUT);
    gpio::setValue(0, CAN1_TRANSCEIVER_ENABLE_PIN, value);
    #endif
}

/* Called by the Interrupt Service Routine whenever an event we registered for
 * occurs - this is where we wake up and decide to process a message. */
static void handleCan1Interrupt() {
    openxc::can::pic32::handleCanInterrupt(&getCanBuses()[0]);
}

static void handleCan2Interrupt() {
    openxc::can::pic32::handleCanInterrupt(&getCanBuses()[1]);
}

void openxc::can::initialize(CanBus* bus, bool writable, CanBus* buses,
        const int busCount) {
    can::initializeCommon(bus);
    // Switch the CAN module ON and switch it to Configuration mode. Wait till
    // the switch is complete
    CAN_CONTROLLER(bus)->enableModule(true);
    switchControllerMode(bus, CAN::CONFIGURATION);

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
    CAN_CONTROLLER(bus)->assignMemoryBuffer(
            CAN_CONTROLLER_BUFFERS[bus->address - 1],
            BUS_MEMORY_BUFFER_SIZE);

    // Configure channel 0 for TX with 8 byte buffers and with "Remote Transmit
    // Request" disabled, meaning that other nodes can't request for us to
    // transmit data.
    CAN_CONTROLLER(bus)->configureChannelForTx(CAN::CHANNEL0, 8,
            CAN::TX_RTR_DISABLED, CAN::LOW_MEDIUM_PRIORITY);

    // Configure channel 1 for RX with 8 byte buffers - remember this is channel
    // 1 on the given bus, it doesn't mean CAN1 or CAN2 on the chipKIT board.
    CAN_CONTROLLER(bus)->configureChannelForRx(CAN::CHANNEL1, 8,
            CAN::RX_FULL_RECEIVE);

    if(!configureDefaultFilters(bus, openxc::signals::getMessages(),
                openxc::signals::getMessageCount(), buses, busCount)) {
        debug("Unable to initialize CAN acceptance filters");
    }

    // Enable interrupt and events. Enable the receive channel not empty event
    // (channel event) and the receive channel event (module event). The
    // interrrupt peripheral library is used to enable the CAN interrupt to the
    // CPU.
    CAN_CONTROLLER(bus)->enableChannelEvent(CAN::CHANNEL1,
            CAN::RX_CHANNEL_NOT_EMPTY, true);
    CAN_CONTROLLER(bus)->enableModuleEvent(CAN::RX_EVENT, true);

    // enable the bus activity wake-up event (to enable wake from sleep)
    CAN_CONTROLLER(bus)->enableModuleEvent(
            CAN::BUS_ACTIVITY_WAKEUP_EVENT, true);
    CAN_CONTROLLER(bus)->enableFeature(CAN::WAKEUP_BUS_FILTER, true);

    // switch ON off-chip CAN line drivers (if necessary)
    #if defined(CAN1_TRANSCEIVER_SWITCHED)
    GpioValue value = CAN1_TRANSCEIVER_ENABLE_POLARITY ? GPIO_VALUE_HIGH : GPIO_VALUE_LOW;
    gpio::setDirection(0, CAN1_TRANSCEIVER_ENABLE_PIN, GPIO_DIRECTION_OUTPUT);
    gpio::setValue(0, CAN1_TRANSCEIVER_ENABLE_PIN, value);
    #endif

    // move CAN module to OPERATIONAL state (go on bus)
    CAN::OP_MODE mode;
    if(bus->loopback) {
        debug("Initializing bus %d in loopback mode", bus->address);
        mode = CAN::LOOPBACK;
    } else if(writable) {
        debug("Initializing bus %d in writable mode", bus->address);
        mode = CAN::NORMAL_OPERATION;
    } else {
        debug("Initializing bus %d in listen only mode", bus->address);
        mode = CAN::LISTEN_ONLY;
    }

    switchControllerMode(bus, mode);

    // TODO an error if the address isn't valid
    CAN_CONTROLLER(bus)->attachInterrupt(bus->address == 1 ?
            handleCan1Interrupt : handleCan2Interrupt);
    debug("Done.");
}
