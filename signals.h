#pragma once

// TODO: Use rational arithmetic

extern "C" {

const int SIGNAL_LENGTH = 20;

typedef struct cyclic_buf_s {
    float data[SIGNAL_LENGTH];
    size_t head;
} cyclic_buf_t;

typedef struct cyclic_buf2_s {
    float data[SIGNAL_LENGTH][2];
    size_t head;
} cyclic_buf2_t;

extern const cyclic_buf_t EMPTY_BUF;
extern const cyclic_buf2_t EMPTY_BUF2;

float cyclic_index(cyclic_buf_t* buf, int i);
float* cyclic_index2(cyclic_buf2_t* buf, int i);
void cyclic_insert(cyclic_buf_t* buf, float e);
void cyclic_insert2(cyclic_buf2_t* buf, float* e);

void convolve_with(cyclic_buf_t* signal,
                   int start,
                   int end,
                   const float* kernel,
                   int kernel_size,
                   cyclic_buf_t* out);

float convolve_point(cyclic_buf_t* signal,
                     int start,
                     const float* kernel,
                     int kernel_size);

}
