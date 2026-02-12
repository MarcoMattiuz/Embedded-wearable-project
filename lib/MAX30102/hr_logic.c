#include "hr_logic.h"

// Variabile globale per il DC stimato
static int32_t RED_dc_estimate = 0;
static int32_t IR_dc_estimate = 0;
// Peak detection variables
int16_t IR_AC_Max = -2000;
int16_t IR_AC_Min = 2000;
int16_t IR_AC_Signal_min = 0;
int16_t IR_AC_Signal_max = 0;
int16_t IR_Average_Estimated;
int16_t IR_AC_Signal_Current = 0;
int16_t IR_AC_Signal_Previous = 0;
int16_t positiveEdge = 0;
int16_t negativeEdge = 0;
int32_t ir_avg_reg = 0;


// Beat detection timing
// static uint32_t last_beat_sample = 0;
static const uint32_t       MIN_BEAT_INTERVAL = 15; // Minimum samples between beats 
uint32_t                    sample_counter = 0; // Global sample counter for accurate BPM calculation
int                         threshold = 40;
long                        lastBeatSample = 0; // Sample number at which the last beat occurred
float                       oldBPM = 0.0f;
const float                 SAMPLE_RATE = 50.0f; // 50 samples per second
#define                     MAX_RATE_SIZE 10 //max rate size
float                       rates[MAX_RATE_SIZE]; //Array of heart rates
int                         rate_size = 4; // variable rate size
int                         rates_index = 0; 
int                         deltaBPM = 0;
//low passing filter
static const uint16_t       FIRCoeffs[12] = {172, 321, 579, 927, 1360, 1858, 2390, 2916, 3391, 3768, 4012, 4096};



int32_t mul16(int16_t x, int16_t y)
{
  return((long)x * (long)y);
}
//IIR filter "exponential moving average", used to calculate the DC (continuous) part of the signal
static int16_t averageDCEstimator(int32_t *p, uint16_t x)
{
    *p += ((((long) x << 15) - *p) >> 4); //x * 2^15 (to work in fixed point, for higher precision)
                                          //(x*2^15)-*p where *p is the current dc value
                                          // >> 4 means divide by 16 (* 1/16)
    return (*p >> 15);
}

int16_t lowPassFIRFilterIR(int16_t din)
{  
    static int16_t cbuf[32];
    static uint8_t offset = 0;
    cbuf[offset] = din;

    int32_t z = mul16(FIRCoeffs[11], cbuf[(offset - 11) & 0x1F]);
    
    for (uint8_t i = 0 ; i < 11 ; i++)
    {
        z += mul16(FIRCoeffs[i], cbuf[(offset - i) & 0x1F] + cbuf[(offset - 22 + i) & 0x1F]);
    }

    offset++;
    offset %= 32; //Wrap condition

    return(z >> 15);
}

int16_t lowPassFIRFilterRED(int16_t din)
{  
    static int16_t cbuf[32];
    static uint8_t offset = 0;
    cbuf[offset] = din;

    int32_t z = mul16(FIRCoeffs[11], cbuf[(offset - 11) & 0x1F]);
    
    for (uint8_t i = 0 ; i < 11 ; i++)
    {
        z += mul16(FIRCoeffs[i], cbuf[(offset - i) & 0x1F] + cbuf[(offset - 22 + i) & 0x1F]);
    }

    offset++;
    offset %= 32; //Wrap condition

    return(z >> 15);
}

// functions to get AC filtered signals
int16_t get_RED_AC(uint32_t sample) {
    int16_t dc_estimate = averageDCEstimator(&RED_dc_estimate, sample);
    return lowPassFIRFilterRED(sample - dc_estimate);
}

int16_t get_IR_AC(uint32_t sample) {
    int16_t dc_estimate = averageDCEstimator(&IR_dc_estimate, sample);
    return lowPassFIRFilterIR(sample - dc_estimate);
}

bool beat_detected(int16_t ir_ac) {
    bool beat = false;

    IR_AC_Signal_Previous = IR_AC_Signal_Current;
    IR_AC_Signal_Current = ir_ac;

    // a positive zero crossing potential beat
    if (IR_AC_Signal_Previous < 0 && IR_AC_Signal_Current >= 0) {
        
        int amplitude = IR_AC_Max - IR_AC_Min;
        // DBG_PRINTF("AMPLITUDE: %d\n",amplitude);

        // Refractory period: distanza minima tra due beat
        uint32_t delta = sample_counter - lastBeatSample;
        if (delta < MIN_BEAT_INTERVAL) {
            return false;   // troppo vicino → SCARTO
        }

        if (amplitude > 100 && amplitude < 2000) {
            beat = true;
        }

        // reset min/max
        IR_AC_Max = -2000;
        IR_AC_Min = 2000;
    }

    // find max positive
    if (IR_AC_Signal_Current > IR_AC_Max)
        IR_AC_Max = IR_AC_Signal_Current;

    // find min negative
    if (IR_AC_Signal_Current < IR_AC_Min)
        IR_AC_Min = IR_AC_Signal_Current;

    return beat;
}


/*
    Calculates bpm: the first time it senses a beat it saves the sample_counter,
    the other times it uses the current and past sample_counter of when it had sensed
    a beat to calculate the bpm.
    Every time a new bpm is calculated it is used to calculate the delta of samples for the 
    rate_size, and then it is used a circular buffer to calculate the avg bpm, (avg goes from 2 - 10)
*/
void calculateBPM(int16_t ir_ac, int16_t *BPM, int16_t *AVG_BPM) {
    sample_counter++;
    if (beat_detected(ir_ac)) {
        long now = sample_counter;
        //first beat
        if (lastBeatSample == 0) {
            lastBeatSample = now;
            return;
        }
        long delta = now - lastBeatSample;
        lastBeatSample = now;
        // calculate current BPM
        float currBPM = 60.0f * SAMPLE_RATE / delta;
        // deltaBPM = currBPM-oldBPM >= 0 ? currBPM-oldBPM : -(currBPM-oldBPM);
        oldBPM = currBPM;
        if (currBPM > 40 && currBPM < 210) {

            // moving avg
            rates[rates_index] = currBPM;
            rates_index = (rates_index + 1) % rate_size;

            float sum = 0;
            for (int i = 0; i < rate_size; i++)
                sum += rates[i];

            *BPM = currBPM;
            *AVG_BPM = sum / rate_size;
            // DBG_PRINTF(">--BEAT--< BPM: %d | AVG: %d\n - deltaBPM: %d - rate_size: %d\n", *BPM, *AVG_BPM, deltaBPM, rate_size);
            // DBG_PRINTF(">-----BEAT-----<\n");
        }
    }
}

