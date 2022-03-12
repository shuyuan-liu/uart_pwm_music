#include <stdio.h>
#include <stdint.h>
#include <sndfile.h>

int main(int argc, char const* argv[])
{
    if (argc != 4) {
        printf("Usage: %s <input_audio_file> <output_file> <output_simulated_audio_file>\n", argv[0]);
        return 1;
    }

    SF_INFO audio_input_format;
    SNDFILE* file_audio_in = sf_open(argv[1], SFM_READ, &audio_input_format);
    if (!file_audio_in) {
        printf("Couldn't open %s for input\n", argv[1]);
        return 2;
    }

    SF_INFO audio_output_format = audio_input_format;
    audio_output_format.format = SF_FORMAT_WAV | SF_FORMAT_PCM_U8;
    SNDFILE* file_audio_out = sf_open(argv[3], SFM_WRITE, &audio_output_format);
    if (!file_audio_out) {
        printf("Couldn't open %s for output\n", argv[3]);
        return 2;
    }

    FILE* file_binary_out = fopen(argv[2], "wb+");
    if (!file_binary_out) {
        printf("Couldn't open %s for output", argv[2]);
        return 2;
    }

    size_t samples_precessed = 0;
    double input_buffer[10] = {0};
    double output_buffer[10] = {0};
    double integrator_1 = 0.0;
    double integrator_2 = 0.0;

    while (sf_read_double(file_audio_in, input_buffer, 10) == 10) {
        uint8_t out_byte = 0x00;

        for (int i = 0; i < 10; i++) {
            double orig_sample = input_buffer[i];
            double quantized_sample;

            if (i == 0) {
                quantized_sample = -1;
            } else if (i == 9) {
                quantized_sample = 1;
            } else {
                quantized_sample = (integrator_2 > 0) ? 1 : -1;
            }

            integrator_1 += 1 * (orig_sample - quantized_sample);
            integrator_2 += 1 * (integrator_1 - quantized_sample);

            // printf("%.2lf,%.2lf,%.2lf,%.0f\n", orig_sample, integrator_1, integrator_2, quantized_sample);

            if (i != 0 && i != 9 && quantized_sample == 1) { out_byte |= (1 << (i - 1)); }
            output_buffer[i] = quantized_sample;
        }

        sf_write_double(file_audio_out, output_buffer, 10);

        fputc(out_byte, file_binary_out);   
        
        samples_precessed += 10;
        // if (samples_precessed % 10000 == 0) {
        //     printf("%.1f%%\n",
        //            (double)samples_precessed / audio_input_format.frames * 100);
        // }
    }

    sf_close(file_audio_in);
    sf_close(file_audio_out);
    fclose(file_binary_out);

    return 0;
}