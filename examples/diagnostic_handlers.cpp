#include "can/canread.h"

float handleMyDiagRequest(const DiagnosticResponse* response, float parsedPayload) {
    return response->payload[0] * 3 + response->payload[1] * 2;
}
