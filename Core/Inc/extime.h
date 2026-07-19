#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

void delayMillis(size_t ms);
void delayMicros(size_t us);
int64_t timeMicros();
int64_t timeMillis();

#ifdef __cplusplus
}
#endif
