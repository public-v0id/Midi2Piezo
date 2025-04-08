#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "midi.h"

void throw_error(char* error, char* inp, size_t size, int inpfd, FILE* out) {
	fprintf(stderr, error);
	munmap(inp, size);
	close(inpfd);
	fclose(out);
	
}

int main(int argc, char** argv) {
	char* inp_name = NULL;
	char* out_name = NULL;
	for (int i = 1; i < argc; ++i) {
		if (strncmp(argv[i], "-o", 2) == 0 && i < argc-1) {
			out_name = argv[i];
		}
		else if (inp_name == NULL) {
			inp_name = argv[i];
		}
	}
	if (inp_name == NULL) {
		fprintf(stderr, "Error! Input filename not provided!\n");
		return 0;
	}
	if (out_name == NULL) {
		out_name = "out.rub";
	}
	int inpfd;
	FILE* out_file = fopen(out_name, "a");
	struct stat inp_statbuf;
	if (out_file == NULL) {
		fprintf(stderr, "Error! Unable to open output file\n");
		return 0;
	}
	if ((inpfd = open(inp_name, O_RDWR)) < 0) {
		fprintf(stderr, "Error! Unable to open input file\n");
		fclose(out_file);
		return 0;
	}
	if (fstat(inpfd, &inp_statbuf) < 0) {
		fprintf(stderr, "Error! Unable to find input file end\n");
		close(inpfd);
		fclose(out_file);
		return 0;
	}
	char* inp_file;
	if ((inp_file = mmap(0, inp_statbuf.st_size, PROT_READ, MAP_SHARED, inpfd, 0)) == NULL) {
		fprintf(stderr, "Error! Couldn't map file to memory!\n");
		close(inpfd);
		fclose(out_file);
		return 0;
	}
	midi_file inp_midi;
	if (midiRead(inp_file, &inp_midi, inp_statbuf.st_size) != OK) {
		throw_error("Error! Input file is not a valid MIDI file!\n", inp_file, inp_statbuf.st_size, inpfd, out_file);
		return 0;
	}
	printf("%" PRIu64 " %" PRIu16 " %" PRIu16 " %" PRIu16 " %" PRIu64 "\n", inp_midi.header_size, inp_midi.format, inp_midi.tracks, inp_midi.PPQN, inp_midi.data_size);
	size_t i = 22;
	double bpm = 120.0;
	int prevCom = -1; //0 if On, 1 if Off
	int8_t note = 0;
	int freq[] = {16, 17, 18, 19, 21, 22, 23, 25, 26, 28, 29, 31, 33, 35, 37, 39, 41, 44, 46, 49, 52, 55, 58, 62, 65, 69, 73, 78, 82, 87, 93, 98, 104, 110, 117, 124, 131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247, 262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 493, 523, 554, 587, 622, 659, 699, 740, 784, 831, 880, 932, 988, 1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976, 2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951, 4186, 4485, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902};
	while (i < inp_statbuf.st_size) {
		uint64_t dt = 0;
		while (i < inp_statbuf.st_size && inp_file[i] & 0x80 != 0) {
			dt = dt*128+(uint64_t)((uint8_t)inp_file[i]&0x7F);
			++i;
		}
		if (i >= inp_statbuf.st_size) break;
		dt = dt*128+(uint64_t)((uint8_t)inp_file[i]&0x7F);
		++i;
		if (i >= inp_statbuf.st_size) break;
		printf("dt = %" PRIu64 " i = %"  PRIu64 " data = %" PRIu8 " ", dt, i, (uint8_t)inp_file[i]);
		if ((uint8_t)inp_file[i] == 0xFF) {
			printf("Command\n");
			++i;
			switch(inp_file[i]) {
				case 0x00:
					i += 4;
					break;
				case 0x01:
				case 0x02:
				case 0x03:
				case 0x04:
				case 0x05:
				case 0x06:
				case 0x07: {
					uint64_t len = 0;
					++i;
					while ((i < inp_statbuf.st_size) && ((uint8_t)inp_file[i] >= 128)) {
						len = len*128+(uint64_t)((uint8_t)inp_file[i]&0x7F);
						++i;
					}
					if (i >= inp_statbuf.st_size) break;
					len = len*128+(uint8_t)inp_file[i]&0x7F;
					printf("len is %" PRIu64 " ", len);
					++i;
					if (i+len >= inp_statbuf.st_size) break;
					i += len;
					break; }
				case 0x20:
					i += 3;
					break;
				case 0x2F:
					i+= 2;
					break;
				case 0x51:
					uint64_t tempo = ((uint64_t)inp_file[i+2]*256+(uint8_t)inp_file[i+3])*256+(uint8_t)inp_file[i+4];
					bpm = 60000000.0/tempo;
					i += 5;
					break;
				case 0x54:
					i += 7;
					break;
				case 0x58:
					i += 6;
					break;
				case 0x59:
					i += 4;
					break;
				case 0x7F: {
					uint64_t len = 0;
					++i;
					while ((i < inp_statbuf.st_size) && (inp_file[i] & 0x80 != 0)) {
						len = len*128+(uint64_t)((uint8_t)inp_file[i]&0x7F);
						++i;
					}
					if (i >= inp_statbuf.st_size) break;
					len = len*128+(uint8_t)inp_file[i]&0x7F;
					++i;
					if (i+len+1 >= inp_statbuf.st_size) break;
					i += len+1;
					break; }
			}
		}
		else {
			printf("Normal data\n");
			if ((uint8_t)inp_file[i] >= 192 && (uint8_t)inp_file[i] <= 207) {
				i += 2;
			}
			else {
				if ((uint8_t)inp_file[i] >= 144 && (uint8_t)inp_file[i] <= 169) {
					if (prevCom == 0) {
						throw_error("Error! Converter supports MIDIs with only one note per tine!\n", inp_file, inp_statbuf.st_size, inpfd, out_file);
						return 0;
					}
					note = inp_file[i+1];
					prevCom = 0;
					printf("note = %" PRIu8 " ", note);
				}
				else if ((uint8_t)inp_file[i] >= 128 && (uint8_t)inp_file[i] <= 143) {
					if (prevCom == 1) {
						throw_error("Error! Converter supports MIDIs with only one note per tine!\n", inp_file, inp_statbuf.st_size, inpfd, out_file);
						return 0;
					}
					printf("note = %d length = %f ", note, dt*60.0/(inp_midi.PPQN*bpm));
					fprintf(out_file, "%f, %f, ", 1.0*freq[note], dt*60000.0/(inp_midi.PPQN*bpm));
					prevCom = 1;
				}
				i += 3;
			}
		}
	}
	munmap(inp_file, inp_statbuf.st_size);
	close(inpfd);
	fclose(out_file);
}
