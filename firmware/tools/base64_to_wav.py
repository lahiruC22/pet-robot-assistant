#!/usr/bin/env python3
"""
Convert base64 audio chunks from ESP32 serial output to WAV file.

Usage:
1. Run your ESP32 and save serial output to a text file
2. Run this script: python base64_to_wav.py serial_output.txt output.wav
3. The script will extract all [AUDIO_BASE64] lines and combine them into a WAV file

Note: This assumes the audio format matches the ElevenLabs agent configuration:
- Sample Rate: 24000 Hz (from SPEAKER_SAMPLE_RATE in config.h)
- Format: 16-bit PCM
- Channels: 1 (Mono)
"""

import base64
import wave
import argparse
import re
import sys

def extract_audio_base64_from_serial(file_path):
    """Extract all [AUDIO_BASE64] lines from serial output."""
    audio_chunks = []
    
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            for line in f:
                # Look for lines that start with [AUDIO_BASE64]
                match = re.match(r'\[AUDIO_BASE64\]\s*(.+)', line.strip())
                if match:
                    base64_data = match.group(1).strip()
                    audio_chunks.append(base64_data)
                    print(f"Found audio chunk: {len(base64_data)} characters")
    
    except FileNotFoundError:
        print(f"Error: File '{file_path}' not found")
        return None
    except Exception as e:
        print(f"Error reading file: {e}")
        return None
    
    if not audio_chunks:
        print("No [AUDIO_BASE64] lines found in the file")
        return None
    
    print(f"Total audio chunks found: {len(audio_chunks)}")
    return audio_chunks

def combine_base64_chunks(chunks):
    """Combine all base64 chunks into one string and decode to binary."""
    combined_base64 = ''.join(chunks)
    print(f"Combined base64 length: {len(combined_base64)} characters")
    
    try:
        audio_data = base64.b64decode(combined_base64)
        print(f"Decoded audio data: {len(audio_data)} bytes")
        return audio_data
    except Exception as e:
        print(f"Error decoding base64: {e}")
        return None

def create_wav_file(audio_data, output_path, sample_rate=24000, channels=1, sample_width=2):
    """Create WAV file from raw audio data."""
    try:
        with wave.open(output_path, 'wb') as wav_file:
            wav_file.setnchannels(channels)
            wav_file.setsampwidth(sample_width)  # 2 bytes = 16-bit
            wav_file.setframerate(sample_rate)
            wav_file.writeframes(audio_data)
        
        print(f"WAV file created: {output_path}")
        print(f"Duration: {len(audio_data) / (sample_rate * channels * sample_width):.2f} seconds")
        return True
    
    except Exception as e:
        print(f"Error creating WAV file: {e}")
        return False

def main():
    parser = argparse.ArgumentParser(description='Convert ESP32 serial output to WAV file')
    parser.add_argument('input_file', help='Text file containing serial output')
    parser.add_argument('output_file', help='Output WAV file path')
    parser.add_argument('--sample-rate', type=int, default=24000, 
                       help='Sample rate in Hz (default: 24000)')
    parser.add_argument('--channels', type=int, default=1,
                       help='Number of channels (default: 1)')
    
    args = parser.parse_args()
    
    # Extract audio chunks from serial output
    audio_chunks = extract_audio_base64_from_serial(args.input_file)
    if not audio_chunks:
        sys.exit(1)
    
    # Combine and decode chunks
    audio_data = combine_base64_chunks(audio_chunks)
    if not audio_data:
        sys.exit(1)
    
    # Create WAV file
    if create_wav_file(audio_data, args.output_file, args.sample_rate, args.channels):
        print("Conversion completed successfully!")
    else:
        sys.exit(1)

if __name__ == "__main__":
    main()
