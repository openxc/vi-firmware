#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "can/canutil.h"

namespace openxc {
namespace config {

typedef struct {
    int messageSetIndex;
} Configuration;

Configuration* getConfiguration();

} // namespace config
} // namespace openxc

#endif // _CONFIG_H_
