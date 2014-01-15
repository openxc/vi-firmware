#include "USB/USB.h"
extern "C"{
#include "fat_filelib.h"
}
#include <vector>
#include <string>
static FL_FILE *readConfig;
static FL_FILE *file;
static bool writeOut;

void mediaSetup();
void mediaShutdown();
void writeToUSB(char *message);
void setWriteStatus(bool status);
bool getWriteStatus();
int media_init();
int media_read(unsigned long sector, unsigned char *buffer, unsigned long sector_count);
int media_write(unsigned long sector, unsigned char *buffer, unsigned long sector_count);
int sequence();

