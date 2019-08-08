#include "jgbc.h"
#include "mmu.h"
#include "cpu.h"
#include "apu.h"

static inline void update_envelope(ChannelEnvelope *envelope);
static inline void update_length(ChannelLength *length, bool *channel_enabled);

static inline void init_square_wave(GameBoy *gb, const uint8_t idx);
static inline void read_square(GameBoy *gb, const uint16_t address, const uint8_t value, const uint8_t idx);
static inline void update_square(GameBoy *gb, const uint8_t idx);
static inline void update_square_sweep(GameBoy *gb);
static inline void trigger_square(GameBoy *gb, const uint8_t idx);

static inline void init_wave(GameBoy *gb);
static inline void read_wave(GameBoy *gb, const uint16_t address, const uint8_t value);
static inline void update_wave(GameBoy *gb);
static inline void trigger_wave(GameBoy *gb);

static inline void init_noise(GameBoy *gb);
static inline void read_noise(GameBoy *gb, const uint16_t address, const uint8_t value);
static inline void update_noise(GameBoy *gb);
static inline void trigger_noise(GameBoy *gb);


void init_apu(GameBoy *gb) {

    SDL_AudioSpec *desired = &gb->apu.desired_spec;
    SDL_zero(*desired);
    desired->freq = SAMPLE_RATE;
    desired->format = AUDIO_F32SYS;
    desired->channels = AUDIO_CHANNELS;
    desired->samples = AUDIO_SAMPLES;

    gb->apu.device_id = SDL_OpenAudioDevice(
        NULL, 
        0, 
        desired, 
        &gb->apu.actual_spec, 
        SDL_AUDIO_ALLOW_FREQUENCY_CHANGE
    );

    gb->apu.enabled = true;
    gb->apu.frame_sequencer_clock = 0;
    gb->apu.frame_sequencer_step = 0;
    gb->apu.downsample_clock = 0;

    gb->apu.buffer = malloc(gb->apu.actual_spec.size);
    gb->apu.buffer_position = 0;

    // memset(&gb->apu.square_waves[0], 0, sizeof(SquareWave));

    // TODO: replace with memset
    init_square_wave(gb, 0);
    init_square_wave(gb, 1);
    init_wave(gb);
}

static void init_square_wave(GameBoy *gb, const uint8_t idx) {

    assert(idx <= 1);
    SquareWave *square = &gb->apu.square_waves[idx];

    square->enabled = false;
    square->dac_enabled = false;
    square->frequency = 0;
    square->clock = 0;

    square->duty.mode = 0;
    square->duty.step = 0;

    square->length.enabled = false;
    square->length.clock = 0;

    square->envelope.period = 0;
    square->envelope.initial_volume = 0;
    square->envelope.current_volume = 0;
    square->envelope.mode = Decrease;

    square->sweep.enabled = false;
    square->sweep.period = 0;
    square->sweep.shift = 0;
    square->sweep.current_frequency = 0;
    square->sweep.mode = Addition;
}

static void init_wave(GameBoy *gb) {

    Wave *wave = &gb->apu.wave;
    wave->enabled = false;
    wave->position = 0;
    wave->clock = 0;
    wave->frequency = 0;
    wave->length.clock = 0;
    wave->length.enabled = false;
    wave->volume_code = 0;
}

static void init_noise(GameBoy *gb) {

    Noise *noise = &gb->apu.noise;
    noise->enabled = false;
    noise->clock = 0;
    noise->clock_shift = 0;
    noise->divisor_code = 0;
    noise->last_result = 0;
    noise->lfsr = 0;

    noise->envelope.period = 0;
    noise->envelope.initial_volume = 0;
    noise->envelope.current_volume = 0;
    noise->envelope.mode = Decrease;

    noise->length.clock = 0;
    noise->length.enabled = false;
    noise->width_mode = 0;
}

void update_apu(GameBoy *gb) {

    APU *apu = &gb->apu;

    if(!apu->enabled)
        return;

    for(int i = 0; i < gb->cpu.ticks; ++i) { 

        if(apu->frame_sequencer_clock >= FRAME_SEQUENCER_DIVIDER) {

            apu->frame_sequencer_clock = 0;
            apu->frame_sequencer_step++;
            apu->frame_sequencer_step %= 9;

            switch(gb->apu.frame_sequencer_step) {
                case 2:
                case 6:
                    /*update_square_sweep(gb);*/
                case 0:
                case 4:
                    update_length(&apu->square_waves[0].length, &apu->square_waves[0].enabled);
                    update_length(&apu->square_waves[1].length, &apu->square_waves[1].enabled);
                    update_length(&apu->wave.length, &apu->wave.enabled);
                    update_length(&apu->noise.length, &apu->noise.enabled);
                    break;

                case 7: // every 8 clocks
                    update_envelope(&apu->square_waves[0].envelope);
                    update_envelope(&apu->square_waves[1].envelope);
                    update_envelope(&apu->noise.envelope);
                    break;
            }
        }
        else
            apu->frame_sequencer_clock++;

        update_square(gb, 0);
        update_square(gb, 1);
        update_wave(gb);
        update_noise(gb);

        if(apu->downsample_clock++ >= DOWNSAMPLE_DIVIDER) {
            apu->downsample_clock = 0;

            float volume_left = (SREAD8(NR50) & NR50_VOL_LEFT) >> 4;
            volume_left /= 7;

            float volume_right = SREAD8(NR50) & NR50_VOL_RIGHT;
            volume_right /= 7;

            uint8_t left = 0;
            uint8_t right = 0;

            for(int i = 0; i < 4; ++i) {
                left += apu->channels[i];
                right += apu->channels[i];
            }

            left *= volume_left;
            right *= volume_right;

            apu->buffer[apu->buffer_position] = left / 255.0f;
            apu->buffer[apu->buffer_position + 1] = right / 255.0f;

            apu->buffer_position += AUDIO_CHANNELS;
        }
    }

    if(apu->buffer_position >= apu->actual_spec.size / sizeof(float)) {
        SDL_QueueAudio(apu->device_id, apu->buffer, apu->actual_spec.size);
        apu->buffer_position = 0;
    }
}

void audio_register_write(GameBoy *gb, const uint16_t address, const uint8_t value) {

    switch(address) {

        // Square Wave 1
        case NR10:
        case NR11:
        case NR12:
        case NR13:
        case NR14:
            read_square(gb, address, value, 0);
            break;

        // Square Wave 2
        case NR21:
        case NR22:
        case NR23:
        case NR24:
            read_square(gb, address, value, 1);
            break;

        // Wave
        case NR30:
        case NR31:
        case NR32:
        case NR33:
        case NR34:
            read_wave(gb, address, value);
            break;

        // Noise
        case NR41:
        case NR42:
        case NR43:
        case NR44:
            read_noise(gb, address, value);
            break;

        case NR52:
            gb->apu.enabled = (value & NR52_SND_ENABLED);
            break;
    }
}


static inline void update_envelope(ChannelEnvelope *envelope) {

    if(envelope->period == 0)
        return;

    if(envelope->mode == Increase && envelope->current_volume < 15)
        envelope->current_volume++;
    else if(envelope->mode == Decrease && envelope->current_volume > 0)
        envelope->current_volume--;
}

static inline void update_length(ChannelLength *length, bool *channel_enabled) {

    if(!length->enabled || length->clock == 0)
        return;

    if(length->clock-- == 0)
        *channel_enabled = false;
}

static inline void read_square(GameBoy *gb, const uint16_t address, const uint8_t value, const uint8_t idx) {

    assert(idx <= 1);
    SquareWave *square = &gb->apu.square_waves[idx];

    switch(address) {
        
        case NR10:
            square->sweep.period = (SweepMode) ((value & SQUARE_SWEEP) >> 4);
            square->sweep.mode = (value & SQUARE_MODE) >> 3;
            square->sweep.shift = value & SQUARE_SHIFT;
            break;

        case NR11:
        case NR21:
            square->duty.mode = (value & SQUARE_DUTY_MODE) >> 6;
            square->length.clock = value & CHANNEL_LENGTH;
            break;

        case NR12:
        case NR22:
            square->envelope.initial_volume = (value & CHANNEL_ENVELOPE_INITIAL_VOLUME) >> 4;
            square->envelope.mode = (EnvelopeMode) (value & CHANNEL_ENVELOPE_MODE) >> 3;
            square->envelope.period = value & CHANNEL_ENVELOPE_PERIOD;
            square->dac_enabled = value & SQUARE_DAC_ENABLED; // check upper 5 bits for 0
            break;

        case NR13:
        case NR23:
            square->frequency &= 0xFF00;
            square->frequency |= value;
            break;

        case NR14:
        case NR24:
            square->length.enabled = (value & CHANNEL_LENGTH_ENABLE) >> 6;
            square->frequency = ((value & CHANNEL_FREQUENCY_MSB) << 8) | (square->frequency & 0xFF);
            if(value & CHANNEL_TRIGGER) trigger_square(gb, idx);
            break;
    }
}

static inline void update_square(GameBoy *gb, const uint8_t idx) {

    assert(idx <= 1);
    SquareWave *square = &gb->apu.square_waves[idx];

    static const bool duty_table[4][8] = {
        { 0, 0, 0, 0, 0, 0, 0, 1 }, // 12.5%
        { 1, 0, 0, 0, 0, 0, 0, 1 }, // 25%
        { 1, 0, 0, 0, 0, 1, 1, 1 }, // 50%
        { 0, 1, 1, 1, 1, 1, 1, 0 }  // 75%
    };

    if(square->clock-- <= 0)
    {
        square->clock = (2048 - square->frequency) * 4;
        square->duty.step++;
        square->duty.step &= 0x7;
    }

    if(square->enabled && square->dac_enabled && 
       duty_table[square->duty.mode][square->duty.step]) {

        // Maximum envelope volume is 15 so fill the byte 255 / 15 = 17
        gb->apu.channels[idx] = square->envelope.current_volume * 17;
    }
    else {
        gb->apu.channels[idx] = 0;
    }
}


static inline void update_square_sweep(GameBoy *gb) {

    SquareWave *square = &gb->apu.square_waves[0];

    if(!square->sweep.enabled || square->sweep.period == 0)
        return;

    // TODO: Implement
}

static inline void trigger_square(GameBoy *gb, const uint8_t idx) {

    assert(idx <= 1);
    SquareWave *square = &gb->apu.square_waves[idx];
    square->enabled = true;

    if(square->length.clock == 0)
        square->length.clock = 64;

    square->clock = (2048 - square->frequency) * 4;
    square->envelope.current_volume = square->envelope.initial_volume;

    square->sweep.current_frequency = square->frequency;
    // sweep timer reset
    square->sweep.enabled = (square->sweep.period > 0 || square->sweep.shift > 0);

    /*if(square->sweep.shift > 0)*/
        // frequency calculation and overflow check
}

static inline void read_wave(GameBoy *gb, const uint16_t address, const uint8_t value) {

    Wave *wave = &gb->apu.wave;

    switch(address) {
        
        case NR30:
            wave->enabled = (value & WAVE_ENABLED) >> 7;
            break;

        case NR31:
            wave->length.clock = value;
            break;

        case NR32:
            wave->volume_code = (value & WAVE_VOLUME) >> 5;
            break;

        case NR33:
            wave->frequency &= 0xFF00;
            wave->frequency |= value;
            break;

        case NR34:
            wave->length.enabled = (value & CHANNEL_LENGTH_ENABLE) >> 6;
            wave->frequency = ((value & CHANNEL_FREQUENCY_MSB) << 8) | (wave->frequency & 0xFF);
            if(value & CHANNEL_TRIGGER) trigger_wave(gb);
            break;
    }
}

static inline void update_wave(GameBoy *gb) {

    Wave *wave = &gb->apu.wave;

    if(wave->clock-- <= 0)
    {
        wave->clock = (2048 - wave->frequency) * 2;

        if(wave->position++ == 31)
            wave->position = 0;
    }

    if(wave->enabled && wave->volume_code > 0) {

        uint8_t sample;

        // Top 4 bits
        if(wave->position % 2 == 0) {
            sample = (SREAD8(WAVE_TABLE_START + wave->position) & 0xF0) >> 4;
        }
        // Lower 4 bits
        else {
            sample = SREAD8(WAVE_TABLE_START + wave->position - 1) & 0xF;
        }

        sample = sample >> (wave->volume_code - 1);
        gb->apu.channels[2] = sample * 3;
    }
    else
        gb->apu.channels[2] = 0;
}

static inline void trigger_wave(GameBoy *gb) {

    Wave *wave = &gb->apu.wave;
    wave->enabled = true;

    if(wave->length.clock == 0)
        wave->length.clock = 256;

    wave->position = 0; 
}

static inline void read_noise(GameBoy *gb, const uint16_t address, const uint8_t value) {

    Noise *noise = &gb->apu.noise;

    switch(address) {
        
        case NR41:
            noise->length.clock = (value & CHANNEL_LENGTH);
            break;

        case NR42:
            noise->envelope.initial_volume = (value & CHANNEL_ENVELOPE_INITIAL_VOLUME) >> 4;
            noise->envelope.mode = (EnvelopeMode) (value & CHANNEL_ENVELOPE_MODE) >> 3;
            noise->envelope.period = value & CHANNEL_ENVELOPE_PERIOD;
            break;

        case NR43:
            noise->clock_shift = (value & NOISE_CLOCK_SHIFT) >> 4;
            noise->width_mode = (value & NOISE_WIDTH_MODE) >> 3;
            noise->divisor_code = (value & NOISE_DIVISOR_CODE);
            break;

        case NR44:
            noise->length.enabled = (value & CHANNEL_LENGTH_ENABLE) >> 6;
            if(value & CHANNEL_TRIGGER) trigger_noise(gb);
            break;
    }
}

static inline void update_noise(GameBoy *gb) {

    Noise *noise = &gb->apu.noise;

    if(!noise->enabled)
        return;

    if(noise->clock-- <= 0)
    {
        uint8_t divisor;

        switch(noise->divisor_code) {
            case 0: divisor = 8; break;
            case 1: divisor = 16; break;
            case 2: divisor = 32; break;
            case 3: divisor = 48; break;
            case 4: divisor = 64; break;
            case 5: divisor = 80; break;
            case 6: divisor = 96; break;
            case 7: divisor = 112; break;
        }

        noise->clock = (divisor << noise->clock_shift);

        uint8_t new_bit = (GET_BIT(noise->lfsr, 1) ^ GET_BIT(noise->lfsr, 0));

        noise->lfsr = noise->lfsr >> 1;
        noise->lfsr |= (new_bit << 14);

        if(noise->width_mode == 1)
            noise->lfsr |= (new_bit << 5);

        noise->last_result = !GET_BIT(noise->lfsr, 0);
    }

    gb->apu.channels[3] = noise->last_result * noise->envelope.current_volume; 
}

static inline void trigger_noise(GameBoy *gb) {

    Noise *noise = &gb->apu.noise;
    noise->enabled = true;

    if(noise->length.clock == 0)
        noise->length.clock = 64;

    noise->lfsr = 0x7FFF;
}