#include <Arduino.h>
#include <math.h>
#include <driver/i2s.h>
#include <mbedtls/base64.h>
#include <SPIFFS.h>

#define I2S_DOUT 6  // Data
#define I2S_BCLK 5   // Bit Clock
#define I2S_LRC  4   // Left/Right Clock

#define DEFAULT_SAMPLE_RATE 16000  // Changed to match your WAV files
#define BUFFER_SIZE         256    // Number of samples per buffer
#define MAX_AUDIO_BUFFER    105000 // Maximum decoded audio buffer size
#define DEFAULT_VOLUME_GAIN 2.0f   // Default volume gain (2x amplification)

// Current I2S sample rate (can be changed dynamically)
uint32_t current_sample_rate = DEFAULT_SAMPLE_RATE;
float volume_gain = DEFAULT_VOLUME_GAIN;

void setupI2S(uint32_t sample_rate = DEFAULT_SAMPLE_RATE) {
  // Stop I2S if it's already running
  i2s_driver_uninstall(I2S_NUM_0);
  
  // Add delay to ensure proper shutdown
  delay(100);
  
  // Configure I2S peripheral with specified sample rate
  const i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = sample_rate,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  esp_err_t result = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  if (result != ESP_OK) {
    Serial.print("I2S driver install failed: ");
    Serial.println(result);
    return;
  }
  
  result = i2s_set_pin(I2S_NUM_0, &pin_config);
  if (result != ESP_OK) {
    Serial.print("I2S pin config failed: ");
    Serial.println(result);
    return;
  }
  
  current_sample_rate = sample_rate;
  Serial.print("✓ I2S successfully configured for sample rate: ");
  Serial.print(sample_rate);
  Serial.println(" Hz");
}

// Base64 decode function
bool decodeBase64Audio(const char* base64_input, uint8_t* audio_buffer, size_t* audio_length) {
  size_t input_len = strlen(base64_input);
  size_t output_len;
  
  Serial.print("Input base64 length: ");
  Serial.println(input_len);
  
  // Decode base64 to binary audio data
  int ret = mbedtls_base64_decode(audio_buffer, MAX_AUDIO_BUFFER, &output_len, 
                                  (const unsigned char*)base64_input, input_len);
  
  if (ret == 0) {
    *audio_length = output_len;
    Serial.print("Decoded ");
    Serial.print(output_len);
    Serial.println(" bytes of PCM 16-bit audio data");
    return true;
  } else {
    Serial.print("Base64 decode failed with error code: ");
    Serial.println(ret);
    if (ret == MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL) {
      Serial.println("Output buffer too small for decoded data");
    }
    return false;
  }
}

// Apply volume gain to audio samples with clipping protection
void applyVolumeGain(int16_t* samples, size_t sample_count, float gain) {
  for (size_t i = 0; i < sample_count; i++) {
    int32_t amplified = (int32_t)(samples[i] * gain);
    
    // Clip to prevent distortion
    if (amplified > 32767) {
      samples[i] = 32767;
    } else if (amplified < -32768) {
      samples[i] = -32768;
    } else {
      samples[i] = (int16_t)amplified;
    }
  }
}

// New function to handle mono audio data properly
void playMonoAudioData(uint8_t* audio_data, size_t length) {
  // Validate that we have valid PCM 16-bit data
  if (length < 2) {
    Serial.println("Error: Audio data too short for 16-bit PCM");
    return;
  }
  
  int16_t* mono_samples = (int16_t*)audio_data;
  size_t mono_sample_count = length / 2; // 16-bit samples (2 bytes per sample)
  
  Serial.print("Converting ");
  Serial.print(mono_sample_count);
  Serial.println(" mono samples to stereo");
  
  // Create stereo buffer (double the size)
  size_t stereo_length = mono_sample_count * 4; // 2 channels * 2 bytes per sample
  int16_t* stereo_buffer = (int16_t*)malloc(stereo_length);
  
  if (stereo_buffer == NULL) {
    Serial.println("❌ Failed to allocate stereo buffer");
    return;
  }
  
  // Convert mono to stereo by duplicating each sample
  for (size_t i = 0; i < mono_sample_count; i++) {
    stereo_buffer[2 * i] = mono_samples[i];     // Left channel
    stereo_buffer[2 * i + 1] = mono_samples[i]; // Right channel (duplicate)
  }
  
  // Apply volume gain
  Serial.print("Applying volume gain: ");
  Serial.println(volume_gain);
  applyVolumeGain(stereo_buffer, mono_sample_count * 2, volume_gain);
  
  size_t bytes_written;
  i2s_write(I2S_NUM_0, stereo_buffer, stereo_length, &bytes_written, portMAX_DELAY);
  
  Serial.print("Successfully wrote ");
  Serial.print(bytes_written);
  Serial.println(" bytes to I2S (stereo converted)");
  
  free(stereo_buffer);
}

// Enhanced playAudioData function with better stereo detection
void playAudioData(uint8_t* audio_data, size_t length) {
  // Validate that we have valid PCM 16-bit data
  if (length < 2) {
    Serial.println("Error: Audio data too short for 16-bit PCM");
    return;
  }
  
  // Convert to 16-bit samples and play
  int16_t* samples = (int16_t*)audio_data;
  size_t sample_count = length / 2; // 16-bit samples (2 bytes per sample)
  
  Serial.print("Processing ");
  Serial.print(sample_count);
  Serial.println(" 16-bit PCM samples as stereo");
  
  // Apply volume gain
  Serial.print("Applying volume gain: ");
  Serial.println(volume_gain);
  applyVolumeGain(samples, sample_count, volume_gain);
  
  size_t bytes_written;
  i2s_write(I2S_NUM_0, samples, length, &bytes_written, portMAX_DELAY);
  
  Serial.print("Successfully wrote ");
  Serial.print(bytes_written);
  Serial.println(" bytes to I2S");
}

// List files in SPIFFS
void listSPIFFSFiles() {
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  
  Serial.println("Files in SPIFFS:");
  while (file) {
    Serial.print("  ");
    Serial.print(file.name());
    Serial.print(" (");
    Serial.print(file.size());
    Serial.println(" bytes)");
    file = root.openNextFile();
  }
  Serial.println("End of file list");
}

// Enhanced WAV header parsing
struct WAVHeader {
  uint32_t sample_rate;
  uint16_t num_channels;
  uint16_t bits_per_sample;
  uint32_t data_size;
  bool is_valid;
};

WAVHeader parseWAVHeader(uint8_t* header) {
  WAVHeader wav = {0};
  
  // Check RIFF signature
  if (header[0] != 'R' || header[1] != 'I' || header[2] != 'F' || header[3] != 'F') {
    Serial.println("Error: Not a valid RIFF file");
    wav.is_valid = false;
    return wav;
  }
  
  // Check WAVE signature
  if (header[8] != 'W' || header[9] != 'A' || header[10] != 'V' || header[11] != 'E') {
    Serial.println("Error: Not a valid WAVE file");
    wav.is_valid = false;
    return wav;
  }
  
  // Extract audio parameters (little-endian format)
  wav.sample_rate = header[24] | (header[25] << 8) | (header[26] << 16) | (header[27] << 24);
  wav.num_channels = header[22] | (header[23] << 8);
  wav.bits_per_sample = header[34] | (header[35] << 8);
  wav.data_size = header[40] | (header[41] << 8) | (header[42] << 16) | (header[43] << 24);
  wav.is_valid = true;
  
  // Enhanced debug output
  Serial.println("📊 WAV Header Analysis:");
  Serial.print("  - Sample Rate: "); Serial.print(wav.sample_rate); Serial.println(" Hz");
  Serial.print("  - Channels: "); Serial.print(wav.num_channels); 
  if (wav.num_channels == 1) Serial.println(" (MONO)");
  else if (wav.num_channels == 2) Serial.println(" (STEREO)");
  else Serial.println(" (UNKNOWN)");
  Serial.print("  - Bits per Sample: "); Serial.println(wav.bits_per_sample);
  Serial.print("  - Data Size: "); Serial.print(wav.data_size); Serial.println(" bytes");
  
  // Calculate expected duration
  uint32_t total_samples = wav.data_size / (wav.bits_per_sample / 8) / wav.num_channels;
  float duration = (float)total_samples / wav.sample_rate;
  Serial.print("  - Expected Duration: "); Serial.print(duration); Serial.println(" seconds");
  
  return wav;
}

// Enhanced playAudioFileFromSPIFFS with better sample rate and channel handling
bool playAudioFileFromSPIFFS(const char* filename) {
  if (!SPIFFS.exists(filename)) {
    Serial.print("❌ File not found: ");
    Serial.println(filename);
    return false;
  }
  
  File file = SPIFFS.open(filename, "r");
  if (!file) {
    Serial.print("❌ Failed to open file: ");
    Serial.println(filename);
    return false;
  }
  
  size_t file_size = file.size();
  Serial.print("\n🎵 Playing file: ");
  Serial.print(filename);
  Serial.print(" (");
  Serial.print(file_size);
  Serial.println(" bytes)");
  
  String filename_str = String(filename);
  filename_str.toLowerCase();
  
  if (filename_str.endsWith(".wav")) {
    // Handle WAV files - Enhanced sample rate handling
    Serial.println("🎼 Processing WAV file...");
    
    // Read WAV header (44 bytes for standard WAV)
    uint8_t wav_header[44];
    if (file.readBytes((char*)wav_header, 44) != 44) {
      Serial.println("❌ Failed to read WAV header");
      file.close();
      return false;
    }
    
    // Parse WAV header
    WAVHeader wav_info = parseWAVHeader(wav_header);
    
    if (!wav_info.is_valid) {
      Serial.println("❌ Invalid WAV file format");
      file.close();
      return false;
    }
    
    // ALWAYS reconfigure I2S for the WAV file's sample rate
    Serial.print("🔧 Configuring I2S for WAV sample rate: ");
    Serial.print(wav_info.sample_rate);
    Serial.println(" Hz");
    setupI2S(wav_info.sample_rate);
    
    // Small delay to ensure I2S is ready
    delay(100);
    
    // Validate format
    if (wav_info.bits_per_sample != 16) {
      Serial.print("❌ Unsupported bit depth: ");
      Serial.print(wav_info.bits_per_sample);
      Serial.println(" (only 16-bit supported)");
      file.close();
      return false;
    }
    
    // Get audio data size (file size - header size)
    size_t audio_data_size = file_size - 44;
    
    uint8_t* audio_buffer = (uint8_t*)malloc(audio_data_size);
    if (audio_buffer == NULL) {
      Serial.println("❌ Failed to allocate audio buffer for WAV");
      file.close();
      return false;
    }
    
    size_t bytes_read = file.readBytes((char*)audio_buffer, audio_data_size);
    file.close();
    
    if (bytes_read != audio_data_size) {
      Serial.println("❌ Failed to read complete audio data");
      free(audio_buffer);
      return false;
    }
    
    Serial.println("🔊 Playing WAV PCM data at correct sample rate");
    
    // Handle mono vs stereo properly
    if (wav_info.num_channels == 1) {
      Serial.println("📻 Converting mono WAV to stereo for I2S");
      playMonoAudioData(audio_buffer, audio_data_size);
    } else if (wav_info.num_channels == 2) {
      Serial.println("🎵 Playing stereo WAV data");
      playAudioData(audio_buffer, audio_data_size);
    } else {
      Serial.print("❌ Unsupported channel count: ");
      Serial.println(wav_info.num_channels);
      free(audio_buffer);
      return false;
    }
    
    free(audio_buffer);
    return true;
  } 
  else if (filename_str.endsWith(".b64")) {
    // Handle Base64 encoded files
    Serial.println("Processing Base64 file...");
    char* base64_buffer = (char*)malloc(file_size + 1);
    if (base64_buffer == NULL) {
      Serial.println("Failed to allocate buffer for base64 file");
      file.close();
      return false;
    }
    
    file.readBytes(base64_buffer, file_size);
    base64_buffer[file_size] = '\0';
    file.close();
    
    // Decode and play
    uint8_t* audio_buffer = (uint8_t*)malloc(MAX_AUDIO_BUFFER);
    if (audio_buffer == NULL) {
      Serial.println("Failed to allocate audio buffer");
      free(base64_buffer);
      return false;
    }
    
    size_t audio_length;
    if (decodeBase64Audio(base64_buffer, audio_buffer, &audio_length)) {
      playAudioData(audio_buffer, audio_length);
      free(audio_buffer);
      free(base64_buffer);
      return true;
    } else {
      free(audio_buffer);
      free(base64_buffer);
      return false;
    }
  } 
  else {
    // Handle RAW PCM files (.raw, .pcm, or any other extension)
    Serial.println("Processing RAW PCM file...");
    
    // For raw files, assume they match current I2S sample rate
    // You might want to reset to default sample rate for raw files
    if (current_sample_rate != DEFAULT_SAMPLE_RATE) {
      Serial.print("Resetting I2S to default sample rate: ");
      Serial.println(DEFAULT_SAMPLE_RATE);
      setupI2S(DEFAULT_SAMPLE_RATE);
    }
    
    uint8_t* audio_buffer = (uint8_t*)malloc(file_size);
    if (audio_buffer == NULL) {
      Serial.println("Failed to allocate audio buffer");
      file.close();
      return false;
    }
    
    file.readBytes((char*)audio_buffer, file_size);
    file.close();
    
    Serial.println("Playing raw PCM data from file");
    playAudioData(audio_buffer, file_size);
    
    free(audio_buffer);
    return true;
  }
}

// Auto-play all audio files in SPIFFS
void playAllAudioFiles() {
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  
  Serial.println("Playing all audio files in sequence...");
  
  while (file) {
    String filename = file.name();
    file = root.openNextFile(); // Get next file before playing current one
    
    // Ensure filename starts with "/" for SPIFFS
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }
    
    // Check if it's an audio file
    if (filename.endsWith(".wav") || filename.endsWith(".b64") || 
        filename.endsWith(".pcm") || filename.endsWith(".raw")) {
      
      Serial.print("Playing: ");
      Serial.println(filename);
      
      playAudioFileFromSPIFFS(filename.c_str());
      
      delay(500); // Small pause between files
    }
  }
  
  Serial.println("Finished playing all audio files");
}

// Process serial commands
void processSerialCommand(String command) {
  command.trim();
  command.toLowerCase();
  
  if (command == "list" || command == "ls") {
    listSPIFFSFiles();
  } else if (command.startsWith("play ")) {
    String filename = command.substring(5);
    filename.trim();
    if (filename.length() > 0) {
      if (!filename.startsWith("/")) {
        filename = "/" + filename;
      }
      playAudioFileFromSPIFFS(filename.c_str());
    } else {
      Serial.println("Usage: play <filename>");
    }
  } else if (command == "playall") {
    playAllAudioFiles();
  } else if (command.startsWith("volume ")) {
    String volume_str = command.substring(7);
    volume_str.trim();
    float new_volume = volume_str.toFloat();
    if (new_volume > 0 && new_volume <= 10.0) {
      volume_gain = new_volume;
      Serial.print("Volume set to: ");
      Serial.println(volume_gain);
    } else {
      Serial.println("Volume must be between 0.1 and 10.0");
      Serial.print("Current volume: ");
      Serial.println(volume_gain);
    }
  } else if (command == "volume") {
    Serial.print("Current volume gain: ");
    Serial.println(volume_gain);
  } else if (command == "help") {
    Serial.println("Available commands:");
    Serial.println("  list or ls - List files in SPIFFS");
    Serial.println("  play <filename> - Play specific audio file");
    Serial.println("  playall - Play all audio files in sequence");
    Serial.println("  volume <gain> - Set volume gain (0.1 to 10.0)");
    Serial.println("  volume - Show current volume");
    Serial.println("  help - Show this help");
  } else {
    Serial.println("Unknown command. Type 'help' for available commands.");
  }
}

void setup() {
  Serial.begin(115200);
  
  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  
  setupI2S();
  
  Serial.println("ESP32 Audio Player Ready");
  Serial.println("=========================");
  Serial.println("Supported file formats:");
  Serial.println("  .wav - WAV audio files (16-bit PCM)");
  Serial.println("  .b64 - Base64 encoded PCM audio");
  Serial.println("  .pcm/.raw - Raw PCM 16-bit audio");
  Serial.println("");
  Serial.println("Commands:");
  Serial.println("  list - List files in SPIFFS");
  Serial.println("  play <filename> - Play specific audio file");
  Serial.println("  playall - Play all audio files in sequence");
  Serial.println("  volume <gain> - Set volume (0.1 to 10.0)");
  Serial.println("  volume - Show current volume");
  Serial.println("  help - Show help");
  Serial.print("Current volume gain: ");
  Serial.println(volume_gain);
  Serial.println("=========================");
  
  // Show available files
  listSPIFFSFiles();
  
  // Auto-play all audio files on startup
  Serial.println("");
  Serial.println("Auto-playing all audio files...");
  delay(2000); // 2 second delay before starting
  playAllAudioFiles();
}

void loop() {
  // Check if serial data is available for commands only
  if (Serial.available()) {
    String incoming = Serial.readStringUntil('\n');
    incoming.trim();
    
    if (incoming.length() > 0) {
      processSerialCommand(incoming);
    }
  }
  
  delay(100); // Small delay to prevent excessive CPU usage
}
