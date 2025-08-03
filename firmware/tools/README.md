# Audio Conversion Tools

This directory contains tools to convert base64 audio chunks from the ESP32 serial output into WAV files for analysis and verification.

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

### ElevenLabs Conversational AI Audio

**Input (Microphone to ElevenLabs):**
- **Sample Rate**: 16000 Hz (as defined in `MIC_SAMPLE_RATE` in config.h)
- **Encoding**: 16-bit PCM (as defined in `MIC_ENCODING` in config.h)
- **Channels**: 1 (Mono)
- **Format**: Base64-encoded audio chunks

**Output (ElevenLabs to Speaker):**
- **Sample Rate**: 24000 Hz (as defined in `SPEAKER_SAMPLE_RATE` in config.h)
- **Encoding**: 16-bit PCM (as defined in `SPEAKER_ENCODING` in config.h)
- **Channels**: 1 (Mono)
- **Format**: Base64-encoded audio chunks

## Example Serial Output

### Recording Audio (Input)
Look for lines like this in your serial output when microphone is recording:
```
[MIC] Recording started successfully!
[MIC] Recorded 1/3 seconds
[MIC] Recording complete! Duration: 3047 ms
=== Base64 Audio Data ===
UklGRiQBAABXQVZFZm10IBAAAAABAAABAGH...
=== End Base64 Data ===
```

### Receiving Audio (Output)
Look for lines like this when receiving ElevenLabs responses:
```
[AUDIO] Received audio response (Event ID: 12345)
[AUDIO] Base64 audio length: 234567 characters
[AUDIO_BASE64] UklGRiQBAABXQVZFZm10IBAAAAABAAABAGH...
```

## Dependencies

The conversion script requires Python 3 with the `wave` module (included in standard library).

## Troubleshooting

### No Audio Chunks Found

1. **For Recording Audio:**
   - Make sure your ESP32 is successfully recording
   - Check that the microphone is properly connected
   - Verify that `[MIC]` log messages appear in serial output

2. **For Response Audio:**
   - Ensure WebSocket connection to ElevenLabs is successful
   - Check that `[AUDIO_BASE64]` lines appear in serial output
   - Verify your agent is configured to send audio responses

### Base64 Decode Errors

1. **Corrupted Data**: Serial communication issues can corrupt base64 data
2. **Incomplete Capture**: Make sure to capture the complete base64 string
3. **Wrong Format**: Verify the audio format matches expectations

### Audio Quality Issues

1. **Sample Rate Mismatch**: Use `--sample-rate` parameter to match your configuration
2. **Volume Issues**: 
   - Recording too quiet: Check microphone gain in microphone.cpp
   - Playback too loud/quiet: Check speaker volume settings
3. **Distortion**: May indicate clipping or hardware issues

## Agent Audio Configuration

Make sure your ElevenLabs agent settings match your ESP32 configuration:

### For Input Audio (Microphone)
- Input format: PCM 16-bit
- Sample rate: 16000 Hz (matches `MIC_SAMPLE_RATE`)

### For Output Audio (Speaker)  
- Output format: PCM 16-bit
- Sample rate: 24000 Hz (matches `SPEAKER_SAMPLE_RATE`)

## Advanced Usage

### Batch Conversion
Convert multiple serial outputs:
```bash
for file in serial_output_*.txt; do
    python tools/base64_to_wav.py "$file" "${file%.txt}.wav"
done
```

### Audio Analysis
Use the converted WAV files with audio analysis tools:
```bash
# View audio properties
ffprobe output.wav

# Convert to other formats
ffmpeg -i output.wav output.mp3

# Analyze with Audacity, etc.
```

### Real-time Monitoring
Monitor and convert audio in real-time:
```bash
# Terminal 1: Monitor and save output
pio device monitor | tee serial_output.txt

# Terminal 2: Convert as data comes in
tail -f serial_output.txt | python tools/base64_to_wav.py --stdin output.wav
```

## Testing Audio Quality

### Recording Quality Test
1. Record a known phrase ("The quick brown fox...")
2. Convert to WAV using this tool
3. Play back and verify clarity
4. Adjust microphone gain if needed

### Playback Quality Test
1. Send known text to ElevenLabs agent
2. Capture the audio response
3. Convert to WAV using this tool
4. Compare with direct ElevenLabs output
5. Adjust speaker volume/settings if needed

### Latency Analysis
1. Record timestamp when audio is sent
2. Record timestamp when audio playback starts
3. Measure total round-trip time
4. Optimize network/processing as needed

---

**Compatible with:** ESP32-S3, ElevenLabs Conversational AI v1  
**Last Updated:** August 2, 2025  
**Python Version:** 3.6+
