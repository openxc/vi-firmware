#ifndef _BUFFERS_H_
#define _BUFFERS_H_

/* Public: Set the buffer to all newlines and reset the bufferIndex back to 0.
 *
 * buffer - The buffer to clear.
 * bufferIndex - A pointer to an index variable that should be set back to 0.
 * bufferSize - The maximum size of the buffer.
 */
void resetBuffer(char* buffer, int* bufferIndex, const int bufferSize);

/* Public: Pass the buffer to the callback, which should return true if an
 * OpenXC message is found and processed, then reset the buffer back to all
 * newlines. If no message is found, keep the buffer intact unless the buffer is
 * full or corrupted (i.e. it has a NULL character but we stil didn't find an
 * OpenXC message), reset it back to blank.
 *
 * buffer - The message buffer to check for a message.
 * bufferIndex - The current amount of data in the buffer.
 * bufferSize - The maximum size of the buffer.
 * callback - A function that will return true if an OpenXC message is found in
 * the buffer.
 */
void processBuffer(char* buffer, int* bufferIndex, const int bufferSize,
        bool (*callback)(char*));

#endif // _BUFFERS_H_
