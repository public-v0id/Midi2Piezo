#include "midi.h"

enum midi_error midiRead(char* buf, midi_file* file, size_t sz) {
	if (sz < 22 || (strncmp(buf, "MThd", 4) != 0) || (strncmp(buf+14, "MTrk", 4) != 0)) {
		return INVALIDFORMAT;
	}
	file->header_size = 6;
	file->format = ((uint16_t)buf[8]*256)+(uint8_t)buf[9];
	file->tracks = ((uint16_t)buf[10]*256)+(uint8_t)buf[11];
	file->PPQN = ((uint16_t)buf[12]*256)+(uint8_t)buf[13];
	file->data_size = (((uint64_t)buf[18]*256+(uint8_t)buf[19])*256+(uint8_t)buf[20])*256+(uint8_t)buf[21];
	return OK;
}
