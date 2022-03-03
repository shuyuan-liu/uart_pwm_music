# Play (fairly good quality) music through your USB-to-UART adaptor!

## Usage

In the “samples” folder there are ready-made files that can be fed directly to a serial port. For a quick test, take the file at the appropriate speed, and start from step 6 below.

All filenames in the commands below are examples only and do not affect how this program functions. These instructions, especially feeding the resulting file to the serial port, assume a Linux machine.

1. Compile this program, using for example `cc -O2 main.c -o uart_pwm`

2. Find your UART adaptor's maximum symbol rate. Common ones are:

	| Chip   | Max baud | Stable baud (less noise) |
	| ------ | -------- | ------------------------ |
	| FT232R | 3M       | ==TODO==                 |
	| CH340  | 2M       | ==TODO==                 |

3. Calculate audio sample rate = symbol rate / 10 / 8.

4. Convert your audio file to a mono WAV at this sample rate. Examples:
      - With FFmpeg: `ffmpeg -i input.mp3 -ar <sample_rate> -ac 1 resampled.wav`
    
      - With SoX (supports mostly uncompressed formats only): `sox input.flac -r <sample_rate> -c 1 resampled.wav`

5. Run this program: `./uart_pwm resampled.wav output.bin`

6. Set your serial port to the desired speed, and disable any text processing (“raw”): `stty -F /dev/ttyUSB0 raw ospeed <baud> ispeed <baud>`

	> I’m setting `ispeed` too because if I set only `ospeed` the command fails the first time and needs to be run again to succeed, for some reason that I haven’t investigated.
	
7. Connect speakers / headphones to the TX and ground of your UART adaptor through a current-limiting resistor (a few kΩ).

8. Feed the output file to the serial port: `cat output.bin > /dev/ttyUSB0`.

9. Enjoy the music!


## Outputting a PWM Signal

```
   Start              Stop
━━━━┑ ┍━┯━┯━┯━┯━┯━┯━┯━┯━━━━━
    │0│X│X│X│X│X│X│X│X│1
    ┕━┷━┷━┷━┷━┷━┷━┷━┷━┙
      LSB ········· MSB
```

> While UART defines how to send data as a series of bits, it is standards like RS-232, RS-485, etc. that define the voltage levels that represent 0 and 1. Hence in the diagram I’ve written “0” and “1” instead of “high” and “low”. For common USB-to-UART adaptors, 0 = low and 1 = high. Some adaptor chips can be set to invert the levels, but this is uncommon.

The diagram above shows one byte transferred over a UART configured with 8 data bits, no parity bit, and 1 stop bit (“8N1”), totalling 10 bits per byte of data sent. This is a very common configuration and usually the default. When sending multiple bytes in succession, there is usually no space between packets – the stop bit of one packet is immediately followed by the start bit of the next packet:

```
 ┌────── byte 1 ─────┬────── byte 2 ─────┬────── byte 3 ─────┐
━┑ ┍━┯━┯━┯━┯━┯━┯━┯━┯━┑ ┍━┯━┯━┯━┯━┯━┯━┯━┯━┑ ┍━┯━┯━┯━┯━┯━┯━┯━┯━┑
 │0│X│X│X│X│X│X│X│X│1│0│X│X│X│X│X│X│X│X│1│0│X│X│X│X│X│X│X│X│1│  ···
 ┕━┷━┷━┷━┷━┷━┷━┷━┷━┙ ┕━┷━┷━┷━┷━┷━┷━┷━┷━┙ ┕━┷━┷━┷━┷━┷━┷━┷━┷━┙ ┕━
```

> Unfortunately for us, some chips can pause briefly between bytes when running at high speeds. While this doesn’t violate the UART protocol, it causes additional noise and slowdown in the audio.

By changing the 8 data bits, a PWM effect can be achieved. To make a 50% duty cycle for example, the data byte `11110000` can be used. Note that data is sent LSB first over UART, thus the reversed bits in each byte.

```
 ┌────── byte 1 ─────┬────── byte 2 ─────┬────── byte 3 ─────┐
━┑         ┍━━━━━━━━━┑         ┍━━━━━━━━━┑         ┍━━━━━━━━━┑
 │0 0 0 0 0│1 1 1 1 1│0 0 0 0 0│1 1 1 1 1│0 0 0 0 0│1 1 1 1 1│  ···
 ┕━━━━━━━━━┙         ┕━━━━━━━━━┙         ┕━━━━━━━━━┙         ┕━
```

Alternatively, we could use the bit pattern `01010101`, giving a higher switching frequency and thus a smoother output (after low-pass filtering):

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

> Being unable to go to 0% and 100% should have little effect, as it just linearly shrinks the output range to 10% — 90%.

## Byte Patterns

==TODO==
