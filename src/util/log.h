#ifndef _LOG_H_
#define _LOG_H_

namespace openxc {
namespace util {
namespace log {

extern const int MAX_LOG_LINE_LENGTH;

/* Public: Initialize the debug logging framework. This function must be called
 *      before using debug().
 */
void initialize();

/* Public: Construct a string for the given format and args and output it on
 * whatever debug interface the current platform is using.
 *
 * This appends a \r\n to the end of the message.
 *
 * format - A printf-style format string.
 * args - printf-style arguments that match the format string.
 */
void debug(const char* format, ...);

/* Private: Log a completed message to UART.
 */
void debugUart(const char* message);

} // namespace log
} // namespace util
} // namespace openxc

#endif // _LOG_H_
