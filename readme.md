# Play (fairly good quality) music through your USB-to-UART adapter!

## Usage

All filenames in the commands below are examples only and do not affect how this program functions. These instructions, especially feeding the resulting file to the serial port, assume a Linux machine.

- Compile this program, using for example `cc -O2 main.c -o uart_pwm`

- Find your UART adaptor's maximum symbol rate. Common ones are:

	| Chip   | Max baud | Stable baud (less noise) |
	| ------ | -------- | ------------------------ |
	| FT232R | 3M       | ==TODO==                 |
	| CH340  | 2M       | ==TODO==                 |

- Calculate audio sample rate = symbol rate / 10 / 8.

- Convert audio file to an unsigned 8-bit mono WAV at this sample rate. Examples:
  - With FFmpeg: `ffmpeg -i input.mp3 -ar <sample_rate> -ac 1 -c:a pcm_u8 resampled.wav`
  
  - With SoX: `sox input.flac -r <sample_rate> -c 1 -e unsigned-integer -b 8 resampled.wav`
  
    > From a few tests I found SoX to give better sound quality than FFmpeg when converting to 8-bit. SoX can’t directly handle compressed audio formats like MP3, so they need to go through FFmpeg first:
  	>
  	> `ffmpeg -i original.mp3 -c:a pcm_f32le intermediate.wav` at the original sample rate and with high precision (float samples), and then
  	>
  	> `sox intermediate.wav -r <sample_rate> -c 1 -e unsigned-integer -b 8 resampled.wav` ready for processing.
  	
  
- Run this program: `./uart_pwm resampled.wav output.bin`

- Set your serial port to the desired speed, and disable any text processing (“raw”): `stty -F /dev/ttyUSB0 raw ospeed <baud> ispeed <baud>`

	> I’m setting `ispeed` too because if I set only `ospeed`, the command fails the first time and needs to be run again to correctly set the speed.
	
- Connect speakers / headphones to the TX and ground of your UART adapter through a current-limiting resistor (a few kΩ).

- Feed the output file to the serial port: `cat output.bin > /dev/ttyUSB0`.
- Enjoy the music!


## Outputting a PWM signal

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

## Byte patterns

==TODO==
