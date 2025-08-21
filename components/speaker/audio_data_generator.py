import sys
import os
import struct

def parse_wav(wav_file_path):
    with open(wav_file_path, "rb") as f:
        riff_id = f.read(4)
        if riff_id != b'RIFF':
            raise ValueError("Not a valid RIFF file")

        f.read(4)

        wave_id = f.read(4)
        if wave_id != b'WAVE':
            raise ValueError("Not a valid WAVE file.")
        
        audio_format = 0
        num_channels = 0
        sample_rate = 0
        bits_per_sample = 0
        
        fmt_found = False
        while not fmt_found:
            chunk_id = f.read(4)
            if len(chunk_id) < 4:
                raise ValueError("Required 'fmt ' chunk not found or file ended unexpectedly.")
            
            chunk_size_bytes = f.read(4)
            if len(chunk_size_bytes) < 4:
                raise ValueError("File ended unexpectedly while reading chunk size.")
            chunk_size = struct.unpack('<I', chunk_size_bytes)[0]

            if chunk_id == b'fmt ':
                fmt_found = True
                
                audio_format = struct.unpack('<H', f.read(2))[0]
                num_channels = struct.unpack('<H', f.read(2))[0]
                sample_rate = struct.unpack('<I', f.read(4))[0]
                f.read(4)
                f.read(2) 
                bits_per_sample = struct.unpack('<H', f.read(2))[0]

                if audio_format != 1:
                    raise ValueError(f"Unsupported audio format: {audio_format}. Only PCM (1) is supported.")
                if bits_per_sample not in [8, 16]: 
                    raise ValueError(f"Unsupported bits per sample: {bits_per_sample}. Only 8 or 16 are supported.")
                
                if chunk_size > 16:
                    f.read(chunk_size - 16)
            else:
                f.seek(chunk_size, 1)

        data_found = False
        raw_audio_data = b''
        while not data_found:
            chunk_id = f.read(4)
            if len(chunk_id) < 4:
                raise ValueError("Required 'data' chunk not found or file ended unexpectedly.")

            chunk_size_bytes = f.read(4)
            if len(chunk_size_bytes) < 4:
                raise ValueError("File ended unexpectedly while reading chunk size.")
            chunk_size = struct.unpack('<I', chunk_size_bytes)[0]

            if chunk_id == b'data':
                data_found = True
                raw_audio_data = f.read(chunk_size)
                if len(raw_audio_data) < chunk_size:
                    raise ValueError("File ended unexpectedly while reading audio data.")
            else:
                f.seek(chunk_size, 1)

        processed_data = []
        if bits_per_sample == 8:
            if num_channels == 1:
                processed_data = list(raw_audio_data)
            else:
                for i in range(0, len(raw_audio_data), num_channels):
                    processed_data.append(raw_audio_data[i])
        elif bits_per_sample == 16:
            bytes_per_sample = bits_per_sample // 8
            for i in range(0, len(raw_audio_data), bytes_per_sample * num_channels):
                sample_16bit = struct.unpack('<h', raw_audio_data[i : i + bytes_per_sample])[0]
                sample_8bit = int((sample_16bit + 32768) / 256.0)
                processed_data.append(max(0, min(255, sample_8bit)))
    
    return processed_data, sample_rate

def generate_c_file(output_file_path, audio_data, sample_rate, base_name):
    with open(output_file_path, "w") as f:
        f.write(f"// {base_name}.h\n\n")
        f.write("#pragma once\n")
        f.write(f"#define {base_name.upper()}SAMPLE_RATE {sample_rate}\n\n")
        f.write(f"uint8_t {base_name}_fx[] = {{\n\t")

        for i, byte_val in enumerate(audio_data):
            f.write(f"0x{byte_val:02X}")
            if i < len(audio_data) - 1:
                f.write(", ")
            if (i + 1) % 16 == 0 and (i + 1) < len(audio_data):
                f.write("\n\t")
        f.write("\n};\n\n")
        f.write(f"size_t {base_name}_len = {len(audio_data)};\n\n")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python audio_data_generator.py <input_wav_file> <output_c_header_file>")
        sys.exit(1)
    
    input_wav_path = sys.argv[1]
    output_header_path = sys.argv[2]

    if not os.path.exists(input_wav_path):
        print(f"Error: Input WAV file not found at '{input_wav_path}'")
        sys.exit(1)

    try:
        audio_data, sample_rate = parse_wav(input_wav_path)

        file_name = os.path.basename(input_wav_path)
        base_name = os.path.splitext(file_name)[0]

        generate_c_file(output_header_path, audio_data, sample_rate, base_name)
        print(f"Successfully generated {output_header_path} from {input_wav_path}")
    except Exception as e:
        print(f"Error processing WAV file: {e}")
        sys.exit(1)

    print(f"Processing '{input_wav_path}'...")
