#include "diagnostics.h"
#include "signals.h"
#include "can/canwrite.h"
#include "util/log.h"
#include <bitfield/bitfield.h>
#include <limits.h>

using openxc::diagnostics::ActiveRequestList;
using openxc::diagnostics::ActiveDiagnosticRequest;
using openxc::diagnostics::ActiveRequestListEntry;
using openxc::diagnostics::DiagnosticsManager;
using openxc::signals::getCanBuses;
using openxc::util::log::debug;

DiagnosticRequestHandle DIAG_HANDLE;

static ActiveRequestListEntry* popListEntry(ActiveRequestList* list) {
    ActiveRequestListEntry* result = list->lh_first;
    if(result != NULL) {
        LIST_REMOVE(list->lh_first, entries);
    }
    return result;
}

static bool sendDiagnosticCanMessage(const uint16_t arbitration_id,
        const uint8_t* data, const uint8_t size) {
    // TODO right now this doesn't support sending requests on anything but bus
    // 1. a few things need to change to support uses bus 2, or using different
    // buses for different requests.
    //
    // this is OK to use the signals.h functions directly here, since we intend
    // for those to change at runtime soon - we don't want to provide the buses
    // when the DiagnosticsManager is initialized, for example.
    //
    // second, the send_can_message shim prototype from isotp-c doesn't have an
    // argument to identify the bus for the message, so we need to add that
    const CanBus* bus = &getCanBuses()[0];
    const CanMessage message = {arbitration_id, get_bitfield(data, size, 0,
            size * CHAR_BIT)};
    return openxc::can::write::sendCanMessage(bus, &message);
}

void openxc::diagnostics::initialize(DiagnosticsManager* manager) {
    manager->shims = diagnostic_init_shims(openxc::util::log::debug,
                       sendDiagnosticCanMessage, NULL);

    LIST_INIT(&manager->activeRequests);
    LIST_INIT(&manager->freeActiveRequests);

    for(int i = 0; i < MAX_SIMULTANEOUS_DIAG_REQUESTS; i++) {
        LIST_INSERT_HEAD(&manager->freeActiveRequests,
                &manager->activeListEntries[i], entries);
    }
}

void openxc::diagnostics::sendRequests(DiagnosticsManager* manager) {
    for(ActiveRequestListEntry* activeRequest = manager->activeRequests.lh_first;
            activeRequest != NULL; activeRequest = activeRequest->entries.le_next) {
        // TODO
        // is it recurring?
        // is it completed?
        // is the clock ticked?
        //      send again!
    }
}

void openxc::diagnostics::receiveCanMessage(DiagnosticsManager* manager,
        CanMessage* message) {
    if(!DIAG_HANDLE.completed) {
        // TODO could we put this union in CanMessage? may not be necessary
        // if we migrate way from uint64_t but i don't think it would
        // negatively impact memory
        ArrayOrBytes combined;
        combined.whole = message->data;
        DiagnosticResponse response = diagnostic_receive_can_frame(&manager->shims,
                &DIAG_HANDLE, message->id, combined.bytes,
                sizeof(combined.bytes));
        if(response.completed && DIAG_HANDLE.completed) {
            if(DIAG_HANDLE.success) {
                if(response.success) {
                    debug("Diagnostic response received: arb_id: 0x%02x, mode: 0x%x, pid: 0x%x, payload: 0x%02x%02x%02x%02x%02x%02x%02x%02x, size: %d",
                            response.arbitration_id,
                            response.mode,
                            response.pid,
                            response.payload[0],
                            response.payload[1],
                            response.payload[2],
                            response.payload[3],
                            response.payload[4],
                            response.payload[5],
                            response.payload[6],
                            response.payload[7],
                            response.payload_length);

                } else {
                    debug("Negative diagnostic response received, NRC: 0x%x",
                            response.negative_response_code);
                }
            } else {
                debug("Fatal error wen sending or receiving diagnostic request");
            }
        }
    }
}

static void cleanupActiveRequests(DiagnosticsManager* manager) {
    // clean up the active request list, move as many to the free list as
    // possible
    for(ActiveRequestListEntry* entry = manager->activeRequests.lh_first;
            entry != NULL; entry = entry->entries.le_next) {
        ActiveDiagnosticRequest* request = &entry->request;
        if(request->handle.completed && !request->recurring) {
            LIST_INSERT_HEAD(&manager->freeActiveRequests, entry, entries);
            LIST_REMOVE(entry, entries);
        }
    }

}

/* Public:
 *
 * frequencyHz - a value of 0 means it's a non-recurring request.
 */
bool openxc::diagnostics::addDiagnosticRequest(DiagnosticsManager* manager,
        DiagnosticRequestHandle* handle, const char* genericName,
        const DiagnosticResponseDecoder decoder, const uint8_t frequencyHz) {

    ActiveRequestListEntry* newEntry = popListEntry(
            &manager->freeActiveRequests);
    if(newEntry == NULL) {
        cleanupActiveRequests(manager);
        newEntry = popListEntry(&manager->freeActiveRequests);
    }

    if(newEntry == NULL) {
        debug("Unable to allocate space for a new diagnostic request");
        // TODO at the moment we're *never* deleting requests that were going to
        // the broadcast address, because they're never "completed" unless we
        // time them out while waiting for response from multiple modules
        return false;
    }

    newEntry->request.handle = *handle;
    strncpy(newEntry->request.genericName, genericName, MAX_GENERIC_NAME_LENGTH);
    newEntry->request.decoder = decoder;
    newEntry->request.recurring = frequencyHz != 0;
    if(newEntry->request.recurring) {
        newEntry->request.frequencyClock = {0};
        newEntry->request.frequencyClock.frequency = frequencyHz;
    }

    return true;
}

bool openxc::diagnostics::addDiagnosticRequest(DiagnosticsManager* manager,
        DiagnosticRequestHandle* handle, const char* genericName,
        const DiagnosticResponseDecoder decoder) {
    return addDiagnosticRequest(manager, handle, genericName, decoder, 0);
}

bool openxc::diagnostics::addDiagnosticRequest(DiagnosticsManager* manager,
        DiagnosticRequest* request, const char* genericName,
        const DiagnosticResponseDecoder decoder) {
    return addDiagnosticRequest(manager, request, genericName, decoder, 0);
}

bool openxc::diagnostics::addDiagnosticRequest(DiagnosticsManager* manager,
        DiagnosticRequest* request, const char* genericName,
        const DiagnosticResponseDecoder decoder, const uint8_t frequencyHz) {
    DiagnosticRequestHandle handle = diagnostic_request(&manager->shims, request, NULL);
    return addDiagnosticRequest(manager, &handle, genericName, decoder,
            frequencyHz);
}
