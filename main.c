#include <stdio.h>
#include <stdint.h>

// There are 8 possible output waveforms (and thus different duty cycles) for one byte
const int NUM_LEVELS = 8;

// For 8 bit samples, this many values will result in each PWM level
const int INTERVAL_SIZE = 256 / NUM_LEVELS;

// Dithering by 8x, each sample approximated with 8 output bytes
const double DITHERED_INTERVAL_SIZE = INTERVAL_SIZE / 8;

const uint8_t BYTE_WAVEFORMS[9] =
{
    // With start and stop bits, sent LSB first:
    0x00, // ▁▁▁▁▁▁▁▁▁▆
    0x08, // ▁▁▁▁▆▁▁▁▁▆
    0x12, // ▁▁▆▁▁▆▁▁▁▆
    0x29, // ▁▆▁▁▆▁▆▁▁▆
    0x55, // ▁▆▁▆▁▆▁▆▁▆ --- antisymmetric above and below
    0x6B, // ▁▆▆▁▆▁▆▆▁▆
    0xB7, // ▁▆▆▆▁▆▆▁▆▆
    0xEF, // ▁▆▆▆▆▁▆▆▆▆
    0xFF  // ▁▆▆▆▆▆▆▆▆▆
};

// Every sample represented by 8 bytes. + means this byte is promoted to use
// the next higher PWM level.
const uint8_t DITHER_PATTERNS[8] =
{
    // LSB for the first byte in each 8-byte group
    0x00, // ________
    0x01, // _______+
    0x11, // ___+___+
    0x49, // _+__+__+
    0x55, // _+_+_+_+ --- antisymmetric above and below
    0x6D, // _++_++_+
    0x77, // _+++_+++
    0x7F  // _+++++++
    // No 0xFF because "++++++++" would overlap the next higher "________".
};

int main(int argc, char const *argv[])
{
    if (argc != 3) {
        printf("Usage: %s <u8_mono_wav_file> <output_file>\n", argv[0]);
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

    while (!feof(file_in)) {
        double sample = fgetc(file_in);
        double scaled_sample   = sample / INTERVAL_SIZE;
        int    quantized_level = sample / INTERVAL_SIZE;

        // Amount of correction needed as fraction of a level
        // 0 <= error < 1
        double error = scaled_sample - quantized_level;

        // Choose from 8 fine levels to compensate for error
        // 0 <= dither_step < 8 fits DITHER_PATTERNS[]
        int    dither_step = error * 8; 

        for (int byte_in_group = 0; byte_in_group < 8; byte_in_group++)
        {
            fputc
            (
                BYTE_WAVEFORMS
                [
                    quantized_level
                    + (((1 << byte_in_group) & DITHER_PATTERNS[dither_step]) ? 1 : 0)
                ],
                file_out
            );
        }
    }

    fclose(file_in);
    fclose(file_out);

    return 0;
}
