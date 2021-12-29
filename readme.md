# Play music through your USB-to-UART adapter!

## Usage

- Resample audio file to 50k samples per second and convert to mono unsigned-8-bit WAV
- Run program to process WAV file
- Connect headphones / amplified speaker to UART TX through a current limiting resistor
- `cat` the resulting file to the UART port at 3M baud

## How it works

```
   Start              Stop
━━━━┑ ┍━┯━┯━┯━┯━┯━┯━┯━┯━━━━━
    │0│X│X│X│X│X│X│X│X│1
    ┕━┷━┷━┷━┷━┷━┷━┷━┷━┙
      LSB ......... MSB
```

> While UART defines how to send data as a series of bits, it is standards like RS-232, RS-485, etc. that define the voltage levels that represent 0 and 1. Hence in the diagram I’ve written “0” and “1” instead of “high” and “low”. For common USB-to-UART adapters, 0 = low and 1 = high. Some adapter chips allow inverting the voltage levels, but this is uncommon.

The diagram above shows how one byte is transferred over a UART configured to use 8 data bits, no parity bit, and 1 stop bit (“8N1”), totalling 10 bits per packet. The data is sent LSB first. When sending multiple bytes in succession, there is usually no space between each packet – the stop bit of a packet is immediately followed by the start bit of the next packet:

```
┄┬────── byte 1 ─────┬────── byte 2 ─────┬────── byte 3 ─────┬┄
┅┑ ┍━┯━┯━┯━┯━┯━┯━┯━┯━┑ ┍━┯━┯━┯━┯━┯━┯━┯━┯━┑ ┍━┯━┯━┯━┯━┯━┯━┯━┯━┑
 │0│X│X│X│X│X│X│X│X│1│0│X│X│X│X│X│X│X│X│1│0│X│X│X│X│X│X│X│X│1│
 ┕━┷━┷━┷━┷━┷━┷━┷━┷━┙ ┕━┷━┷━┷━┷━┷━┷━┷━┷━┙ ┕━┷━┷━┷━┷━┷━┷━┷━┷━┙ ┕┅
```

By changing the 8 data bits, a PWM effect can be achieved. To simulate a 50% signal level for example, the data byte `11110000` can be used.

```
┄┬────── byte 1 ─────┬────── byte 2 ─────┬────── byte 3 ─────┬┄
┅┑         ┍━━━━━━━━━┑         ┍━━━━━━━━━┑         ┍━━━━━━━━━┑
 │0 0 0 0 0│1 1 1 1 1│0 0 0 0 0│1 1 1 1 1│0 0 0 0 0│1 1 1 1 1│
 ┕━━━━━━━━━┙         ┕━━━━━━━━━┙         ┕━━━━━━━━━┙         ┕┅
```

Alternatively, we could use the bit pattern `10101010` with a higher switching frequency to give a smoother output (after low-pass filtering):

```
┄┬────── byte 1 ─────┬────── byte 2 ─────┬────── byte 3 ─────┬┄
┅┑ ┍━┑ ┍━┑ ┍━┑ ┍━┑ ┍━┑ ┍━┑ ┍━┑ ┍━┑ ┍━┑ ┍━┑ ┍━┑ ┍━┑ ┍━┑ ┍━┑ ┍━┑
 │0│1│0│1│0│1│0│1│0│1│0│1│0│1│0│1│0│1│0│1│0│1│0│1│0│1│0│1│0│1│
 ┕━┙ ┕━┙ ┕━┙ ┕━┙ ┕━┙ ┕━┙ ┕━┙ ┕━┙ ┕━┙ ┕━┙ ┕━┙ ┕━┙ ┕━┙ ┕━┙ ┕━┙ ┕┅
```

Any other pattern with 4 zeros and 4 ones will in theory give the same average level.

The start and stop bits are a fixed 0 and 1 respectively and can’t be changed. This means the minimum level possible is 10%:

```
┄┬────── byte 1 ─────┬────── byte 2 ─────┬────── byte 3 ─────┬┄
┅┑                 ┍━┑                 ┍━┑                 ┍━┑
 │0 0 0 0 0 0 0 0 0│1│0 0 0 0 0 0 0 0 0│1│0 0 0 0 0 0 0 0 0│1│
 ┕━━━━━━━━━━━━━━━━━┙ ┕━━━━━━━━━━━━━━━━━┙ ┕━━━━━━━━━━━━━━━━━┙ ┕┅
```

And the maximum possible is 90%:

```
┄┬────── byte 1 ─────┬────── byte 2 ─────┬────── byte 3 ─────┬┄
┅┑ ┍━━━━━━━━━━━━━━━━━┑ ┍━━━━━━━━━━━━━━━━━┑ ┍━━━━━━━━━━━━━━━━━┑
 │0│1 1 1 1 1 1 1 1 1│0│1 1 1 1 1 1 1 1 1│0│1 1 1 1 1 1 1 1 1│
 ┕━┙                 ┕━┙                 ┕━┙                 ┕┅
```

