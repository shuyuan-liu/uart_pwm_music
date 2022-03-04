#include <stdio.h>
#include <stdint.h>
#include <sndfile.h>

// There are 9 possible output waveforms (and thus different duty cycles)
// for one byte. This gives 8 intervals between the levels.
const int NUM_LEVELS = 8;

// For 8 bit samples, this many values will result in each PWM level
const double INTERVAL_SIZE = 1.0 / NUM_LEVELS;

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
    if (argc != 3)
    {
        printf("Usage: %s <input_audio_file> <output_file>\n", argv[0]);
        return 1;
    }

    SF_INFO audio_input_format;
    SNDFILE *file_audio_in = sf_open(argv[1], SFM_READ, &audio_input_format);
    if (!file_audio_in)
    {
        printf("Couldn't open %s for input\n", argv[1]);
        return 2;
    }

    FILE *file_binary_out = fopen(argv[2], "wb+");
    if (!file_binary_out)
    {
        printf("Couldn't open %s\n for output", argv[2]);
        return 2;
    }

    while (1)
    {
        double sample;
        if (!sf_read_double(file_audio_in, &sample, 1)) // -1 <= sample <= 1
        {
            break;
        }
        //printf("s=%+0.2f, ", sample);

        double sample_normalized = (sample + 1.0) * 4; // 0 to 8, 9 levels
        int sample_quantized = (int)sample_normalized;
        //printf("q=%d, ", sample_quantized);

        // Amount of correction needed as fraction of a level
        // 0 <= error < 1
        double error = sample_normalized - sample_quantized;
        //printf("e=%0.2lf, ", error);

        // Choose from 8 fine levels to compensate for error
        // 0 <= dither_step < 8 fits DITHER_PATTERNS[]
        int dither_step = error * 8;
        //printf("d=%d, ", dither_step);

        for (int i = 0; i < 8; i++)
        {
            //printf("%d ", sample_quantized + (((1 << i) & DITHER_PATTERNS[dither_step]) ? 1 : 0));
            uint8_t byte_to_write = BYTE_WAVEFORMS[sample_quantized + (((1 << i) & DITHER_PATTERNS[dither_step]) ? 1 : 0)];
            fputc(byte_to_write, file_binary_out);
        }
        //printf("\n");
    }

    sf_close(file_audio_in);
    fclose(file_binary_out);

    return 0;
}
