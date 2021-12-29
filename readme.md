# Play music through your USB-to-UART adapter!

## Usage

- Compile this program with your C compiler
- Resample your audio file to 50k samples per second and convert to a mono unsigned-8-bit WAV
	- Example using FFmpeg: `ffmpeg -i input.mp3 -ar 50k -ac 1 -c:a pcm_u8 resampled.wav`
	- Example using SoX: `sox input.wav -r 50k -c 1 -e unsigned-integer -b 8 resampled.wav`

- Run this program to process the WAV file
	- Assuming the compiler named the executable `a.out`, run `./a.out resampled.wav output.bin`

- Connect headphones / amplified speaker to UART TX through a current limiting resistor
- `cat` the resulting file to the UART port at 3,000,000 baud
	- Example: `cat output.bin >/dev/ttyUSB0`


## How it works

```
   Start              Stop
━━━━┑ ┍━┯━┯━┯━┯━┯━┯━┯━┯━━━━━
    │0│X│X│X│X│X│X│X│X│1
    ┕━┷━┷━┷━┷━┷━┷━┷━┷━┙
      LSB ········· MSB
```

> While UART defines how to send data as a series of bits, it is standards like RS-232, RS-485, etc. that define the voltage levels that represent 0 and 1. Hence in the diagram I’ve written “0” and “1” instead of “high” and “low”. For common USB-to-UART adapters, 0 = low and 1 = high. Some adapter chips can be set to invert the levels, but this is uncommon.

The diagram above shows how one byte is transferred over a UART configured to use 8 data bits, no parity bit, and 1 stop bit (“8N1”), totalling 10 bits per packet. The data is sent LSB first. When sending multiple bytes in succession, there is usually no space between each packet – the stop bit of a packet is immediately followed by the start bit of the next packet:

```
 ┌────── byte 1 ─────┬────── byte 2 ─────┬────── byte 3 ─────┐
━┑ ┍━┯━┯━┯━┯━┯━┯━┯━┯━┑ ┍━┯━┯━┯━┯━┯━┯━┯━┯━┑ ┍━┯━┯━┯━┯━┯━┯━┯━┯━┑
 │0│X│X│X│X│X│X│X│X│1│0│X│X│X│X│X│X│X│X│1│0│X│X│X│X│X│X│X│X│1│  ···
 ┕━┷━┷━┷━┷━┷━┷━┷━┷━┙ ┕━┷━┷━┷━┷━┷━┷━┷━┷━┙ ┕━┷━┷━┷━┷━┷━┷━┷━┷━┙ ┕━
```

By changing the 8 data bits, a PWM effect can be achieved. To make a 50% duty cycle for example, the data byte `11110000` can be used.

```
 ┌────── byte 1 ─────┬────── byte 2 ─────┬────── byte 3 ─────┐
━┑         ┍━━━━━━━━━┑         ┍━━━━━━━━━┑         ┍━━━━━━━━━┑
 │0 0 0 0 0│1 1 1 1 1│0 0 0 0 0│1 1 1 1 1│0 0 0 0 0│1 1 1 1 1│  ···
 ┕━━━━━━━━━┙         ┕━━━━━━━━━┙         ┕━━━━━━━━━┙         ┕━
```

Alternatively, we could use the bit pattern `10101010`, giving a higher “switching frequency” and thus a smoother output (after low-pass filtering):

```
 ┌────── byte 1 ─────┬────── byte 2 ─────┬────── byte 3 ─────┐
━┑ ┍━┑ ┍━┑ ┍━┑ ┍━┑ ┍━┑ ┍━┑ ┍━┑ ┍━┑ ┍━┑ ┍━┑ ┍━┑ ┍━┑ ┍━┑ ┍━┑ ┍━┑
 │0│1│0│1│0│1│0│1│0│1│0│1│0│1│0│1│0│1│0│1│0│1│0│1│0│1│0│1│0│1│  ···
 ┕━┙ ┕━┙ ┕━┙ ┕━┙ ┕━┙ ┕━┙ ┕━┙ ┕━┙ ┕━┙ ┕━┙ ┕━┙ ┕━┙ ┕━┙ ┕━┙ ┕━┙ ┕━
```

Any other pattern with 4 zeros and 4 ones will in theory give the same average level.

The start and stop bits are a fixed 0 and 1 respectively and can’t be changed. This means the minimum duty cycle possible is 10%:

```
 ┌────── byte 1 ─────┬────── byte 2 ─────┬────── byte 3 ─────┐
━┑                 ┍━┑                 ┍━┑                 ┍━┑
 │0 0 0 0 0 0 0 0 0│1│0 0 0 0 0 0 0 0 0│1│0 0 0 0 0 0 0 0 0│1│  ···
 ┕━━━━━━━━━━━━━━━━━┙ ┕━━━━━━━━━━━━━━━━━┙ ┕━━━━━━━━━━━━━━━━━┙ ┕━
```

and the maximum is 90%:

```
 ┌────── byte 1 ─────┬────── byte 2 ─────┬────── byte 3 ─────┐
━┑ ┍━━━━━━━━━━━━━━━━━┑ ┍━━━━━━━━━━━━━━━━━┑ ┍━━━━━━━━━━━━━━━━━┑
 │0│1 1 1 1 1 1 1 1 1│0│1 1 1 1 1 1 1 1 1│0│1 1 1 1 1 1 1 1 1│  ···
 ┕━┙                 ┕━┙                 ┕━┙                 ┕━
```

With this technique we can now approximate an audio signal. This isn’t strictly PWM since bit patterns with different switching frequencies can be used as seen above, but the principle is the same. Obviously, the audio signal created in this way has a non-zero DC component (i.e. average value), and a capacitor is required between the TX pin and the audio output device to remove the DC bias.

> With a 5V / 3.3V UART and a pair of headphones, a 1k ~ 10k resistor between the TX pin and the headphones seems to work well enough.

Running the UART at 3M baud gives a “audio sample rate” of 300k: a bit too high compared to the common 44.1k or 48k. We could simply resample our music to 300k, convert each sample to one “PWM” byte, and feed it to the UART; however, with only 9 possible signal levels (compared to the 256 levels with normal 8-bit samples or 65536 with 16-bit samples), the result is rather unpleasant.
