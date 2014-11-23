#ifndef FFT_H_
#define FFT_H_

void fft(float *re, float *im, int m);

bool fft_isinterrupted();
void fft_interrupt();
void fft_disableinterrupt();

#endif /* FFT_H_ */
