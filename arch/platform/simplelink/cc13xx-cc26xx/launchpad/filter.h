#ifndef FILTER_H_
#define FILTER_H_

#define SIZE 1100

#include <math.h>

void threshold(float data[SIZE], float maxDiff) {
	int i;
	int j;
	int k;
	float swap;
	float window[7];
	
	for (i = 0; i < 7; i++) {
		window[i] = data[i];
	}
	
	for (i = 0; i < 6; i++) {
		j = i;
		
		for (k = i + 1; k < 7; k++) {
			if (window[j] > window[k]) {
				j = k;
			}
		}
		
		if (j != i) {
			swap = window[i];
			window[i] = window[j];
			window[j] = swap;
		}
	}
	
	if (data[0] - window[3] > maxDiff || data[0] - window[3] < -maxDiff) {
		data[0] = window[3];
	}
	
	for (i = 1; i < SIZE; i++) {
		if (data[i] - data[i-1] > maxDiff || data[i] - data[i-1] < -maxDiff) {
			data[i] = data[i-1];
		}
	}
}

void dc(float data[SIZE], float alpha) {
	int i;
	float w[SIZE];
	
	w[0] = 0;
	
	for (i = 1; i < SIZE; i++) {
		w[i] = (float)data[i] + alpha*w[i-1];
		data[i] = w[i] - w[i-1];
	}
}

void meanMedian(float data[SIZE], const int window) {
	float values[20];
	float sum = 0;
	int index = 0;
	int count = 0;
	int i;

	for (i = 0; i < window; i++) {
	    values[i] = 0;
	}

	for (i = 0; i < SIZE; i++) {
		float avg;

		sum -= values[index];
		values[index] = data[i];
		sum += values[index];

		index++;
		index = index % window;

		if (count < window) {
			count++;
		}

		avg = sum / (float)count;

		data[i] = avg - data[i];
	}
}

void butterworth(float data[SIZE]) {
	float v[2];
	int i;
	
	v[0] = 0.0;
	v[1] = 0.0;
	
	for (i = 0; i < SIZE; i++) {
		v[0] = v[1];
		// constants for cut-off frequency Fc = 4Hz
		v[1] = (1.121602444751934047e-1 * data[i]) + (0.77567951104961319064 * v[0]);
		
		data[i] = v[0] + v[1];
	}
}

float max(float data[SIZE], int begin, int end) {
	int i;
	float m;
	
	m = data[begin];
	
	for (i = begin; i < end; i++) {
		if (data[i] > m) {
			m = data[i];
		}
	}
	
	return m;
}

float average(float *data, int size) {
    float sum;
    int i;

    sum = 0;
    for (i = 0; i < size; i++) {
        sum += data[i];
    }

    return sum / (float)size;
}

float heartrate(float data[SIZE]) {
	bool peak;
	int beats;
	int i;
	float threshold;
	
	peak = false;
	beats = 0;

	threshold = max(data, 100, 300) / 2.0; // multiplier should be tweaked depending on LED current

	// offset to the right by 100 after dc filter
	for (i = 100; i < 200; i++) {
        if (!peak && data[i] > threshold) {
            peak = true;
            beats++;
        }

        if (peak && data[i] <= threshold) {
            peak = false;
        }
    }

	for (i = 200; i < SIZE - 100; i++) {
	    threshold = max(data, i - 100, i + 100) / 2.0;

		if (!peak && data[i] > threshold) {
			peak = true;
			beats++;
		}

		if (peak && data[i] <= threshold) {
			peak = false;
		}
	}

	threshold = max(data, SIZE - 200, SIZE) / 2.0;

    for (i = SIZE - 100; i < SIZE; i++) {
        if (!peak && data[i] > threshold) {
            peak = true;
            beats++;
        }

        if (peak && data[i] <= threshold) {
            peak = false;
        }
    }

	// offset to the right by 100 after dc filter => 10 seconds instead of 11
	return (float)beats * 6000.0 / (float)(SIZE - 100);
}

float heartrate2(float data[SIZE]) {
    bool peak;
    int beats;
    int i;
    float threshold;
    float beatTime[50];

    peak = false;
    beats = 0;

    threshold = max(data, 100, 300) / 2.0; // multiplier should be tweaked depending on LED current

    // offset to the right by 100 after dc filter
    for (i = 100; i < 200; i++) {
        if (!peak && data[i] > threshold) {
            peak = true;
            beatTime[beats] = (float)i / 100.0;
            beats++;
        }

        if (peak && data[i] <= threshold) {
            peak = false;
        }
    }

    for (i = 200; i < SIZE - 100; i++) {
        threshold = max(data, i - 100, i + 100) / 2.0;

        if (!peak && data[i] > threshold) {
            peak = true;
            beatTime[beats] = (float)i / 100.0;
            beats++;
        }

        if (peak && data[i] <= threshold) {
            peak = false;
        }
    }

    threshold = max(data, SIZE - 200, SIZE) / 2.0;

    for (i = SIZE - 100; i < SIZE; i++) {
        if (!peak && data[i] > threshold) {
            peak = true;
            beatTime[beats] = (float)i / 100.0;
            beats++;
        }

        if (peak && data[i] <= threshold) {
            peak = false;
        }
    }

    for (i = 0; i < beats - 1; i++) {
        beatTime[i] = beatTime[i+1] - beatTime[i];
        beatTime[i] = 60.0 / beatTime[i];
    }

    return average(beatTime, beats - 1);
}

float spo2(float dataIR[SIZE], float dataR[SIZE]) {
	float irSquareSum;
	float rSquareSum;
	float rmsRatio;
	int i;
	
	irSquareSum = 0.0;
	rSquareSum = 0.0;
	rmsRatio = 0.0;
	
	// offset to the right by 100 after dc filter
	for (i = 100; i < SIZE; i++) {
		irSquareSum += dataIR[i] * dataIR[i];
		rSquareSum += dataR[i] * dataR[i];
	}

	rmsRatio = log(sqrt(rSquareSum / (SIZE - 100))) / log(sqrt(irSquareSum / (SIZE - 100)));
	
	//include wavelengths into formula
	//rmsRatio = (log(sqrt(rSquareSum / (SIZE - 100))) * 660.0) / (log(sqrt(irSquareSum / (SIZE - 100))) * 880.0);
	
	//constants require calibration
	return 115.0 - 18.0 * rmsRatio;
}

#endif
