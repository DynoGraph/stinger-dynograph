#if !defined(DYNOGRAPH_H_)
#define DYNOGRAPH_H_
#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>

struct dynograph_edge {
    int64_t src;
    int64_t dst;
    int64_t weight;
    int64_t timestamp;
};

struct dynograph_dataset {
    int64_t num_batches;
    int64_t num_edges;
    int64_t directed;
    struct dynograph_edge edges[0];
};

struct dynograph_edge_batch {
    const int64_t num_edges;
    const int64_t directed;
    const struct dynograph_edge* edges;
};

void dynograph_message(const char* fmt, ...);
void dynograph_error(const char* fmt, ...);

struct dynograph_dataset * dynograph_load_dataset(const char* path, int64_t num_batches);
struct dynograph_edge_batch dynograph_get_batch(const struct dynograph_dataset * dataset, int64_t batch_id);
void dynograph_free_dataset(struct dynograph_dataset * dataset);
int64_t dynograph_get_timestamp_for_window(const struct dynograph_dataset * dataset, int64_t batch_id, int64_t window_size);

#endif /* __DYNOGRAPH_H_ */
