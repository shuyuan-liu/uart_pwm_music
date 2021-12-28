#include <stdio.h>

const int LEVELS_EACH_BYTE = 9; // 0b00000000, 0b10000000, 0b11000000, ..., 0b11111111
const double LEVEL_STEP_SIZE = 256.0f / LEVELS_EACH_BYTE; // For 8 bit samples, this many values will result in each PWM level
const int DITHER_RATE_FACTOR = 6; // Approximate each sample with 8 PWM bytes
const double DITHER_OFFSET_STEP = LEVEL_STEP_SIZE / DITHER_RATE_FACTOR;

int main(int argc, char const *argv[])
{
    if (argc != 3) {
        printf("Usage: %s <u8_wav_file> <output_file>\n", argv[0]);
        return 1;
    }

    FILE* file_in = fopen(argv[1], "rb");
    if (!file_in) {
        printf("Couldn't open %s for input\n", argv[1]);
        return 2;
    }

    FILE* file_out = fopen(argv[2], "wb+");
    if (!file_out) {
        printf("Couldn't open %s\n for output", argv[2]);
        return 2;
    }

    unsigned int byte_count = 0;
    while (!feof(file_in)) {
        unsigned char sample = fgetc(file_in);
        fputc(0xFF << (int)(sample / LEVEL_STEP_SIZE), file_out);
        fputc(0xFF << (int)((sample + 2 * DITHER_OFFSET_STEP) / LEVEL_STEP_SIZE), file_out);
        fputc(0xFF << (int)((sample + 4 * DITHER_OFFSET_STEP) / LEVEL_STEP_SIZE), file_out);
        fputc(0xFF << (int)((sample + 1 * DITHER_OFFSET_STEP) / LEVEL_STEP_SIZE), file_out);
        fputc(0xFF << (int)((sample + 3 * DITHER_OFFSET_STEP) / LEVEL_STEP_SIZE), file_out);
        fputc(0xFF << (int)((sample + 5 * DITHER_OFFSET_STEP) / LEVEL_STEP_SIZE), file_out);
        byte_count++;
    }

    printf("Processed %ld samples\n", byte_count);
    fclose(file_in);
    fclose(file_out);

    return 0;
}