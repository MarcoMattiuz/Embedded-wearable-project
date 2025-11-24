#include "hr_logic.h"

// Variabile globale per il DC stimato
static int32_t RED_dc_estimate = 0;
static int32_t IR_dc_estimate = 0;
int16_t cbuf[32];
uint8_t offset = 0;
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
static const uint32_t MIN_BEAT_INTERVAL = 20; // Minimum samples between beats (0.5 seconds at 50 SPS)
uint32_t sample_counter = 0; // Global sample counter for accurate BPM calculation
int threshold = 0;
long lastBeatSample = 0; // Sample number at which the last beat occurred
float oldBPM = 0.0f;
int countBPMmeasures = 1;
const float SAMPLE_RATE = 50.0f; // 50 samples per second
#define RATE_SIZE 4 //Increase this for more averaging. 4 is good.
float rates[RATE_SIZE]; //Array of heart rates
int rateSpot = 0;
//low passing filter
static const uint16_t FIRCoeffs[12] = {172, 321, 579, 927, 1360, 1858, 2390, 2916, 3391, 3768, 4012, 4096};





int32_t mul16(int16_t x, int16_t y)
{
  return((long)x * (long)y);
}
//IIR filter "exponential moving average"
static int16_t averageDCEstimator(int32_t *p, uint16_t x)
{
    *p += ((((long) x << 15) - *p) >> 4); //x * 2^15 (to work in fixed point, for higher precision)
                                          //(x*2^15)-*p where *p is the current dc value
                                          // >> 4 means divide by 16 (* 1/16)
    return (*p >> 15);
}


int16_t lowPassFIRFilter(int16_t din)
{  
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
// Funzione per ottenere la parte AC del segnale
int16_t get_RED_AC(uint32_t sample) {
    int16_t dc_estimate = averageDCEstimator(&RED_dc_estimate, sample);
    return lowPassFIRFilter(sample - dc_estimate);
}
int16_t get_IR_AC(uint32_t sample) {
    // int16_t dc_estimate = DCEstimatorWithMean(sample);
    int16_t dc_estimate = averageDCEstimator(&IR_dc_estimate, sample);
    return lowPassFIRFilter(sample - dc_estimate);
}
static int32_t dc_w = 0;

int16_t get_IR_AC2(int32_t x)
{
    dc_w = dc_w + ((x - dc_w) >> 3);   // coeff più veloce: >>3 invece di >>4
    return lowPassFIRFilter((int16_t)(x - dc_w));
}

// bool beat_detected(int16_t ir_ac) {
//     bool beatDetected = false;

//     //  Save current state
//     IR_AC_Signal_Previous = IR_AC_Signal_Current;
    
//     //  Process next data sample
//     IR_AC_Signal_Current = ir_ac;

//     // printf("IR_AC_prev: %d - IR_AC_curr: %d\n",IR_AC_Signal_Previous,IR_AC_Signal_Current);
//     //  Detect positive zero crossing (rising edge)
//     if ((IR_AC_Signal_Previous + threshold < 0) && (IR_AC_Signal_Current - threshold >= 0))
//     {
//         IR_AC_Max = IR_AC_Signal_max; //Adjust our AC max and min
//         IR_AC_Min = IR_AC_Signal_min;

//         positiveEdge = 1;
//         negativeEdge = 0;
//         IR_AC_Signal_max = 0;

//         // Check for valid beat amplitude and minimum time interval
//         int16_t amplitude = IR_AC_Max - IR_AC_Min;
//         uint32_t samples_since_last_beat = sample_counter - last_beat_sample;
        
//         if ((amplitude > 50) && (amplitude < 2000) && (samples_since_last_beat > MIN_BEAT_INTERVAL))
//         {
//             //Heart beat detected!
//             beatDetected = true;
//             last_beat_sample = sample_counter;
//         }
//     }

//     //  Detect negative zero crossing (falling edge)
//     if ((IR_AC_Signal_Previous - threshold > 0) && (IR_AC_Signal_Current + threshold <= 0))
//     {
//         positiveEdge = 0;
//         negativeEdge = 1;
//         IR_AC_Signal_min = 0;
//     }

//     //  Find Maximum value in positive cycle
//     if (positiveEdge && (IR_AC_Signal_Current > IR_AC_Signal_Previous))
//     {
//         IR_AC_Signal_max = IR_AC_Signal_Current;
//     }

//     //  Find Minimum value in negative cycle
//     if (negativeEdge && (IR_AC_Signal_Current < IR_AC_Signal_Previous))
//     {
//         IR_AC_Signal_min = IR_AC_Signal_Current;
//     }
    
//     return(beatDetected);
// }    

bool beat_detected(int16_t ir_ac) {
    bool beat = false;

    IR_AC_Signal_Previous = IR_AC_Signal_Current;
    IR_AC_Signal_Current = ir_ac;

    // Zero crossing positivo → potenziale beat
    if (IR_AC_Signal_Previous < 0 && IR_AC_Signal_Current >= 0) {
        
        // Ampiezza del ciclo misurata in modo semplice
        int amplitude = IR_AC_Max - IR_AC_Min;


        // Refractory period: distanza minima tra due beat
        uint32_t delta = sample_counter - lastBeatSample;
        if (delta < MIN_BEAT_INTERVAL) {
            return false;   // troppo vicino → SCARTO
        }

        // Deve superare una soglia minima
        if (amplitude > 80 && amplitude < 2000) {
            beat = true;
        }

        // reset min/max
        IR_AC_Max = -2000;
        IR_AC_Min = 2000;
    }

    // Durante la fase positiva -> trova max
    if (IR_AC_Signal_Current > IR_AC_Max)
        IR_AC_Max = IR_AC_Signal_Current;

    // Durante la fase negativa -> trova min
    if (IR_AC_Signal_Current < IR_AC_Min)
        IR_AC_Min = IR_AC_Signal_Current;

    return beat;
}



void calculateBPM(int16_t ir_ac, float *BPM, float *AVG_BPM) {

    // Aumenta il contatore globale dei campioni
    sample_counter++;
    // printf("sample_counter: %ld.    ",sample_counter);
    // Se è stato rilevato un battito
    if (beat_detected(ir_ac)) {

        long now = sample_counter;

        // Gestione primo beat → non possiamo ancora calcolare BPM
        if (lastBeatSample == 0) {
            lastBeatSample = now;
            return;
        }

        // Samples trascorsi dall'ultimo beat
        long delta = now - lastBeatSample;
        lastBeatSample = now;

        // Converti in BPM
        float currBPM = 60.0f * SAMPLE_RATE / delta;

        // Filtri per stabilità
        if (currBPM > 30 && currBPM < 220) {

            // Memorizza BPM e crea media scorrevole
            rates[rateSpot] = currBPM;
            rateSpot = (rateSpot + 1) % RATE_SIZE;

            float sum = 0;
            for (int i = 0; i < RATE_SIZE; i++)
                sum += rates[i];

            *BPM = currBPM;
            *AVG_BPM = sum / RATE_SIZE;

            printf("BEAT → BPM: %.1f | AVG: %.1f\n", *BPM, *AVG_BPM);
        }
    }
}

