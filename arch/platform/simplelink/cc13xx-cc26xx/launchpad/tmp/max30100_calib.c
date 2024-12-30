// pulse_detector.c
#include "contiki.h"
#include "clock.h"
#include "max30100_calib.h"
#include "max30100.h"

// Global variables
static PulseDetectorState currentPulseDetectorState = PULSE_IDLE;
static uint8_t bpmIndex = 0;
static uint8_t valuesBPMCount = 0;
static uint32_t lastREDLedCurrentCheck = 0;
static int redLEDCurrent = 50; // Initial RED LED current
//static int IrLedCurrent = 50.0; // Initial IR LED current
static int valuesBPM[PULSE_BPM_SAMPLE_SIZE] = {0};
static int valuesBPMSum = 0;
static int lastBeatThreshold = 0;
static int currentBPM = 0;

// Function to perform DC removal using the provided filter
dcFilter_t dcRemoval(uint16_t x, int prev_w, int alpha) {
    dcFilter_t filtered;
    filtered.w = x + alpha * prev_w;
    filtered.result = filtered.w - prev_w;
    return filtered;
}

// Function to perform mean difference filtering
int meanDiff(int M, meanDiffFilter_t *filterValues) {
    int avg = 0;
    filterValues->sum -= filterValues->values[filterValues->index];
    filterValues->values[filterValues->index] = M;
    filterValues->sum += filterValues->values[filterValues->index];
    filterValues->index++;
    filterValues->index = filterValues->index % MEAN_FILTER_SIZE;
    if (filterValues->count < MEAN_FILTER_SIZE)
        filterValues->count++;
    avg = filterValues->sum / filterValues->count;
    return avg - M;
}

// Function to perform low-pass Butterworth filtering
void lowPassButterworthFilter(int x, butterworthFilter_t *filterResult) {
    filterResult->v[0] = filterResult->v[1];
    // Fs = 100Hz and Fc = 10Hz
    filterResult->v[1] = (2.452372752527856026e-1 * x) + (0.50952544949442879485 * filterResult->v[0]);
    filterResult->result = filterResult->v[0] + filterResult->v[1];
}

// Function to detect pulses and calculate heart rate
bool detectPulse(int sensor_value) {
    static int prev_sensor_value = 0;
    static uint8_t values_went_down = 0;
    static uint32_t currentBeat = 0;
    static uint32_t lastBeat = 0;

    if (sensor_value > PULSE_MAX_THRESHOLD) {
        currentPulseDetectorState = PULSE_IDLE;
        prev_sensor_value = 0;
        lastBeat = 0;
        currentBeat = 0;
        values_went_down = 0;
        lastBeatThreshold = 0;
        return false;
    }

    switch (currentPulseDetectorState) {
        case PULSE_IDLE:
            if (sensor_value >= PULSE_MIN_THRESHOLD) {
                currentPulseDetectorState = PULSE_TRACE_UP;
                values_went_down = 0;
            }
            break;

        case PULSE_TRACE_UP:
            if (sensor_value > prev_sensor_value) {
                currentBeat = clock_seconds();
                lastBeatThreshold = sensor_value;
            } else {
                uint32_t beatDuration = currentBeat - lastBeat;
                lastBeat = currentBeat;
                int rawBPM = 0;
                if (beatDuration > 0)
                    rawBPM = 60000/ (int)beatDuration;
                valuesBPM[bpmIndex] = rawBPM;
                valuesBPMSum = 0;
                for (int i = 0; i < PULSE_BPM_SAMPLE_SIZE; i++) {
                    valuesBPMSum += valuesBPM[i];
                }
                bpmIndex++;
                bpmIndex = bpmIndex % PULSE_BPM_SAMPLE_SIZE;
                if (valuesBPMCount < PULSE_BPM_SAMPLE_SIZE)
                    valuesBPMCount++;
                currentBPM = valuesBPMSum / valuesBPMCount;

                currentPulseDetectorState = PULSE_TRACE_DOWN;
                return true;
            }
            break;
        case PULSE_TRACE_DOWN:
            if (sensor_value < prev_sensor_value) {
                values_went_down++;
            }
            if (sensor_value < PULSE_MIN_THRESHOLD) {
                currentPulseDetectorState = PULSE_IDLE;
            }
            break;
    }

    prev_sensor_value = sensor_value;
    return false;
}

// Function to balance intensities of IR and RED LEDs
void balanceIntensities(int redLedDC, int IRLedDC) {
    if (clock_seconds() - lastREDLedCurrentCheck >= RED_LED_CURRENT_ADJUSTMENT_MS) {
        if (IRLedDC - redLedDC > MAGIC_ACCEPTABLE_INTENSITY_DIFF && redLEDCurrent < 50) {
            redLEDCurrent++;
            printf("RED LED Current +\n");
        } else if (redLedDC - IRLedDC > MAGIC_ACCEPTABLE_INTENSITY_DIFF && redLEDCurrent > 0) {
            redLEDCurrent--;
            printf("RED LED Current -\n");
        }

        lastREDLedCurrentCheck = clock_seconds();
    }
}
int getcurrentBPM(){
    return currentBPM;
}
// Main loop
void pulseDetectorMainLoop() {
    // Initialize your setup here

    // Sample sensor values (replace with actual sensor readings)
    int rawIR = 50000;
    int rawRed = 30000;

    // Initialize filter parameters
    struct dcFilter_t dcFilterIR = {0, 0};
    struct meanDiffFilter_t meanDiffFilterIR = {{0}, 0, 0, 0};
    struct butterworthFilter_t butterworthFilterIR = {{0}, 0};

    // Initialize other variables
    // ...

    // Main loop
    while (1) {
        // DC removal
        dcFilterIR = dcRemoval(rawIR, dcFilterIR.w, 0.95);

        // Mean difference filtering
        int meanDiffResult = meanDiff(dcFilterIR.result, &meanDiffFilterIR);

        // Low-pass Butterworth filtering
        butterworthFilterIR.result = 0;
        lowPassButterworthFilter(meanDiffResult, &butterworthFilterIR);

        // Detect pulse and calculate heart rate
        if (detectPulse(butterworthFilterIR.result)) {
            // Print heart rate
            printf("Heart Rate: %d BPM\n", currentBPM);
        }

        // Balance IR and RED LED intensities
        balanceIntesities(rawRed, rawIR);

        // Sleep for a while to simulate time passing
        clock_wait(100);  // 100ms sleep
    }
}

