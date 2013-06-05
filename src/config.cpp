#include "config.h"

openxc::config::Configuration* openxc::config::getConfiguration() {
    static openxc::config::Configuration CONFIG;
    return &CONFIG;
}
