#ifndef _CANUTIL_CHIPKIT_H_
#define _CANUTIL_CHIPKIT_H_

#include "canutil.h"
#include "canread.h"
#include "canwrite.h"
#include "chipKITCAN.h"

#define SYS_FREQ (80000000L)

extern CanFilterMask* initializeFilterMasks(uint64_t, int*);
extern CanFilter* initializeFilters(uint64_t, int*);

/* Public: Initializes message filter masks and filters on the CAN controller.
 *
 * canMod - a pointer to an initialized CAN module class.
 * filterMasks - an array of the filter masks to initialize.
 * filters - an array of filters to initialize.
 */
void configureFilters(CAN *canMod, CanFilterMask* filterMasks,
        int filterMaskCount, CanFilter* filters, int filterCount);

/* Public: Initialize the CAN controller. See inline comments for description of
 * the process.
 *
 * bus - A CanBus struct defining the bus's metadata for initialization.
 */
void initializeCan(CanBus* bus);

#endif // _CANUTIL_CHIPKIT_H_
