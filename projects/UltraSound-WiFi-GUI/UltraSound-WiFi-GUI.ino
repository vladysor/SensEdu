#include "SensEdu.h"
#include "CMSIS_DSP.h"
#include "SineLUT.h"
#include "FilterTaps.h"
#include "DACWave.h" // contains wave and its size

uint32_t lib_error = 0;
uint8_t error_led = D86;

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */
#define IS_TRANSMIT_DETAILED_DATA   true    // activate full raw, filtered, xcorr data transmission
#define BAN_DISTANCE	            20	    // min distance [cm] - how many self reflections cancelled
#define ACTUAL_SAMPLING_RATE        250000  // You need to measure this value using a wave generator with a fixed e.g. 1kHz Sine
#define STORE_BUF_SIZE              32 * 32 // 2400 for 1 measurement per second. 
                            	        // only multiples of 32!!!!!! (64 chunk size of bytes, so 32 for 16bit)

/* --------------------------------- Filter --------------------------------- */
#define FILTER_BLOCK_LENGTH     32      // how many samples we want to process every time we call the fir process function AT
#define FILTER_TAP_NUM          32      // tap number for the bandpass filter

static float32_t firStateBuffer[FILTER_BLOCK_LENGTH + FILTER_TAP_NUM - 1]; // current filter state buffer
arm_fir_instance_f32 Fir_filt; // creating an object instance

/* ----------------------------------- ADC ---------------------------------- */
ADC_TypeDef* adc1 = ADC1;
ADC_TypeDef* adc2 = ADC2;

const uint8_t adc1_mic_num = 2; // 2 microphones for adc1
const uint8_t adc2_mic_num = 2; // 2 microphones for adc2

uint8_t adc1_pins[adc1_mic_num] = {A5, A10}; // mic1 and mic2
uint8_t adc2_pins[adc2_mic_num] = {A1, A6}; // mic3 and mic4 

// must be:
// 1. multiple of 32 words (64 half-words) to ensure cache coherence
// 2. properly aligned
const uint16_t adc1_data_size = STORE_BUF_SIZE * adc1_mic_num;
const uint16_t adc2_data_size = STORE_BUF_SIZE * adc2_mic_num;
__attribute__((aligned(__SCB_DCACHE_LINE_SIZE))) uint16_t adc1_data[adc1_data_size];
__attribute__((aligned(__SCB_DCACHE_LINE_SIZE))) uint16_t adc2_data[adc2_data_size];

SensEdu_ADC_Settings adc1_settings = {
    .adc = adc1,
    .pins = adc1_pins,
    .pin_num = adc1_mic_num,

    .conv_mode = SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED,
    .sampling_freq = 250000,
    
    .dma_mode = SENSEDU_ADC_DMA_CONNECT,
    .mem_address = (uint16_t*)adc1_data,
    .mem_size = adc1_data_size
};

SensEdu_ADC_Settings adc2_settings = {
    .adc = adc2,
    .pins = adc2_pins,
    .pin_num = adc2_mic_num,

    .conv_mode = SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED,
    .sampling_freq = 250000,
    
    .dma_mode = SENSEDU_ADC_DMA_CONNECT,
    .mem_address = (uint16_t*)&adc2_data,
    .mem_size = adc2_data_size
};

/* ----------------------------------- DAC ---------------------------------- */
DAC_Channel* dac_channel = DAC_CH1;
// lut settings are in SineLUT.h
#define DAC_SINE_FREQ     	32000                           // 32kHz
#define DAC_SAMPLE_RATE     DAC_SINE_FREQ * sine_lut_size   // 64 samples per one sine cycle

SensEdu_DAC_Settings dac_settings = {dac_channel, DAC_SAMPLE_RATE, (uint16_t*)sine_lut, sine_lut_size, 
    SENSEDU_DAC_MODE_BURST_WAVE, dac_cycle_num}; // specifying burst mode 

/* -------------------------------------------------------------------------- */
/*                                  Constants                                 */
/* -------------------------------------------------------------------------- */

const uint16_t air_speed = 343; // m/s

// e.g. 25cm ban means 0.25*2/343 time ban, then multiply by sample rate
const uint32_t banned_sample_num = ((BAN_DISTANCE*2*ACTUAL_SAMPLING_RATE)/air_speed)/100; 

/* -------------------------------------------------------------------------- */
/*                              Global Structure                              */
/* -------------------------------------------------------------------------- */
typedef struct {
	uint8_t ban_flag; // activate self reflections ban
	char serial_read_buf;
    float xcorr_buffer[STORE_BUF_SIZE]; // use same buffer for several functions for memory saving
    float modified_xcorr_buffer[STORE_BUF_SIZE]; // use same buffer for several functions for memory saving
    uint8_t trust;
    
} SenseduBoard;

static SenseduBoard SenseduBoardObj;

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */

void setup() {
	SenseduBoard* main_obj_ptr = &SenseduBoardObj;
	main_obj_init(main_obj_ptr);

    // initializing the filter
    arm_fir_init_f32(&Fir_filt, FILTER_TAP_NUM, filter_taps, firStateBuffer, FILTER_BLOCK_LENGTH); 

    Serial.begin(115200);

    /* DAC */
    SensEdu_DAC_Init(&dac_settings);
    
    /* ADC 1 */
    SensEdu_ADC_Init(&adc1_settings);
    SensEdu_ADC_Enable(adc1);

    /* ADC 2 */
    SensEdu_ADC_Init(&adc2_settings);
    SensEdu_ADC_Enable(adc2);

    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        handle_error();
    }
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */

void loop() {
	SenseduBoard* main_obj_ptr = &SenseduBoardObj;
    // Measurement is initiated by the signal from computing device (matlab script)
    static char serial_buf = 0;
    
    // Measurement is initiated by signal from computing device
    while (1) {
        while (Serial.available() == 0); // Wait for a signal
        serial_buf = Serial.read();
        if (serial_buf == 't') {
            break; 
        }
    }
    
    // Start dac->adc sequence
    SensEdu_DAC_Enable(dac_channel);
    while(!SensEdu_DAC_GetBurstCompleteFlag(dac_channel)); // wait for dac to finish sending the burst
    SensEdu_DAC_ClearBurstCompleteFlag(dac_channel); 
    
    // Start ADCs
    SensEdu_ADC_Start(adc1);
    SensEdu_ADC_Start(adc2);

    // Wait for the data from ADC1
    while(!SensEdu_ADC_GetTransferStatus(adc1));
    SensEdu_ADC_ClearTransferStatus(adc1);

    // Wait for the data from ADC2
    while(!SensEdu_ADC_GetTransferStatus(adc2));
    SensEdu_ADC_ClearTransferStatus(adc2);

    // Calculating distance for each microphone
    static uint32_t distance[4];
    process_and_transmit_data(main_obj_ptr->xcorr_buffer, sizeof(main_obj_ptr->xcorr_buffer), adc1_data, sizeof(adc1_data), "1", 2, main_obj_ptr->ban_flag, IS_TRANSMIT_DETAILED_DATA);
    distance[0] = calculate_distance(main_obj_ptr->xcorr_buffer);
    process_and_transmit_data(main_obj_ptr->xcorr_buffer, sizeof(main_obj_ptr->xcorr_buffer), adc1_data, sizeof(adc1_data), "2", 2, main_obj_ptr->ban_flag, IS_TRANSMIT_DETAILED_DATA);
    distance[1] = calculate_distance(main_obj_ptr->xcorr_buffer);
    process_and_transmit_data(main_obj_ptr->xcorr_buffer, sizeof(main_obj_ptr->xcorr_buffer), adc2_data, sizeof(adc2_data), "1", 2, main_obj_ptr->ban_flag, IS_TRANSMIT_DETAILED_DATA);
	distance[2] = calculate_distance(main_obj_ptr->xcorr_buffer);
    process_and_transmit_data(main_obj_ptr->xcorr_buffer, sizeof(main_obj_ptr->xcorr_buffer), adc2_data, sizeof(adc2_data), "2", 2, main_obj_ptr->ban_flag, IS_TRANSMIT_DETAILED_DATA);
    distance[3] = calculate_distance(main_obj_ptr->xcorr_buffer);

    // Sending the distance measurements
    Serial.write((const uint8_t *) &distance[0], 4);
    Serial.write((const uint8_t *) &distance[1], 4);
    Serial.write((const uint8_t *) &distance[2], 4);
    Serial.write((const uint8_t *) &distance[3], 4);

    // check errors
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        handle_error();
    }
}

void process_and_transmit_data(float* xcorr_buf, size_t xcorr_buf_size, uint16_t* mic_array, size_t mic_array_size, 
    const char* channel, uint8_t adc_ch_num, uint8_t ban_flag, uint8_t is_detailed_transmission) {

    /* -------------------------------- RAW DATA -------------------------------- */
    if (is_detailed_transmission)
        serial_send_array((const uint8_t*)mic_array, mic_array_size, channel, adc_ch_num);
    
    /* --------------------- RESCALED, FILTERED, NO COUPLED --------------------- */
    // Rescale from [0, (2^16-1)] to [-1, 1] and filter around 32 kHz
    rescale_adc_wave(xcorr_buf, mic_array, channel, mic_array_size, adc_ch_num);
	if (ban_flag == 1) {
        // remove self reflections from a dataset
		for (uint32_t i = 0; i < banned_sample_num; i++) {
			xcorr_buf[i] = 0;
		}
	}
    if (is_detailed_transmission)
        serial_send_array((const uint8_t*)xcorr_buf, xcorr_buf_size, "b", 1);

    /* ---------------------------------- XCORR --------------------------------- */
	custom_xcorr(xcorr_buf, dac_wave, STORE_BUF_SIZE);
    if (is_detailed_transmission)
	    serial_send_array((const uint8_t*)xcorr_buf, xcorr_buf_size, "b", 1);
    
}