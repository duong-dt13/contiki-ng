// pulse_detector.h

#ifndef PULSE_DETECTOR_H
#define PULSE_DETECTOR_H

#include <stdint.h>
#include <stdio.h>

#define MEAN_FILTER_SIZE 15
#define PULSE_BPM_SAMPLE_SIZE 10
#define RED_LED_CURRENT_ADJUSTMENT_MS 1 // 1000ms
#define PULSE_MAX_THRESHOLD 100
#define PULSE_MIN_THRESHOLD 2000
#define MAGIC_ACCEPTABLE_INTENSITY_DIFF 65000
// Structure for the DC removal filter
typedef struct dcFilter_t {
    int w;
    int result;
}dcFilter_t;

// Structure for the mean difference filter
typedef struct meanDiffFilter_t {
    int values[MEAN_FILTER_SIZE];
    uint8_t index;
    int sum;
    uint8_t count;
}meanDiffFilter_t;

// Structure for the Butterworth filter
typedef struct butterworthFilter_t {
    int v[2];
    int result;
}butterworthFilter_t;

// Enumeration for pulse detection states
typedef enum {
    PULSE_IDLE,
    PULSE_TRACE_UP,
    PULSE_TRACE_DOWN
}PulseDetectorState;

// Function prototypes
dcFilter_t dcRemoval(uint16_t x, int prev_w, int alpha);
int meanDiff(int M, meanDiffFilter_t *filterValues);
void lowPassButterworthFilter(int x, butterworthFilter_t *filterResult);
bool detectPulse(int sensor_value);
void balanceIntesities(int redLedDC, int IRLedDC);
void pulseDetectorMainLoop();
int getcurrentBPM();
#endif
