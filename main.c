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

const double ERR_DIFFUSION_PATTERN[8] =
{
    0.195, 0.1875, 0.1728, 0.1515, 0.1243, 0.0924, 0.0569, 0.0192
}; // Should sum to 1

void shift_array_left(double* array, size_t n);


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


    double errors[1 + 8] = {0}; // 1 for current sample and 8 for next 8 samples

    while (1)
    {
        double sample;
        if (!sf_read_double(file_audio_in, &sample, 1)) // -1 <= sample <= 1
        {
            break;
        }

        double sample_normalized = (sample + 1.0) * 4; // 0 to 8, 9 levels
        int    sample_quantized = round(sample_normalized - errors[0]);
        double error = sample_quantized - sample_normalized;
        
        // Diffuse error to next 8 samples
        for (int i = 0; i < 8; i++)
        {
            errors[i + 1] += error * ERR_DIFFUSION_PATTERN[i];
        }

        fputc(BYTE_WAVEFORMS[sample_quantized], file_binary_out);

        shift_array_left(errors, 1 + 8);
    }

    sf_close(file_audio_in);
    fclose(file_binary_out);

    return 0;
}


void shift_array_left(double* array, size_t n)
{
    for (int i = 0; i < n - 1; i++)
    {
        array[i] = array[i + 1];
    }
    array[n - 1] = 0;
}