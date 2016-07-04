#include "signals.h"

extern "C" {

const cyclic_buf_t EMPTY_BUF = {
    {},
    0
};

const cyclic_buf2_t EMPTY_BUF2 = {
    {},
    0
};

float cyclic_index(cyclic_buf_t* buf, int i) {
    return buf->data[(buf->head + i) % SIGNAL_LENGTH];
}

float* cyclic_index2(cyclic_buf2_t* buf, int i) {
    return buf->data[(buf->head + i) % SIGNAL_LENGTH];
}

void cyclic_insert(cyclic_buf_t* buf, float e) {
    if (buf->head == 0) {
        buf->head = SIGNAL_LENGTH - 1;
    } else {
        buf->head--;
    }
    buf->data[buf->head] = e;
}

void cyclic_insert2(cyclic_buf2_t* buf, float* e) {
    if (buf->head == 0) {
        buf->head = SIGNAL_LENGTH - 1;
    } else {
        buf->head--;
    }
    buf->data[buf->head][0] = e[0];
    buf->data[buf->head][1] = e[1];
}

void convolve_with(cyclic_buf_t* signal,
                   int start,
                   int end,
                   const float* kernel,
                   int kernel_size,
                   cyclic_buf_t* out) {
    int i;
    for (i = end - 1; i >= start; i--) {
        int j;
        float val = 0;
        for (j = 0; j < kernel_size; j++) {
            val += kernel[j] * cyclic_index(signal, i + j);           
        }
        cyclic_insert(out, val);
    }
}

float convolve_point(cyclic_buf_t* signal,
                     int i,
                     const float* kernel,
                     int kernel_size) {
    float val = 0;
    int j;
    for (j = 0; j < kernel_size; j++) {
        val += kernel[j] * cyclic_index(signal, i + j);           
    }
    return val;
}

}
