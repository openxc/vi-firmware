#ifndef __INTERFACE_H__
#define __INTERFACE_H__

namespace openxc {
namespace interface {

typedef enum {
    USB = 0,
    UART = 1,
    NETWORK = 2,
    TELIT = 3
} InterfaceType;

/* Public:
 *
 * type - The type of this interface, one of InterfaceType.
 * allowRawWrites - if raw CAN messages writes are enabled for a bus and this is
 *      true, accept raw write requests from the USB interface.
 */
typedef struct {
    bool allowRawWrites;
    InterfaceType type;
} InterfaceDescriptor;

const char* descriptorToString(InterfaceDescriptor* descriptor);

/* Public: Return true if any of the output interfaces is connected.
 */
bool anyConnected();

} // namespace interface
} // namespace openxc

#endif // __INTERFACE_H__
