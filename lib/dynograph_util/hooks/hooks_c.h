#ifndef HOOKS_C_H
#define HOOKS_C_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void hooks_region_begin(const char* name);
void hooks_region_end();
void hooks_set_attr_u64(const char * key, uint64_t value);
void hooks_set_attr_i64(const char * key, int64_t value);
void hooks_set_attr_f64(const char * key, double value);
void hooks_set_attr_str(const char * key, const char* value);

#ifdef __cplusplus
}
#endif

#endif //HOOKS_C_H
