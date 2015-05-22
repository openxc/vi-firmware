#ifndef _NVMEM_H_
#define _NVMEM_H_

#define NVM_START    0x9D07F000
#define NVM_SIZE     0x1000

namespace openxc {
namespace nvm {

void initialize();
void store();
void load();

} // namespace nvm
} // namespace openxc

#endif
