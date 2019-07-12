#ifndef SAMPLE_H
#define SAMPLE_H

#define FEATURE_CAPACITY 1000

class sample {
public:
    sample() { _mem = (uint64_t*)malloc(sizeof(uint64_t)*FEATURE_CAPACITY); }
    ~sample() { free(_mem); }

    uint64_t* _mem;
    int label, length;
    float ctr, pctr;
};

#endif
