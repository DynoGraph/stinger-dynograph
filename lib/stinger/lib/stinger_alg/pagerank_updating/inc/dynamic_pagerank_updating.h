#ifndef STINGER_DYNAMIC_PAGERANK_UPDATING_H
#define STINGER_DYNAMIC_PAGERANK_UPDATING_H

#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include "stinger_net/stinger_alg.h"
#include "streaming_algorithm.h"

#define BASELINE 0
#define RESTART 1
#define DPR 2
#define DPR_HELD 3
#define DPR_ALL 4
#define NPR_ALG 5

struct spvect {
    int64_t nv;
    int64_t * idx;
    double * val;
};

namespace gt {
  namespace stinger {
    class PageRankUpdating : public IDynamicGraphAlgorithm
    {
    private:
        int n_alg_run = 0;
        int run_alg[NPR_ALG] = {0};
        const char *desc_alg[NPR_ALG] = {
                "pr delta-pr",
                "rpr delta-rpr",
                "dpr delta-dpr",
                "dprheld delta-dprheld",
                "dprall delta-dprall",
        };
        const char *pr_name[NPR_ALG] = {"pr", "pr_restart", "dpr", "dpr_held", "dpr_all"};

        double holdscale = 1.0;

        size_t data_desc_len;
        char * data_desc;

        double init_time, gather_time;
        double cwise_err;

        int iter = 0;
        int64_t nv;
        int64_t max_nv;

        int nbatch = 0;

        double * pr_val[NPR_ALG] = { NULL };
        int pr_val_init = NPR_ALG - 1; /* initialize into this one... */
        //double * old_pr_val[NPR_ALG] = { NULL }; /* Only used for BASELINE and RESTART */
        double * pr_val_delta[NPR_ALG] = { NULL };
        double * residual[NPR_ALG] = { NULL };
        int niter[NPR_ALG] = { 0 };
        double pr_pre_time[NPR_ALG] = { 0.0 };
        double pr_time[NPR_ALG] = { 0.0 };
        int64_t pr_vol[NPR_ALG] = { 0 };
        int64_t pr_nupd[NPR_ALG] = { 0 };
        double pr_nberr[NPR_ALG] = { 0.0 };
        double pr_resdiff[NPR_ALG] = { 0.0 };

        double alpha = 0.85;
        int maxiter = 100;

        struct spvect x = {0};
        struct spvect x_held = {0};
        struct spvect x_all = {0};
        struct spvect b = {0};
        struct spvect b_held = {0};
        struct spvect b_all = {0};
        struct spvect dpr = {0};
        struct spvect dpr_held = {0};
        struct spvect dpr_all = {0};

        int64_t * mark = NULL;
        double * v = NULL;
        double * dzero_workspace = NULL;
        double * workspace = NULL;
        int64_t * iworkspace = NULL;

    public:
        PageRankUpdating(std::string alg_variant, double alpha_arg, int nbatch_arg, double holdscale_arg);
        ~PageRankUpdating();

        // Overridden from IDynamicGraphAlgorithm
        std::string getName();
        int64_t getDataPerVertex();
        std::string getDataDescription();
        void onInit(stinger_registered_alg * alg);
        void onPre(stinger_registered_alg * alg);
        void onPost(stinger_registered_alg * alg);
    };
  }
}

#endif