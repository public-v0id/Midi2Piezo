#ifndef MIDI_H
#define MIDI_H
#include <inttypes.h>
#include <string.h>

typedef struct {
	uint64_t header_size;
	uint16_t format;
	uint16_t tracks;
	uint16_t PPQN;
	uint64_t data_size;
} midi_file;

enum midi_error {
	OK = 0,
	INVALIDFORMAT = -1
};

enum midi_error midiRead(char* buf, midi_file* file, size_t sz);
#endif
