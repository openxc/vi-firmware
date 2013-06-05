#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "can/canutil.h"

namespace openxc {
namespace config {

typedef struct {
    int messageSetIndex;
} Configuration;

static Configuration CONFIG;

} // namespace config
} // namespace openxc

#endif // _CONFIG_H_
