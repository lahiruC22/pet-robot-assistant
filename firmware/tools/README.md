# Audio Conversion Tools

This directory contains tools to convert base64 audio chunks from the ESP32 serial output into WAV files.

## How to Use

### 1. Capture Serial Output
When your ESP32 is running and receiving audio from ElevenLabs, save the serial monitor output to a text file:

```bash
# Using Arduino IDE: Copy serial monitor output to a text file
# Using PlatformIO: 
pio device monitor > serial_output.txt
```

### 2. Convert to WAV
Run the Python conversion script:

```bash
python tools/base64_to_wav.py serial_output.txt output.wav
```

### 3. Optional Parameters
```bash
# Specify different sample rate (if agent configuration differs)
python tools/base64_to_wav.py serial_output.txt output.wav --sample-rate 16000

# Specify stereo audio (uncommon for voice)
python tools/base64_to_wav.py serial_output.txt output.wav --channels 2
```

## Audio Format Details

The ElevenLabs Conversational AI sends audio in the following format:
- **Sample Rate**: 24000 Hz (as defined in `SPEAKER_SAMPLE_RATE` in config.h)
- **Encoding**: 16-bit PCM (as defined in `SPEAKER_ENCODING` in config.h)
- **Channels**: 1 (Mono)
- **Format**: Base64-encoded audio chunks

## Example Serial Output

Look for lines like this in your serial output:
```
[AUDIO_BASE64] UklGRiQBAABXQVZFZm10IBAAAAABAAABAGH...
[AUDIO_BASE64] kBdWF2ZYJhpmGiYcJh4mICYiJiQmJiYoJio...
[AUDIO_BASE64] mLCYuJjAmMiY0JjYmOCY6JjwmPiZAJkIm...
```

## Dependencies

The conversion script requires Python 3 with the `wave` module (included in standard library).

## Troubleshooting

1. **No audio chunks found**: Make sure your ESP32 is receiving audio responses from ElevenLabs
2. **Base64 decode error**: Check that the serial output is clean and properly formatted
3. **Short audio duration**: The agent might be sending very brief responses
4. **Distorted audio**: Verify the sample rate matches your agent's configuration

## Agent Audio Configuration

In your ElevenLabs agent settings, make sure the audio output format matches:
- Output format: PCM 16-bit
- Sample rate: 24000 Hz (or update config.h to match your agent)
