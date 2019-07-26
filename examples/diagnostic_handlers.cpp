#include "can/canread.h"

void handleMyDiagRequest(const DiagnosticResponse* response, float parsedPayload, char* str_buf, int buf_size) {
    snprintf(str_buf, buf_size, "%f", (float) (response->payload[0] * 3 + response->payload[1] * 2));
}
