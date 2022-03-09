#include <stdio.h>
#include <stdint.h>
#include <sndfile.h>
#include <math.h>

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

int cap(int val, int min, int max)
{
    if (val < min)
        return min;
    if (val > max)
        return max;
    return val;
}

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
    printf("sample rate = %d\n", audio_input_format.samplerate);

    FILE *file_binary_out = fopen(argv[2], "wb+");
    if (!file_binary_out)
    {
        printf("Couldn't open %s\n for output", argv[2]);
        return 2;
    }

    static double error = 0;
    while (1)
    {
        double sample;

        if (!sf_read_double(file_audio_in, &sample, 1)) // -1 <= sample <= 1
            break;

        double sample_normalized = (sample + 1.0) * 4; // 0 to 8, 9 levels
        int sample_quantized = cap(round(sample_normalized - error), 0, 8);
        error = sample_quantized - sample_normalized;

        fputc(BYTE_WAVEFORMS[sample_quantized], file_binary_out);
    }

    sf_close(file_audio_in);
    fclose(file_binary_out);

    return 0;
}