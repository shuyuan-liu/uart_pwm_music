# Play music through your USB-to-UART adapter!

- Resample audio file to 50k samples per second and convert to mono unsigned-8-bit WAV
- Run program to process WAV file
- Connect headphones / amplified speaker to UART TX through a current limiting resistor
- `cat` the resulting file to the UART port at 3M baud
