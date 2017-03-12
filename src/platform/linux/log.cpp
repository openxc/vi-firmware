#include <iostream>
#include "util/log.h"

using namespace std;

void openxc::util::log::initialize() {
	cout << "Logger initialized\n";
}

void openxc::util::log::debugUart(const char* message) {
	cout << message;
}
