===============
Board Support
===============

The code base is expanding very organically from supporting only one board to
supporting multiple architectures and board variants. The strategy we have now:

* Switch between "platforms" with the PLATFORM flag - a platform encapsulates a
  micro architecture and a board variant.
* Implement different architecture-specific code in a subfolder for the micro
* Switch pins for board variants in in those same architecture-specific files
  (like in lights.cpp)
