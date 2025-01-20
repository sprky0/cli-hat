#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define TWO_PI 6.283185307f

typedef struct {
    float tune;  // 0..1
    float decay; // 0..1
    float level; // 0..1
    float open;  // 0..1 (0 = closed, 1 = open)
} HatParams;

/**
 * We’ll define six (or more) base frequencies for the “swarm” of square oscillators. 
 * The real 606 has a cluster of slightly detuned oscillators. We'll pick some 
 * close-together frequencies. You can tweak these for different “metallic” results.
 */
static const float BASE_FREQS[6] = {
    452.0f,  // freq1
    539.0f,  // freq2
    645.0f,  // freq3
    750.0f,  // freq4
    851.0f,  // freq5
    946.0f   // freq6
};

/**
 * Generate a 606-ish hi-hat into 'buffer'.
 * sampleRate: e.g. 48000.0f
 * numSamples: length of output buffer
 * params:     user-defined 0..1 for each "knob"
 */
void generateHat606(float *buffer, size_t numSamples, float sampleRate, HatParams params)
{
    // For a hi-hat, we often have a very short or somewhat longer decay, 
    // depending on closed vs open. We'll define a “base” decay time, 
    // then scale it up if "open" is high.

    // Decay time range (closed: ~40 ms => open: ~400 ms or more, etc.)
    float minDecay = 0.04f;  // 40 ms
    float maxDecay = 0.4f;   // 400 ms
    float decayTime = minDecay + (maxDecay - minDecay) * params.decay;

    // The 'open' parameter will *further* scale the decay time.
    // If open=0 => nearly closed, if open=1 => fully open, up to maybe 2x the above.
    float finalDecayTime = decayTime * (1.0f + params.open * 1.5f);

    // Convert to samples
    size_t decaySamples = (size_t)(finalDecayTime * sampleRate);

    // We'll do a near-instant attack
    // So the amplitude will just start at 1.0 and decay from there.
    float amp = 1.0f;
    float ampDecayRate = (decaySamples > 0) 
                         ? (1.0f / (float)decaySamples) 
                         : 1.0f;

    // We'll let 'tune' shift all oscillator frequencies up or down
    // For example, 0 => ~0.8x the base freqs, 1 => ~1.2x
    float freqScale = 0.8f + (0.4f * params.tune);

    // Keep track of phase for each oscillator
    // (6 oscillators in this example)
    float phases[6] = {0};
    
    // We'll define a function that produces a square wave at frequency freq:
    //    square(t) = sign(sin(phase)) => +1 or -1
    // We’ll just do that inline in the loop.

    // Fill the buffer
    for (size_t n = 0; n < numSamples; n++) {
        // Summation of multiple square waves
        float sampleValue = 0.0f;

        for (int i = 0; i < 6; i++) {
            float freq = BASE_FREQS[i] * freqScale;
            // Update phase
            phases[i] += (TWO_PI * freq / sampleRate);
            if (phases[i] > TWO_PI) {
                phases[i] -= TWO_PI;
            }
            // Square wave
            float sq = (sinf(phases[i]) >= 0.0f) ? 1.0f : -1.0f;
            sampleValue += sq;
        }

        // Average them to keep the amplitude in a more reasonable range
        sampleValue /= 6.0f;

        // Apply amplitude envelope
        sampleValue *= amp * params.level;

        // Write to buffer
        buffer[n] = sampleValue;

        // Decay amplitude
        if (n < decaySamples) {
            amp -= ampDecayRate;
            if (amp < 0.0f) amp = 0.0f;
        } else {
            // After decaySamples, amplitude is near zero; 
            // we can just let it remain or clamp
            amp = 0.0f;
        }
    }
}

/**
 * Write a minimal 24-bit mono WAV to the given FILE *.
 * (Same approach as in the kick program.)
 */
void writeWav24Bit(FILE *fp, const float *buffer, size_t numSamples, int sampleRate)
{
    // --- WAV Header ---
    unsigned int dataChunkSize = (unsigned int)(numSamples * 3); // 3 bytes per sample (24-bit)
    unsigned int subchunk2Size = dataChunkSize;
    unsigned int chunkSize     = 36 + subchunk2Size; 
    
    // Write "RIFF"
    fputs("RIFF", fp);
    fwrite(&chunkSize, 4, 1, fp);
    // Write "WAVE"
    fputs("WAVE", fp);
    
    // "fmt " subchunk
    fputs("fmt ", fp);
    unsigned int subchunk1Size = 16; // PCM
    fwrite(&subchunk1Size, 4, 1, fp);
    
    unsigned short audioFormat   = 1; // PCM
    unsigned short numChannels   = 1; // mono
    unsigned int   sRate         = (unsigned int)sampleRate;
    unsigned short bitsPerSample = 24;
    unsigned short blockAlign    = numChannels * (bitsPerSample / 8);
    unsigned int   byteRate      = sRate * blockAlign;
    
    fwrite(&audioFormat,     2, 1, fp);
    fwrite(&numChannels,     2, 1, fp);
    fwrite(&sRate,           4, 1, fp);
    fwrite(&byteRate,        4, 1, fp);
    fwrite(&blockAlign,      2, 1, fp);
    fwrite(&bitsPerSample,   2, 1, fp);
    
    // "data" subchunk
    fputs("data", fp);
    fwrite(&subchunk2Size, 4, 1, fp);
    
    // --- WAV Sample Data (24-bit) ---
    for (size_t i = 0; i < numSamples; i++) {
        float val = buffer[i];
        // Clip to [-1..1]
        if (val > 1.0f)  val = 1.0f;
        if (val < -1.0f) val = -1.0f;
        
        // Convert float [-1..1] to 24-bit int
        float scaled = val * 8388607.0f; // max 24-bit positive = 8388607
        int sample = (int)lrintf(scaled);

        if (sample >  8388607)  sample =  8388607;
        if (sample < -8388608)  sample = -8388608;
        
        // Write as 24-bit little-endian
        unsigned char b1 = (unsigned char)(sample & 0xFF);
        unsigned char b2 = (unsigned char)((sample >> 8) & 0xFF);
        unsigned char b3 = (unsigned char)((sample >> 16) & 0xFF);
        
        fwrite(&b1, 1, 1, fp);
        fwrite(&b2, 1, 1, fp);
        fwrite(&b3, 1, 1, fp);
    }
}

/**
 * main()
 * Usage: 
 *   ./hat606 <tune> <decay> <level> <open> [-o <output.wav>]
 * 
 * - tune  (0..1) : shifts oscillator frequencies
 * - decay (0..1) : short => long
 * - level (0..1) : overall volume
 * - open  (0..1) : 0 => closed, 1 => open; scales decay length
 */
int main(int argc, char *argv[])
{
    if (argc < 5) {
        fprintf(stderr, "Usage: %s <tune> <decay> <level> <open> [-o <output.wav>]\n", argv[0]);
        return 1;
    }
    
    HatParams params;
    params.tune  = atof(argv[1]);  // 0..1
    params.decay = atof(argv[2]);  // 0..1
    params.level = atof(argv[3]);  // 0..1
    params.open  = atof(argv[4]);  // 0..1
    
    // We'll do up to ~1 second default. 
    // If 'open' is high, the sound may ring out longer, 
    // but let's just keep the buffer at 1.5 or 2.0 seconds to be safe.
    float sampleRate = 48000.0f;
    float duration   = 2.0f;  // max length to accommodate open hats
    size_t numSamples = (size_t)(sampleRate * duration);
    
    // Check optional -o
    FILE *outFile = stdout;
    for (int i = 5; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            if ((i + 1) < argc) {
                outFile = fopen(argv[i + 1], "wb");
                if (!outFile) {
                    fprintf(stderr, "Error opening file '%s'\n", argv[i + 1]);
                    return 1;
                }
                i++;
            } else {
                fprintf(stderr, "Error: missing filename after -o\n");
                return 1;
            }
        }
    }
    
    // Allocate buffer
    float *buffer = (float *)calloc(numSamples, sizeof(float));
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed.\n");
        return 1;
    }
    
    // Generate hat
    generateHat606(buffer, numSamples, sampleRate, params);
    
    // Write 24-bit WAV
    writeWav24Bit(outFile, buffer, numSamples, (int)sampleRate);
    
    // Cleanup
    free(buffer);
    if (outFile != stdout) {
        fclose(outFile);
    }
    
    return 0;
}
